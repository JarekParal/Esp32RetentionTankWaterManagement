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

static void server_handle_closeall()
{
  for (int i = 1; i <= 8; i++) set_valve(i, false);
  String j;
  j.reserve(24);
  j += "{\"relays\":";
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

// UI is held in web/index.html and embedded into the firmware via
// platformio.ini's board_build.embed_txtfiles. The IDF build system exposes
// _binary_<path>_start / _end symbols (slashes and dots become underscores).
extern const uint8_t INDEX_HTML_START[] asm("_binary_web_index_html_start");
extern const uint8_t INDEX_HTML_END[]   asm("_binary_web_index_html_end");

static void server_handle_root()
{
  // _end - _start is the file size; embed_txtfiles also appends a NUL, so
  // subtract one to avoid sending it as part of the body.
  size_t len = (INDEX_HTML_END - INDEX_HTML_START);
  if (len > 0 && INDEX_HTML_START[len - 1] == 0) len -= 1;
  server.send_P(200, "text/html", (PGM_P)INDEX_HTML_START, len);
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
  server.on("/closeall", server_handle_closeall);
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
