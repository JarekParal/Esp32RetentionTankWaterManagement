#include "modbus_srv.h"

#include <Arduino.h>
#include <ModbusIP_ESP8266.h>

#include "history.h"
#include "log_buffer.h"

// State + actions live in main.cpp; reach them via extern. See
// doc/MODBUS.md for the register map this file implements.
extern uint8_t relay_mask;
extern uint8_t input_mask;
extern float cached_distance_cm;
extern float cached_current_a;
extern float cached_power_w;
extern uint32_t water_pulse_count;
extern History distance_history;
extern History water_history;
extern History current_history;
void set_valve(int n, bool open);

// Tank fill thresholds mirror oled.cpp / web/config.json. Kept duplicated by
// intent so Modbus clients don't need to know how the % is computed.
constexpr int MB_TANK_EMPTY_CM = 132;
constexpr int MB_TANK_FULL_CM = 20;

// Register addresses — see doc/MODBUS.md for the canonical table.
namespace mb_addr
{
constexpr uint16_t COIL_VALVE_BASE = 0; // 0..7 = valve 1..8
constexpr uint16_t COIL_CLOSE_ALL = 8;  // write-1 trigger
constexpr uint16_t COIL_CLEAR_WATER_SHORT = 16;
constexpr uint16_t COIL_CLEAR_WATER_LONG = 17;
constexpr uint16_t COIL_CLEAR_DIST_SHORT = 18;
constexpr uint16_t COIL_CLEAR_DIST_LONG = 19;
constexpr uint16_t COIL_CLEAR_CURRENT_SHORT = 20;
constexpr uint16_t COIL_CLEAR_CURRENT_LONG = 21;

constexpr uint16_t IST_INPUT_BASE = 0; // 0..7 = input 1..8

constexpr uint16_t HREG_VALVE_BASE = 0; // 0..7 = valve 1..8, 0 closed / 1 open

constexpr uint16_t IREG_DISTANCE_DM = 0;     // distance_cm × 10
constexpr uint16_t IREG_FULLNESS_PCT = 1;    // 0..100
constexpr uint16_t IREG_WATER_PULSES_HI = 2; // 32-bit total, high word first
constexpr uint16_t IREG_WATER_PULSES_LO = 3;
constexpr uint16_t IREG_WATER_FLOW_DLPM = 4; // last-minute pulses × 10 (L/min × 10)
constexpr uint16_t IREG_WATER_24H_HI = 5;    // 32-bit liters in last 24 h
constexpr uint16_t IREG_WATER_24H_LO = 6;
constexpr uint16_t IREG_CURRENT_MA = 7;   // RMS current in milliamperes
constexpr uint16_t IREG_POWER_W = 8;      // estimated active power in watts
constexpr uint16_t IREG_UPTIME_S_HI = 10; // 32-bit seconds since boot
constexpr uint16_t IREG_UPTIME_S_LO = 11;
} // namespace mb_addr

static ModbusIP mb;
// Last sampled flow rate, kept in modbus_srv.cpp so the ireg getter can read
// it without poking at sampler internals in main.cpp. Updated in modbus_poll().
static uint32_t last_seen_water_pulses = 0;
static unsigned long last_flow_sample_ms = 0;
static uint16_t cached_flow_dlpm = 0; // dL/min (i.e. L/min × 10)
constexpr unsigned long FLOW_SAMPLE_MS = 60000;

static int fill_percent(float dist_cm)
{
  if (dist_cm <= 0.0f)
    return 0;
  float span = float(MB_TANK_EMPTY_CM - MB_TANK_FULL_CM);
  float ratio = (float(MB_TANK_EMPTY_CM) - dist_cm) / span;
  int p = (int)(ratio * 100.0f + 0.5f);
  if (p < 0)
    return 0;
  if (p > 100)
    return 100;
  return p;
}

