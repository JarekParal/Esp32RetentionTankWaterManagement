#include "oled.h"

#include <U8g2lib.h>
#include <Wire.h>

#include "log_buffer.h"

// Tank fill thresholds — mirror web/config.json's tank.empty_distance_cm /
// tank.full_distance_cm. Kept hardcoded on purpose: parsing config.json on
// the device would pull in a JSON library for two integers. If the values
// in config.json change, update them here too.
constexpr int OLED_TANK_EMPTY_CM = 140;
constexpr int OLED_TANK_FULL_CM = 20;

// Full-framebuffer U8g2 driver for SSD1306 128x64 over hardware I2C.
// Reset pin not wired on the LaskaKit module → U8X8_PIN_NONE.
static U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);

void oled_init()
{
  u8g2.begin();
  u8g2.setFontPosTop(); // y coordinate addresses the TOP of each glyph row
}

/// Map a raw HC-SR04 distance to a 0..100 fill percentage using the
/// hardcoded thresholds. Returns 0 for invalid (zero / negative) readings.
static int fill_percent(float dist_cm)
{
  if (dist_cm <= 0.0f)
    return 0;
  float span = float(OLED_TANK_EMPTY_CM - OLED_TANK_FULL_CM);
  float ratio = (float(OLED_TANK_EMPTY_CM) - dist_cm) / span;
  int p = (int)(ratio * 100.0f + 0.5f);
  if (p < 0)
    return 0;
  if (p > 100)
    return 100;
  return p;
}

/// Format uptime as "Nd HH:MM" — tight enough to share a row with the IP.
static void format_uptime(char *buf, size_t bufsize, unsigned long uptime_ms)
{
  unsigned long s = uptime_ms / 1000;
  unsigned int days = (unsigned int)(s / 86400);
  s %= 86400;
  unsigned int hours = (unsigned int)(s / 3600);
  s %= 3600;
  unsigned int mins = (unsigned int)(s / 60);
  snprintf(buf, bufsize, "%ud %02u:%02u", days, hours, mins);
}

/// Render 8 channels as one character each: digit (1..8) if the bit is set,
/// '.' otherwise. Always exactly 8 chars, NUL-terminated.
static void format_mask(char *buf, size_t bufsize, uint8_t mask)
{
  if (bufsize < 9)
  {
    if (bufsize > 0)
      buf[0] = '\0';
    return;
  }
  for (int i = 0; i < 8; i++)
  {
    buf[i] = (mask & (1u << i)) ? char('1' + i) : '.';
  }
  buf[8] = '\0';
}

/// Skip the "YYYY-MM-DD " date prefix that util_format_timestamp() prepends
/// to every log line once SNTP has synced; returns the input unchanged when
/// the prefix isn't there (the "boot+Xs" pre-sync form).
static const char *strip_date_prefix(const char *line)
{
  if (!line)
    return "";
  // Synced: "2026-05-17 14:22:31 ..."  -> skip first 11 chars to land on HH:MM:SS
  if (line[0] == '2' && line[4] == '-' && line[7] == '-' && line[10] == ' ')
  {
    return line + 11;
  }
  return line;
}

void oled_render(const OledSnapshot &s)
{
  u8g2.clearBuffer();

  // Row 1: IP + uptime (small). Replaces the old "Retention Tank" title row;
  // adding the water meter pushed everything up by one row.
  u8g2.setFont(u8g2_font_6x10_tf);
  char up[16];
  format_uptime(up, sizeof(up), s.uptime_ms);
  u8g2.setCursor(0, 0);
  u8g2.print(s.ip_str ? s.ip_str : "(no IP)");
  int up_w = u8g2.getStrWidth(up);
  u8g2.setCursor(128 - up_w, 0);
  u8g2.print(up);

  // Row 2-3: tank (headline). Bigger font.
  u8g2.setFont(u8g2_font_7x14B_tf);
  char tank[24];
  snprintf(tank, sizeof(tank), "%.1f cm  %d%%", (double)s.distance_cm, fill_percent(s.distance_cm));
  u8g2.setCursor(0, 12);
  u8g2.print(tank);

  // Row 4: water meter total since boot (small).
  u8g2.setFont(u8g2_font_6x10_tf);
  char water[16];
  snprintf(water, sizeof(water), "Water: %u L", (unsigned)s.water_pulse_count);
  u8g2.setCursor(0, 30);
  u8g2.print(water);

  // Row 5: valve + input masks on one line, small font.
  char vmask[16], imask[16];
  format_mask(vmask, sizeof(vmask), s.relay_mask);
  format_mask(imask, sizeof(imask), s.input_mask);
  char masks[32];
  snprintf(masks, sizeof(masks), "V:%s I:%s", vmask, imask);
  u8g2.setCursor(0, 42);
  u8g2.print(masks);

  // Row 6: most recent log line, prefixed with hh:mm:ss embedded in the line.
  u8g2.setFont(u8g2_font_5x8_tf);
  uint32_t seq = wlog_seq();
  if (seq > 0)
  {
    const char *raw = wlog_line_at(seq - 1);
    const char *line = strip_date_prefix(raw);
    // 5x8 font => 25 chars per 128 px row; cap at 25 + NUL.
    char trimmed[26];
    strncpy(trimmed, line, sizeof(trimmed) - 1);
    trimmed[sizeof(trimmed) - 1] = '\0';
    u8g2.setCursor(0, 55);
    u8g2.print(trimmed);
  }

  u8g2.sendBuffer();
}
