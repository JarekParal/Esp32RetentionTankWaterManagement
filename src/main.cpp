/* HANKERILA HKL-EA8 — ESP32 retention-tank controller
 *
 * Web UI (Ethernet, LAN8720 PHY) for 8 solenoid valves wired through a
 * PCF8574 I2C expander, plus an HC-SR04 ultrasonic level sensor.
 * Build/flash with `pio run -e nodemcu-32s -t upload` (USB) or
 * `pio run -e nodemcu-32s-ota -t upload` (network, ArduinoOTA).
 *
 * pioarduino/platform-espressif32@55.03.38 — Arduino core 3.3.8, ESP-IDF 5.5.4
 * see platformio.ini
 */

#include <ETH.h>
#include <WebServer.h>
#include <ArduinoOTA.h>
#include <PCF8574.h>
#include <Preferences.h>
#include <Wire.h>

#include <stdint.h>

#include "log_buffer.h"
#include "util.h"
#include "history.h"
#include "oled.h"
#include "modbus_srv.h"

constexpr int ULTRASOUND_TRIGER_PIN = 32; // RX on connector
constexpr int ULTRASOUND_ECHO_PIN = 33;   // TX on connector
// R47+R106 removed; PCF8574 /INT (open-drain) wired directly to GPIO12 — see doc/HW modification notes.md
constexpr int INPUT_INT_PIN = 12;

#define ETH_ADDR 0
#define ETH_POWER_PIN -1
#define ETH_MDC_PIN 23
#define ETH_MDIO_PIN 18
#define ETH_TYPE ETH_PHY_LAN8720
#undef ETH_CLK_MODE // ETH.h defaults to ETH_CLOCK_GPIO0_IN; this board uses GPIO17 OUT
#define ETH_CLK_MODE ETH_CLOCK_GPIO17_OUT

IPAddress local_ip(uint32_t(0)); // 0 => use DHCP
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);
IPAddress dns(192, 168, 1, 1);

WebServer server(80);
PCF8574 pcf8574_re(0x24, 4, 5);
PCF8574 pcf8574_in(0x26, 4, 5);

// ---------------- Valve state ----------------
// Non-static so modbus_srv.cpp can reach these via extern. (Equally readable
// over the bus and the HTTP endpoints, so they're truly shared state.)
// bit i = valve (i+1) is OPEN. PCF8574 is active-low: write 0 to OPEN, 1 to CLOSE.
uint8_t relay_mask = 0;

// bit i = input (i+1) is ACTIVE. PCF8574 is active-low: read 0 means active.
uint8_t input_mask = 0;
static volatile bool inputs_changed = false;

/// @brief Drive a single valve to open or closed and log the transition.
/// @param n    1-based valve index (1..8); out-of-range calls are ignored.
/// @param open `true` to open the valve, `false` to close it.
/// @note PCF8574 is active-low — this function inverts the level so callers
///       can think in terms of OPEN / CLOSED.
void set_valve(int n, bool open)
{
  if (n < 1 || n > 8)
    return;
  uint8_t bit = 1u << (n - 1);
  bool was = (relay_mask & bit) != 0;
  if (open)
    relay_mask |= bit;
  else
    relay_mask &= ~bit;
  pcf8574_re.digitalWrite(n - 1, open ? 0 : 1);
  if (was != open)
    wlog_printf("Valve %d -> %s", n, open ? "OPEN" : "CLOSED");
}

// ---------------- Ultrasonic (cached) ----------------
float cached_distance_cm = 0.0f;
static unsigned long last_distance_ms = 0;
constexpr unsigned long DISTANCE_REFRESH_MS = 10000;

// ---------------- OLED refresh ----------------
static unsigned long last_oled_ms = 0;
constexpr unsigned long OLED_REFRESH_MS = 200;

// 10-min × 144 short ring = 24 h, throttled to one NVS write per hour.
// 1-day × 30 long ring = 30 d, one NVS write per daily rollover (~1/day).
History distance_history(
    "dist",
    History::RingConfig{/*period_sec=*/600, /*slot_count=*/144, /*min_persist_sec=*/3600},
    History::RingConfig{/*period_sec=*/86400, /*slot_count=*/30, /*min_persist_sec=*/0});

