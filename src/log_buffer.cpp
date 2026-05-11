#include "log_buffer.h"

#include <stdarg.h>
#include <string.h>

/// Maximum length of a single log line in bytes, including the NUL
/// terminator. Lines longer than this are truncated by wlog_println().
constexpr size_t LOG_LINE_LEN = 96;

/// Number of slots in the ring buffer. Once full, the oldest line is
/// overwritten by the next write. Sized to fit a few screens of log output
/// while staying small enough that a /poll response holds easily in a
/// single TCP segment.
constexpr size_t LOG_BUF_LINES = 64;

/// Backing storage. The line at sequence number `s` lives in
/// `log_buf[s % LOG_BUF_LINES]` while `s >= wlog_oldest_seq()`.
static char log_buf[LOG_BUF_LINES][LOG_LINE_LEN];

/// Total number of lines ever written. Equivalently, the seq the next
/// write will use. Monotonic; never resets.
static uint32_t g_log_seq = 0;

void wlog_println(const char *msg)
{
  Serial.println(msg);
  size_t len = strlen(msg);
  if (len >= LOG_LINE_LEN) len = LOG_LINE_LEN - 1;
  size_t slot = g_log_seq % LOG_BUF_LINES;
  memcpy(log_buf[slot], msg, len);
  log_buf[slot][len] = '\0';
  g_log_seq++;
}

void wlog_println(const String &msg) { wlog_println(msg.c_str()); }

void wlog_printf(const char *fmt, ...)
{
  char tmp[LOG_LINE_LEN];
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(tmp, sizeof(tmp), fmt, ap);
  va_end(ap);
  wlog_println(tmp);
}

uint32_t wlog_seq() { return g_log_seq; }

uint32_t wlog_oldest_seq()
{
  return (g_log_seq > LOG_BUF_LINES) ? (g_log_seq - LOG_BUF_LINES) : 0;
}

const char *wlog_line_at(uint32_t s)
{
  return log_buf[s % LOG_BUF_LINES];
}
