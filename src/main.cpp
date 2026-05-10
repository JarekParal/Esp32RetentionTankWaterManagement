/* HANKERILA HKL-EA8 — ESP32 retention-tank controller
 *
 * Web UI (Ethernet, LAN8720 PHY) for 8 solenoid valves wired through a
 * PCF8574 I2C expander, plus an HC-SR04 ultrasonic level sensor.
 * Build/flash with `pio run -e nodemcu-32s -t upload` (USB) or
 * `pio run -e nodemcu-32s-ota -t upload` (network, ArduinoOTA).
 *
 * espressif32@6.10.0 / Arduino core 2.0.17 — see platformio.ini.
 */

#include <ETH.h>
#include <WebServer.h>
#include <ArduinoOTA.h>
#include <PCF8574.h>
#include <stdarg.h>

constexpr int ULTRASOUND_TRIGER_PIN = 15; // RX on connector
constexpr int ULTRASOUND_ECHO_PIN = 16;   // TX on connector

#define ETH_ADDR 0
#define ETH_POWER_PIN -1
#define ETH_MDC_PIN 23
#define ETH_MDIO_PIN 18
#define ETH_TYPE ETH_PHY_LAN8720
#undef ETH_CLK_MODE                       // ETH.h defaults to ETH_CLOCK_GPIO0_IN; this board uses GPIO17 OUT
#define ETH_CLK_MODE ETH_CLOCK_GPIO17_OUT

IPAddress local_ip(uint32_t(0));    // 0 => use DHCP
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);
IPAddress dns(192, 168, 1, 1);

WebServer server(80);
PCF8574 pcf8574_re(0x24, 4, 5);

// ---------------- Log ring buffer ----------------
constexpr size_t LOG_LINE_LEN = 96;
constexpr size_t LOG_BUF_LINES = 64;
static char log_buf[LOG_BUF_LINES][LOG_LINE_LEN];
static uint32_t log_seq = 0; // next slot index; line at slot ((seq-1) % LOG_BUF_LINES) is most recent

static void wlog_println(const char *msg)
{
  Serial.println(msg);
  size_t len = strlen(msg);
  if (len >= LOG_LINE_LEN) len = LOG_LINE_LEN - 1;
  size_t slot = log_seq % LOG_BUF_LINES;
  memcpy(log_buf[slot], msg, len);
  log_buf[slot][len] = '\0';
  log_seq++;
}

static void wlog_println(const String &msg) { wlog_println(msg.c_str()); }

static void wlog_printf(const char *fmt, ...)
{
  char tmp[LOG_LINE_LEN];
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(tmp, sizeof(tmp), fmt, ap);
  va_end(ap);
  wlog_println(tmp);
}

// ---------------- Valve state ----------------
// bit i = valve (i+1) is OPEN. PCF8574 is active-low: write 0 to OPEN, 1 to CLOSE.
static uint8_t relay_mask = 0;

static void set_valve(int n, bool open)
{
  if (n < 1 || n > 8) return;
  uint8_t bit = 1u << (n - 1);
  bool was = relay_mask & bit;
  if (open) relay_mask |= bit; else relay_mask &= ~bit;
  pcf8574_re.digitalWrite(n - 1, open ? 0 : 1);
  if (was != open) wlog_printf("Valve %d -> %s", n, open ? "OPEN" : "CLOSED");
}

// ---------------- Ultrasonic (cached) ----------------
static volatile float cached_distance_cm = 0.0f;
static unsigned long last_distance_ms = 0;
constexpr unsigned long DISTANCE_REFRESH_MS = 1000;

static float read_distance_cm()
{
  constexpr float SOUND_SPEED = 0.034f;
  digitalWrite(ULTRASOUND_TRIGER_PIN, LOW);
  delayMicroseconds(5);
  digitalWrite(ULTRASOUND_TRIGER_PIN, HIGH);
  delayMicroseconds(20);
  digitalWrite(ULTRASOUND_TRIGER_PIN, LOW);
  long duration = pulseIn(ULTRASOUND_ECHO_PIN, HIGH, 30000); // 30 ms timeout (~5 m max)
  return (duration * SOUND_SPEED) / 2.0f;
}