// Water flow rate (L/min) sampled once a minute from the pulse counter.
// Same ring config as distance so the UI can reuse the same chart code.
History water_history(
    "water",
    History::RingConfig{/*period_sec=*/600, /*slot_count=*/144, /*min_persist_sec=*/3600},
    History::RingConfig{/*period_sec=*/86400, /*slot_count=*/30, /*min_persist_sec=*/0});

// Flow-rate sampler state. Every WATER_SAMPLE_MS we record the number of
// pulses received in the last window as one L/min sample.
static unsigned long last_water_sample_ms = 0;
static uint32_t last_water_pulse_count = 0;
constexpr unsigned long WATER_SAMPLE_MS = 60000;

// ---------------- Digital inputs (PCF8574 @ 0x26, /INT → GPIO12) ----------------

// Water meter on Input 1 — one rising edge = one liter. Counter is persisted
// to NVS and restored at boot. 32 bits hold ~4 billion liters; the household
// will run out of plumbing first.
uint32_t water_pulse_count = 0;
static bool water_pulse_count_dirty = false;
static unsigned long last_water_persist_ms = 0;
constexpr unsigned long WATER_TOTAL_PERSIST_MS = 5UL * 60UL * 1000UL;
static constexpr const char *WATER_TOTAL_NVS_NS = "watercnt";
static constexpr const char *WATER_TOTAL_NVS_KEY = "total";

/// @brief Load the persisted water pulse counter from NVS.
static void load_water_total()
{
  Preferences prefs;
  if (!prefs.begin(WATER_TOTAL_NVS_NS, /*readOnly=*/true))
    return;
  water_pulse_count = prefs.getUInt(WATER_TOTAL_NVS_KEY, 0);
  prefs.end();
  last_water_pulse_count = water_pulse_count;
  wlog_printf("Water total restored: %u L", (unsigned)water_pulse_count);
}

/// @brief Persist the current water pulse counter to NVS.
/// @return True when the NVS write succeeds.
static bool persist_water_total()
{
  Preferences prefs;
  if (!prefs.begin(WATER_TOTAL_NVS_NS, /*readOnly=*/false))
    return false;
  size_t written = prefs.putUInt(WATER_TOTAL_NVS_KEY, water_pulse_count);
  prefs.end();
  if (written == sizeof(water_pulse_count))
  {
    water_pulse_count_dirty = false;
    last_water_persist_ms = millis();
    return true;
  }
  return false;
}

/// @brief Flush the water pulse counter to NVS if the write throttle allows it.
/// @param now Current `millis()` value used for the throttle window.
static void maybe_persist_water_total(unsigned long now)
{
  if (!water_pulse_count_dirty)
    return;
  if (last_water_persist_ms != 0 && now - last_water_persist_ms < WATER_TOTAL_PERSIST_MS)
    return;
  persist_water_total();
}

/// @brief Set and immediately persist the water pulse counter.
/// @param liters New absolute water-meter total in liters.
/// @return True when the NVS write succeeds.
static bool set_water_total(uint32_t liters)
{
  uint32_t previous_water_pulse_count = water_pulse_count;
  uint32_t previous_last_water_pulse_count = last_water_pulse_count;
  bool previous_dirty = water_pulse_count_dirty;

  water_pulse_count = liters;
  last_water_pulse_count = liters;
  water_pulse_count_dirty = true;
  bool ok = persist_water_total();
  if (!ok)
  {
    water_pulse_count = previous_water_pulse_count;
    last_water_pulse_count = previous_last_water_pulse_count;
    water_pulse_count_dirty = previous_dirty;
    return false;
  }

  modbus_reset_water_flow_baseline();
  wlog_printf("Water total set: %u L", (unsigned)water_pulse_count);
  return ok;
}