void modbus_init()
{
  mb.server();
  modbus_reset_water_flow_baseline();

  // ----- Coils 0..7: valve state, read + write -----
  for (int i = 0; i < 8; i++)
  {
    const uint16_t addr = mb_addr::COIL_VALVE_BASE + i;
    mb.addCoil(addr);
    mb.onGetCoil(addr, [i](TRegister *, uint16_t) -> uint16_t
                 { return (relay_mask & (1u << i)) ? 1 : 0; });
    mb.onSetCoil(addr, [i](TRegister *, uint16_t val) -> uint16_t
                 {
      set_valve(i + 1, val != 0);
      return val ? 1 : 0; });
  }

  // ----- Holding registers 0..7: one read/write register per valve -----
  for (int i = 0; i < 8; i++)
  {
    const uint16_t addr = mb_addr::HREG_VALVE_BASE + i;
    mb.addHreg(addr);
    mb.onGetHreg(addr, [i](TRegister *, uint16_t) -> uint16_t
                 { return (relay_mask & (1u << i)) ? 1 : 0; });
    mb.onSetHreg(addr, [i](TRegister *, uint16_t val) -> uint16_t
                 {
      set_valve(i + 1, val != 0);
      return val ? 1 : 0; });
  }

  // ----- Coil 8: close-all trigger (write-1, self-clears) -----
  mb.addCoil(mb_addr::COIL_CLOSE_ALL);
  mb.onSetCoil(mb_addr::COIL_CLOSE_ALL, [](TRegister *, uint16_t val) -> uint16_t
               {
    if (val)
    {
      for (int n = 1; n <= 8; n++) set_valve(n, false);
      wlog_println("Modbus: close-all triggered");
    }
    return 0; });

  // ----- Coils 16..21: history-clear triggers -----
  struct ClearWiring
  {
    uint16_t addr;
    const char *desc;
    void (*action)();
  };
  static const ClearWiring clears[] = {
      {mb_addr::COIL_CLEAR_WATER_SHORT, "water 24h", []()
       { water_history.clear_short(); }},
      {mb_addr::COIL_CLEAR_WATER_LONG, "water 30d", []()
       { water_history.clear_long(); }},
      {mb_addr::COIL_CLEAR_DIST_SHORT, "distance 24h", []()
       { distance_history.clear_short(); }},
      {mb_addr::COIL_CLEAR_DIST_LONG, "distance 30d", []()
       { distance_history.clear_long(); }},
      {mb_addr::COIL_CLEAR_CURRENT_SHORT, "current 24h", []()
       { current_history.clear_short(); }},
      {mb_addr::COIL_CLEAR_CURRENT_LONG, "current 30d", []()
       { current_history.clear_long(); }},
  };
  for (const auto &w : clears)
  {
    mb.addCoil(w.addr);
    const char *desc = w.desc;
    auto action = w.action;
    mb.onSetCoil(w.addr, [desc, action](TRegister *, uint16_t val) -> uint16_t
                 {
      if (val)
      {
        action();
        wlog_printf("Modbus: cleared %s history (RAM+NVS)", desc);
      }
      return 0; });
  }

  // ----- Discrete inputs 0..7: digital inputs (read-only) -----
  for (int i = 0; i < 8; i++)
  {
    const uint16_t addr = mb_addr::IST_INPUT_BASE + i;
    mb.addIsts(addr);
    mb.onGetIsts(addr, [i](TRegister *, uint16_t) -> uint16_t
                 { return (input_mask & (1u << i)) ? 1 : 0; });
  }

  // ----- Input registers: live measurements (read-only) -----
  // Each ireg is added at init; the onGetIreg callback returns the current value.
  auto add_ireg_ro = [](uint16_t addr, uint16_t (*get)())
  {
    mb.addIreg(addr);
    mb.onGetIreg(addr, [get](TRegister *, uint16_t) -> uint16_t
                 { return get(); });
  };

  add_ireg_ro(mb_addr::IREG_DISTANCE_DM, []() -> uint16_t
              {
    if (!isfinite(cached_distance_cm) || cached_distance_cm < 0) return 0;
    long v = lroundf(cached_distance_cm * 10.0f);
    if (v < 0) v = 0;
    if (v > 65535) v = 65535;
    return (uint16_t)v; });
  add_ireg_ro(mb_addr::IREG_FULLNESS_PCT, []() -> uint16_t
              { return (uint16_t)fill_percent(cached_distance_cm); });
  add_ireg_ro(mb_addr::IREG_WATER_PULSES_HI, []() -> uint16_t
              { return (uint16_t)(water_pulse_count >> 16); });
  add_ireg_ro(mb_addr::IREG_WATER_PULSES_LO, []() -> uint16_t
              { return (uint16_t)(water_pulse_count & 0xFFFF); });
  add_ireg_ro(mb_addr::IREG_WATER_FLOW_DLPM, []() -> uint16_t
              { return cached_flow_dlpm; });
  add_ireg_ro(mb_addr::IREG_WATER_24H_HI, []() -> uint16_t
              {
    uint32_t l = (uint32_t)lroundf(water_history.short_window_sum());
    return (uint16_t)(l >> 16); });
  add_ireg_ro(mb_addr::IREG_WATER_24H_LO, []() -> uint16_t
              {
    uint32_t l = (uint32_t)lroundf(water_history.short_window_sum());
    return (uint16_t)(l & 0xFFFF); });
  add_ireg_ro(mb_addr::IREG_CURRENT_MA, []() -> uint16_t
              {
    if (!isfinite(cached_current_a) || cached_current_a < 0.0f) return 0;
    long milliamps = lroundf(cached_current_a * 1000.0f);
    if (milliamps > 65535) milliamps = 65535;
    return (uint16_t)milliamps; });
  add_ireg_ro(mb_addr::IREG_POWER_W, []() -> uint16_t
              {
    if (!isfinite(cached_power_w) || cached_power_w < 0.0f) return 0;
    long watts = lroundf(cached_power_w);
    if (watts > 65535) watts = 65535;
    return (uint16_t)watts; });
  add_ireg_ro(mb_addr::IREG_UPTIME_S_HI, []() -> uint16_t
              { return (uint16_t)((millis() / 1000UL) >> 16); });
  add_ireg_ro(mb_addr::IREG_UPTIME_S_LO, []() -> uint16_t
              { return (uint16_t)((millis() / 1000UL) & 0xFFFF); });

  wlog_println("Modbus TCP listening on :502");
}

void modbus_reset_water_flow_baseline()
{
  last_seen_water_pulses = water_pulse_count;
  cached_flow_dlpm = 0;
}

void modbus_poll()
{
  mb.task();

  // Track the flow rate ourselves so the ireg can return it without waking the
  // sampler in main.cpp. Same 1-minute window the UI/history sampler uses.
  unsigned long now = millis();
  if (now - last_flow_sample_ms >= FLOW_SAMPLE_MS)
  {
    last_flow_sample_ms = now;
    uint32_t delta = water_pulse_count - last_seen_water_pulses;
    last_seen_water_pulses = water_pulse_count;
    // delta pulses per minute → dL/min (×10) so we keep one decimal.
    uint32_t dlpm = delta * 10UL;
    cached_flow_dlpm = (dlpm > 0xFFFF) ? 0xFFFF : (uint16_t)dlpm;
  }
}