// ---------------- HTTP handlers ----------------
static void json_escape_into(String &out, const char *s)
{
  for (const char *p = s; *p; p++) {
    char c = *p;
    if (c == '"' || c == '\\') { out += '\\'; out += c; }
    else if (c == '\n') out += "\\n";
    else if (c == '\r') out += "\\r";
    else if (c == '\t') out += "\\t";
    else if ((unsigned char)c < 0x20) { char buf[8]; snprintf(buf, sizeof(buf), "\\u%04x", c); out += buf; }
    else out += c;
  }
}

static void server_handle_poll()
{
  uint32_t since = (uint32_t)server.arg("since").toInt();
  uint32_t oldest = (log_seq > LOG_BUF_LINES) ? (log_seq - LOG_BUF_LINES) : 0;
  uint32_t from = (since < oldest) ? oldest : since;
  if (from > log_seq) from = log_seq;

  String json;
  json.reserve(1024);
  json += "{\"seq\":";
  json += log_seq;
  json += ",\"relays\":";
  json += relay_mask;
  json += ",\"distance_cm\":";
  json += String(cached_distance_cm, 1);
  json += ",\"uptime_ms\":";
  json += millis();
  json += ",\"lines\":[";
  bool first = true;
  for (uint32_t i = from; i < log_seq; i++) {
    if (!first) json += ',';
    first = false;
    json += '"';
    json_escape_into(json, log_buf[i % LOG_BUF_LINES]);
    json += '"';
  }
  json += "]}";
  server.send(200, "application/json", json);
}

static void server_handle_sw()
{
  int n = server.arg("n").toInt();
  int on = server.arg("on").toInt();
  if (n < 1 || n > 8) {
    server.send(400, "application/json", "{\"error\":\"invalid n\"}");
    return;
  }
  set_valve(n, on != 0);
  String j;
  j.reserve(48);
  j += "{\"n\":";
  j += n;
  j += ",\"on\":";
  j += (on ? 1 : 0);
  j += ",\"relays\":";
  j += relay_mask;
  j += "}";
  server.send(200, "application/json", j);
}

// Back-compat: GET /SW?LED=onN | offN
static void server_handle_sw_legacy()
{
  String state = server.arg("LED");
  for (int i = 0; i < 8; ++i) {
    if (state == "on" + String(i + 1)) set_valve(i + 1, true);
    else if (state == "off" + String(i + 1)) set_valve(i + 1, false);
  }
  server.send(200, "text/plain", "OK");
}