/// @brief Parse an unsigned 32-bit decimal String.
/// @param s Input text; must contain only decimal digits.
/// @param out Parsed value on success.
/// @return True when the full input is a valid uint32_t.
static bool parse_u32_arg(const String &s, uint32_t &out)
{
  if (s.isEmpty())
    return false;
  uint64_t v = 0;
  for (size_t i = 0; i < s.length(); i++)
  {
    char c = s.charAt(i);
    if (c < '0' || c > '9')
      return false;
    v = v * 10u + (uint32_t)(c - '0');
    if (v > UINT32_MAX)
      return false;
  }
  out = (uint32_t)v;
  return true;
}

/// @brief Read all 8 PCF8574 digital inputs, update input_mask, log transitions.
/// @param count_water_edges When true, count Input 1 inactive-to-active edges
///                          as water-meter pulses. Pass false for the initial
///                          boot snapshot so an already-active input is not
///                          counted as a new liter.
/// @note Single Wire.requestFrom() gives an atomic snapshot and bypasses the
///       library's 10 ms latency cache, which silently returns LOW for all pins
///       when called again within the latency window (byteBuffered=0 + no I2C read
///       = all fall through to the default LOW value for pullDown-mode pins).
static void poll_inputs(bool count_water_edges = true)
{
  Wire.requestFrom((uint8_t)0x26, (uint8_t)1);
  if (!Wire.available())
    return; // I2C error — keep last known state
  uint8_t raw = Wire.read();
  uint8_t mask = ~raw; // active-LOW: invert so bit=1 means active
  for (int i = 0; i < 8; i++)
  {
    bool was = (input_mask >> i) & 1;
    bool active = (mask >> i) & 1;
    if (was != active)
      wlog_printf("Input %d -> %s", i + 1, active ? "ACTIVE" : "INACTIVE");
    // Input 1 = water meter: count one liter per inactive→active transition.
    if (count_water_edges && i == 0 && !was && active)
    {
      water_pulse_count++;
      water_pulse_count_dirty = true;
    }
  }
  input_mask = mask;
}

/// @brief GPIO12 ISR — fires on PCF8574 /INT falling edge; deferred to loop().
void IRAM_ATTR isr_pcf_input() { inputs_changed = true; }

/// @brief Trigger the HC-SR04 and convert the echo pulse to centimeters.
/// @return Measured distance in cm; 0.0 on echo timeout (~5 m range).
/// @note Blocking — pulseIn() can stall up to 30 ms while waiting for the echo.
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

/// @brief Append @p s to @p out with JSON string escaping applied.
/// @param out Destination buffer; escaped characters are appended in place.
/// @param s   NUL-terminated source string. Control characters below 0x20
///            are emitted as `\u00XX`; quote, backslash, newline, carriage
///            return, and tab use their short escape forms.
static void json_escape_into(String &out, const char *s)
{
  for (const char *p = s; *p; p++)
  {
    char c = *p;
    if (c == '"' || c == '\\')
    {
      out += '\\';
      out += c;
    }
    else if (c == '\n')
      out += "\\n";
    else if (c == '\r')
      out += "\\r";
    else if (c == '\t')
      out += "\\t";
    else if ((unsigned char)c < 0x20)
    {
      char buf[8];
      snprintf(buf, sizeof(buf), "\\u%04x", c);
      out += buf;
    }
    else
      out += c;
  }
}

