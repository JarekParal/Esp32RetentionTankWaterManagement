#pragma once

#include <Arduino.h>

/// @file oled.h
/// @brief Simplified mirror of the Web UI rendered on a 128x64 SSD1306 OLED.
///
/// Wired to the existing I2C bus (SDA=GPIO4, SCL=GPIO5) at address 0x3C,
/// sharing the bus with the PCF8574 expanders at 0x24 (relays) and 0x26
/// (inputs). The panel has no reset pin; pass U8X8_PIN_NONE.
///
/// Layout (top to bottom):
///   - Title + IP / uptime  (small font)
///   - Tank distance + fill % (larger font, headline)
///   - Valve mask + input mask (one char per channel)
///   - Most recent log line, prefixed with hh:mm:ss
///
/// Fill % uses fixed thresholds (OLED_TANK_EMPTY_CM / OLED_TANK_FULL_CM in
/// oled.cpp) that mirror web/config.json tank.empty_distance_cm /
/// full_distance_cm. Keep them in sync by hand — the firmware does not
/// parse config.json.

/// State snapshot consumed by oled_render(). Fields are copies of the
/// authoritative values in main.cpp; the caller assembles this just before
/// each call and discards it afterwards.
struct OledSnapshot
{
  float distance_cm;  ///< Last cached HC-SR04 reading; 0 if no echo.
  uint8_t relay_mask; ///< Bit i = valve (i+1) OPEN.
  uint8_t input_mask; ///< Bit i = input  (i+1) ACTIVE.
  unsigned long uptime_ms;
  uint32_t water_pulse_count; ///< Lifetime water meter pulses; 1 pulse = 1 L.
  const char *ip_str;         ///< Borrowed; must outlive the oled_render() call.
};

/// @brief Initialize Wire (if not already) and the SSD1306. Safe to call
///        after Wire.begin() has run elsewhere — U8g2 will not reset the bus.
void oled_init();

/// @brief Render one frame to the OLED from @p s. Blocking; ~100 ms over
///        I2C at 100 kHz for a full 1024-byte frame transfer.
void oled_render(const OledSnapshot &s);