// Self-contained UI: CSS + JS inline, no external resources.
static const char PAGE_HTML[] PROGMEM = R"HTML(<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Retention Tank</title>
<style>
:root{--bg:#0f1419;--panel:#1a1f26;--panel2:#222831;--border:#2a3441;--text:#e6e9ee;--muted:#8a96a8;--accent:#3b82f6;--ok:#10b981;--off:#475569}
@media(prefers-color-scheme:light){:root{--bg:#f6f7f9;--panel:#fff;--panel2:#f1f3f6;--border:#e4e7eb;--text:#0f172a;--muted:#64748b;--accent:#2563eb;--ok:#16a34a;--off:#94a3b8}}
*{box-sizing:border-box}
body{margin:0;background:var(--bg);color:var(--text);font:14px/1.4 system-ui,-apple-system,Segoe UI,Roboto,sans-serif;min-height:100vh}
header.top{display:flex;align-items:center;justify-content:space-between;padding:14px 20px;background:var(--panel);border-bottom:1px solid var(--border);flex-wrap:wrap;gap:10px}
header.top h1{margin:0;font-size:18px;font-weight:600}
.metrics{display:flex;gap:18px;font-size:13px}
.metric{display:flex;flex-direction:column;align-items:flex-end}
.metric label{color:var(--muted);font-size:11px;text-transform:uppercase;letter-spacing:.04em}
.metric strong{font-size:15px;font-variant-numeric:tabular-nums}
main{max-width:1100px;margin:0 auto;padding:20px;display:grid;grid-template-columns:1fr;gap:20px}
@media(min-width:900px){main{grid-template-columns:1fr 1fr}}
section{background:var(--panel);border:1px solid var(--border);border-radius:8px;padding:16px}
section>h2{margin:0 0 12px;font-size:13px;font-weight:600;color:var(--muted);text-transform:uppercase;letter-spacing:.05em}
.valve-grid{display:grid;grid-template-columns:repeat(auto-fill,minmax(120px,1fr));gap:10px}
.valve{background:var(--panel2);border:1px solid var(--border);border-radius:6px;padding:12px;display:flex;flex-direction:column;gap:8px;cursor:pointer;transition:border-color .15s,background .15s,opacity .15s;user-select:none}
.valve:hover{border-color:var(--accent)}
.valve.open{background:color-mix(in srgb,var(--ok) 18%,var(--panel2));border-color:var(--ok)}
.valve .name{font-weight:600;font-size:13px}
.valve .status{font-size:11px;color:var(--muted);text-transform:uppercase;letter-spacing:.04em}
.valve.open .status{color:var(--ok)}
.valve .toggle{align-self:flex-end;width:36px;height:20px;background:var(--off);border-radius:11px;position:relative;transition:background .15s;flex-shrink:0}
.valve .toggle::after{content:"";position:absolute;top:2px;left:2px;width:16px;height:16px;background:#fff;border-radius:50%;transition:transform .15s}
.valve.open .toggle{background:var(--ok)}
.valve.open .toggle::after{transform:translateX(16px)}
.valve.pending{opacity:.55;pointer-events:none}
.term-head{display:flex;align-items:center;justify-content:space-between;margin-bottom:10px}
.term-head h2{margin:0;font-size:13px;font-weight:600;color:var(--muted);text-transform:uppercase;letter-spacing:.05em}
.term-head button{background:transparent;border:1px solid var(--border);color:var(--muted);font:inherit;font-size:12px;padding:4px 10px;border-radius:4px;cursor:pointer}
.term-head button:hover{color:var(--text);border-color:var(--accent)}
#terminal{margin:0;background:#000;color:#cfe;border:1px solid var(--border);border-radius:6px;padding:12px;height:340px;overflow-y:auto;font:12px/1.5 ui-monospace,Menlo,Consolas,monospace;white-space:pre-wrap;word-break:break-word}
#terminal:empty::before{content:"(no log lines yet)";color:#556}
.dot{display:inline-block;width:7px;height:7px;border-radius:50%;background:var(--off);margin-left:6px;vertical-align:middle;transition:background .2s}
.dot.live{background:var(--ok);box-shadow:0 0 6px var(--ok)}
</style>
</head>
<body>
<header class="top">
  <h1>Retention Tank<span id="conn" class="dot" title="connection"></span></h1>
  <div class="metrics">
    <div class="metric"><label>Tank distance</label><strong id="distance">&mdash;</strong></div>
    <div class="metric"><label>Uptime</label><strong id="uptime">&mdash;</strong></div>
  </div>
</header>
<main>
  <section>
    <h2>Solenoid Valves</h2>
    <div id="valves" class="valve-grid"></div>
  </section>
  <section>
    <div class="term-head"><h2>Device Log</h2><button id="clearLog">Clear</button></div>
    <pre id="terminal"></pre>
  </section>
</main>
<script>
(()=>{
  const N=8;
  let lastSeq=0, relays=0, paused=false, pending=new Set();
  const $v=document.getElementById('valves'), $t=document.getElementById('terminal'),
        $d=document.getElementById('distance'), $u=document.getElementById('uptime'),
        $c=document.getElementById('conn');
  const fmtUp=ms=>{const s=Math.floor(ms/1000),h=Math.floor(s/3600),m=Math.floor((s%3600)/60);
    return (h?h+'h ':'')+((h||m)?m+'m ':'')+(s%60)+'s';};
  function render(){
    $v.innerHTML='';
    for(let i=0;i<N;i++){
      const open=!!(relays&(1<<i));
      const el=document.createElement('div');
      el.className='valve'+(open?' open':'')+(pending.has(i+1)?' pending':'');
      el.dataset.n=i+1;
      el.innerHTML='<div class="name">Valve '+(i+1)+'</div><div class="status">'+(open?'Open':'Closed')+'</div><div class="toggle"></div>';
      el.addEventListener('click',()=>toggle(i+1,!open));
      $v.appendChild(el);
    }
  }
  async function toggle(n,on){
    pending.add(n); render();
    try{
      const r=await fetch('/sw?n='+n+'&on='+(on?1:0));
      const j=await r.json();
      if(typeof j.relays==='number') relays=j.relays;
    }catch(e){}
    pending.delete(n); render();
  }
  function appendLines(lines){
    if(!lines||!lines.length) return;
    const atBottom=$t.scrollHeight-$t.scrollTop-$t.clientHeight<30;
    for(const l of lines) $t.appendChild(document.createTextNode(l+'\n'));
    if(atBottom) $t.scrollTop=$t.scrollHeight;
  }
  async function poll(){
    if(paused) return;
    try{
      const r=await fetch('/poll?since='+lastSeq);
      if(!r.ok) throw 0;
      const j=await r.json();
      $c.classList.add('live');
      if(j.seq!==undefined) lastSeq=j.seq;
      if(typeof j.relays==='number'&&j.relays!==relays){relays=j.relays;render();}
      if(j.distance_cm!==undefined) $d.textContent=j.distance_cm.toFixed(1)+' cm';
      if(j.uptime_ms!==undefined) $u.textContent=fmtUp(j.uptime_ms);
      appendLines(j.lines);
    }catch(e){$c.classList.remove('live');}
  }
  document.getElementById('clearLog').addEventListener('click',()=>{$t.textContent='';});
  document.addEventListener('visibilitychange',()=>{paused=document.hidden;});
  render(); poll(); setInterval(poll,750);
})();
</script>
</body>
</html>
)HTML";

static void server_handle_root()
{
  server.send_P(200, "text/html", PAGE_HTML);
}

void setup()
{
  pinMode(ULTRASOUND_TRIGER_PIN, OUTPUT);
  pinMode(ULTRASOUND_ECHO_PIN, INPUT);

  Serial.begin(115200);
  delay(1000);

  wlog_println("Starting Ethernet...");
  ETH.begin(ETH_ADDR, ETH_POWER_PIN, ETH_MDC_PIN, ETH_MDIO_PIN, ETH_TYPE, ETH_CLK_MODE);
  if (ETH.config(local_ip, gateway, subnet, dns, dns) == false) {
    wlog_println("LAN8720 configuration failed.");
  } else {
    wlog_println("LAN8720 configuration success.");
  }

  delay(1000);
  wlog_printf("ETH IP: %s", ETH.localIP().toString().c_str());

  for (int i = 0; i < 8; i++) pcf8574_re.pinMode(i, OUTPUT);
  pcf8574_re.begin();
  for (int i = 0; i < 8; i++) pcf8574_re.digitalWrite(i, 1); // all valves CLOSED
  relay_mask = 0;

  server.on("/", server_handle_root);
  server.on("/sw", server_handle_sw);
  server.on("/poll", server_handle_poll);
  server.on("/SW", server_handle_sw_legacy); // back-compat
  server.begin();
  wlog_println("Web server started on :80");

  ArduinoOTA.setHostname("retention-tank");
  // ArduinoOTA.setPassword("change-me"); // uncomment and set in upload_flags --auth=
  ArduinoOTA.onStart([]() { wlog_println("OTA: start"); });
  ArduinoOTA.onEnd([]() { wlog_println("OTA: end"); });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    static int last_pct = -10;
    int pct = total ? (int)((progress * 100u) / total) : 0;
    if (pct >= last_pct + 10) { wlog_printf("OTA: %d%%", pct); last_pct = pct; }
  });
  ArduinoOTA.onError([](ota_error_t error) { wlog_printf("OTA error: %u", (unsigned)error); });
  ArduinoOTA.begin();
  wlog_println("OTA ready");
}

void loop()
{
  ArduinoOTA.handle();
  server.handleClient();

  unsigned long now = millis();
  if (now - last_distance_ms >= DISTANCE_REFRESH_MS) {
    cached_distance_cm = read_distance_cm();
    last_distance_ms = now;
  }
}