/// @brief `GET /poll?since=<seq>` — state snapshot plus new log lines.
///
/// Returns a JSON object with the latest seq, relay mask, distance, uptime,
/// and any log lines with seq in `[since, current_seq)`. Seq numbers are
/// monotonic; if @c since is older than the ring buffer still retains it is
/// clamped forward to wlog_oldest_seq(), and the gap of overwritten lines
/// is lost — the client stays in sync via the returned `"seq"` field.
static void server_handle_poll()
{
  uint32_t since = (uint32_t)server.arg("since").toInt();
  uint32_t current_seq = wlog_seq();
  uint32_t oldest_seq = wlog_oldest_seq();

  // Clamp the replay window to what the ring buffer actually still holds.
  uint32_t start_seq = (since < oldest_seq) ? oldest_seq : since;
  if (start_seq > current_seq)
    start_seq = current_seq;

  String json;
  json.reserve(1024);
  json += "{\"seq\":";
  json += current_seq;
  json += ",\"relays\":";
  json += relay_mask;
  json += ",\"distance_cm\":";
  json += String(cached_distance_cm, 1);
  json += ",\"uptime_ms\":";
  json += millis();
  json += ",\"inputs\":";
  json += input_mask;
  json += ",\"water_pulses\":";
  json += water_pulse_count;
  json += ",\"version\":\"";
  json_escape_into(json, util_version_string());
  json += "\",\"lines\":[";
  bool first = true;
  for (uint32_t s = start_seq; s < current_seq; s++)
  {
    if (!first)
      json += ',';
    first = false;
    json += '"';
    json_escape_into(json, wlog_line_at(s));
    json += '"';
  }
  json += "]}";
  server.send(200, "application/json", json);
}

/// @brief `GET /water/total/set?value=<liters>` — set the persisted water total.
///
/// The value is an absolute uint32 liter count. The endpoint intentionally has
/// no authentication; it updates RAM, writes NVS immediately, resets flow-rate
/// baselines, logs the change, and returns `{"water_pulses":..,"ok":true}`.
static void server_handle_water_total_set()
{
  uint32_t liters = 0;
  if (!parse_u32_arg(server.arg("value"), liters))
  {
    server.send(400, "application/json", "{\"error\":\"value must be 0..4294967295\"}");
    return;
  }

  if (!set_water_total(liters))
  {
    server.send(500, "application/json", "{\"error\":\"NVS write failed\"}");
    return;
  }

  String json;
  json.reserve(48);
  json += "{\"water_pulses\":";
  json += water_pulse_count;
  json += ",\"ok\":true}";
  server.send(200, "application/json", json);
}

/// @brief `GET /sw?n=<1..8>&on=<0|1>` — set one valve, return new state.
///
/// Responds with `{"n":..,"on":..,"relays":..}` on success, or HTTP 400 with
/// `{"error":"invalid n"}` if @c n is out of range.
static void server_handle_sw()
{
  int n = server.arg("n").toInt();
  int on = server.arg("on").toInt();
  if (n < 1 || n > 8)
  {
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

/// @brief `GET /closeall` — close every valve and return the new relay mask.
static void server_handle_closeall()
{
  for (int i = 1; i <= 8; i++)
    set_valve(i, false);
  String j;
  j.reserve(24);
  j += "{\"relays\":";
  j += relay_mask;
  j += "}";
  server.send(200, "application/json", j);
}

/// @brief `GET /SW?LED=onN|offN` — legacy single-valve endpoint.
/// @note Kept for backwards compatibility with the original demo UI; new
///       clients should use /sw with `n` and `on` query parameters.
static void server_handle_sw_legacy()
{
  String state = server.arg("LED");
  for (int i = 0; i < 8; ++i)
  {
    if (state == "on" + String(i + 1))
      set_valve(i + 1, true);
    else if (state == "off" + String(i + 1))
      set_valve(i + 1, false);
  }
  server.send(200, "text/plain", "OK");
}

// UI assets are held under web/ and embedded into the firmware via
// platformio.ini's board_build.embed_txtfiles. The IDF build system exposes
// _binary_<path>_start / _end symbols (slashes and dots become underscores).
extern const uint8_t INDEX_HTML_START[] asm("_binary_web_index_html_start");
extern const uint8_t INDEX_HTML_END[] asm("_binary_web_index_html_end");
extern const uint8_t CONFIG_JSON_START[] asm("_binary_web_config_json_start");
extern const uint8_t CONFIG_JSON_END[] asm("_binary_web_config_json_end");

/// @brief Send an embedded flash blob as an HTTP response body.
/// @param start        Pointer to the blob's `_binary_..._start` symbol.
/// @param end          Pointer to the blob's `_binary_..._end` symbol.
/// @param content_type MIME type for the response.
/// @note `embed_txtfiles` appends a NUL terminator to text blobs; this helper
///       strips it so it isn't sent on the wire.
static void send_embedded(const uint8_t *start, const uint8_t *end, const char *content_type)
{
  size_t len = (end - start);
  if (len > 0 && start[len - 1] == 0)
    len -= 1;
  server.send_P(200, content_type, (PGM_P)start, len);
}

/// @brief `GET /` — serve the web UI HTML from the flash blob.
static void server_handle_root()
{
  send_embedded(INDEX_HTML_START, INDEX_HTML_END, "text/html");
}

/// @brief `GET /config.json` — serve UI configuration (title, valve labels).
/// @note The file is embedded at build time from `web/config.json`; the
///       firmware does not parse it — the browser does.
static void server_handle_config()
{
  send_embedded(CONFIG_JSON_START, CONFIG_JSON_END, "application/json");
}

/// @brief `GET /history.json` — multi-signal min/avg/max history.
///
/// Returns `{"distance":{"short":...,"long":...},"water":{"short":...,"long":...}}`.
/// Each ring is `{"period_sec":N,"buckets":[{"t":..,"min":..,"avg":..,"max":..,"n":..}, ...]}`;
/// empty slots collapse to `{"t":..,"n":0}`. Water values are L/min sampled
/// from the Input 1 pulse counter once per minute. Persisted to NVS, so the
/// series survives reboots.
static void server_handle_history()
{
  String json;
  json.reserve(24 * 1024);
  json += "{\"distance\":";
  distance_history.serialize(json);
  json += ",\"water\":";
  water_history.serialize(json);
  json += '}';
  server.send(200, "application/json", json);
}

/// @brief `GET /history/clear?signal=distance|water&ring=short|long` — erase
///        one history ring, in RAM and NVS, on user demand.
/// @note Two-step confirmation is enforced client-side in the UI; this
///       endpoint performs the destructive action unconditionally and is
///       not idempotent. `signal` defaults to "distance" for back-compat
///       with the original UI which only had the tank-distance clear buttons.
///       Returns 400 if `ring` or `signal` is missing or unknown.
static void server_handle_history_clear()
{
  String signal = server.arg("signal");
  if (signal.isEmpty())
    signal = "distance";
  String ring = server.arg("ring");

  History *h = nullptr;
  const char *signal_label = nullptr;
  if (signal == "distance")
  {
    h = &distance_history;
    signal_label = "distance";
  }
  else if (signal == "water")
  {
    h = &water_history;
    signal_label = "water";
  }
  else
  {
    server.send(400, "application/json", "{\"error\":\"signal must be distance or water\"}");
    return;
  }

  if (ring == "short")
  {
    h->clear_short();
    wlog_printf("History %s 24h ring cleared (RAM+NVS)", signal_label);
    server.send(200, "application/json", "{\"cleared\":\"short\",\"ok\":true}");
  }
  else if (ring == "long")
  {
    h->clear_long();
    wlog_printf("History %s 30d ring cleared (RAM+NVS)", signal_label);
    server.send(200, "application/json", "{\"cleared\":\"long\",\"ok\":true}");
  }
  else
  {
    server.send(400, "application/json", "{\"error\":\"ring must be short or long\"}");
  }
}

/// @brief Arduino entry point: initialize GPIO, Ethernet, HTTP, and OTA.
void setup()
{
  pinMode(ULTRASOUND_TRIGER_PIN, OUTPUT);
  pinMode(ULTRASOUND_ECHO_PIN, INPUT);

  Serial.begin(115200);
  delay(1000);

  wlog_println("Starting Ethernet...");
  ETH.begin(ETH_TYPE, ETH_ADDR, ETH_MDC_PIN, ETH_MDIO_PIN, ETH_POWER_PIN, ETH_CLK_MODE);
  if (ETH.config(local_ip, gateway, subnet, dns, dns) == false)
  {
    wlog_println("LAN8720 configuration failed.");
  }
  else
  {
    wlog_println("LAN8720 configuration success.");
  }

  delay(1000);
  wlog_printf("ETH IP: %s", ETH.localIP().toString().c_str());

  util_init_time();
  wlog_printf("Firmware: %s", util_version_string());

  distance_history.begin();
  water_history.begin();
  load_water_total();

  // xreef/PCF8574 queues pinMode() calls and applies them inside begin();
  // pinMode-before-begin is the library's required order, not the usual Arduino pattern.
  for (int i = 0; i < 8; i++)
    pcf8574_re.pinMode(i, OUTPUT);
  pcf8574_re.begin();
  for (int i = 0; i < 8; i++)
    pcf8574_re.digitalWrite(i, 1); // all valves CLOSED
  relay_mask = 0;

  for (int i = 0; i < 8; i++)
    pcf8574_in.pinMode(i, INPUT);
  pcf8574_in.begin();
  poll_inputs(false); // capture initial state; also deasserts any pending /INT before we attach
  pinMode(INPUT_INT_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(INPUT_INT_PIN), isr_pcf_input, FALLING);

  // OLED at 0x3C — shares the Wire bus already initialized by the PCF8574s.
  oled_init();

  modbus_init();

  server.on("/", server_handle_root);
  server.on("/config.json", server_handle_config);
  server.on("/history.json", server_handle_history);
  server.on("/history/clear", server_handle_history_clear);
  server.on("/water/total/set", server_handle_water_total_set);
  server.on("/sw", server_handle_sw);
  server.on("/closeall", server_handle_closeall);
  server.on("/poll", server_handle_poll);
  server.on("/SW", server_handle_sw_legacy); // back-compat
  server.begin();
  wlog_println("Web server started on :80");

  ArduinoOTA.setHostname("retention-tank");
  // ArduinoOTA.setPassword("change-me"); // uncomment and set in upload_flags --auth=
  ArduinoOTA.onStart([]()
                     {
                       persist_water_total();
                       wlog_println("OTA: start"); });
  ArduinoOTA.onEnd([]()
                   { wlog_println("OTA: end"); });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total)
                        {
    static int last_pct = -10;
    int pct = total ? (int)((progress * 100u) / total) : 0;
    if (pct >= last_pct + 10) { wlog_printf("OTA: %d%%", pct); last_pct = pct; } });
  ArduinoOTA.onError([](ota_error_t error)
                     { wlog_printf("OTA error: %u", (unsigned)error); });
  ArduinoOTA.begin();
  wlog_println("OTA ready");
}

/// @brief Arduino main loop: pump OTA, HTTP, and refresh the cached distance.
void loop()
{
  ArduinoOTA.handle();
  server.handleClient();
  modbus_poll();

  unsigned long now = millis();
  if (now - last_distance_ms >= DISTANCE_REFRESH_MS)
  {
    cached_distance_cm = read_distance_cm();
    last_distance_ms = now;
    wlog_printf("Distance: %.1f cm", cached_distance_cm);
    if (cached_distance_cm > 0.0f)
    {
      distance_history.record(time(nullptr), cached_distance_cm);
    }
  }
  if (inputs_changed)
  {
    inputs_changed = false;
    poll_inputs();
  }
  maybe_persist_water_total(now);
  if (now - last_water_sample_ms >= WATER_SAMPLE_MS)
  {
    last_water_sample_ms = now;
    uint32_t delta = water_pulse_count - last_water_pulse_count;
    last_water_pulse_count = water_pulse_count;
    // One pulse = one liter; window is 60 s, so delta == L/min as a value.
    water_history.record(time(nullptr), (float)delta);
  }
  if (now - last_oled_ms >= OLED_REFRESH_MS)
  {
    last_oled_ms = now;
    String ip = ETH.localIP().toString();
    OledSnapshot snap{
        cached_distance_cm,
        relay_mask,
        input_mask,
        now,
        water_pulse_count,
        ip.c_str(),
    };
    oled_render(snap);
  }
}
