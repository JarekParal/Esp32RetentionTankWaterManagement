#pragma once

#include <Arduino.h>

/// @file log_buffer.h
/// @brief Bounded ring buffer of short log lines, mirrored to Serial.
///
/// Each call to wlog_println() or wlog_printf() appends one line to the
/// buffer and prints it on Serial. Lines are tagged with a monotonically
/// increasing sequence number so HTTP clients can ask for "everything new
/// since seq N" without requiring per-client server-side state.
///
/// Only the most recent N lines are retained (see LOG_BUF_LINES in the
/// implementation); older lines are overwritten in place. Sequence numbers
/// themselves never recycle.
///
/// All functions are intended for the Arduino main loop. They are NOT safe
/// to call from interrupt context — neither Serial.println() nor the buffer
/// update is protected against concurrent access.

/// @brief Append a log line and mirror it to Serial.
/// @param msg NUL-terminated string. Lines longer than the internal line
///            cap are truncated; the NUL terminator is always written.
void wlog_println(const char *msg);

/// @brief Arduino String overload of wlog_println(const char *).
/// @param msg String to log; forwarded via String::c_str().
void wlog_println(const String &msg);

/// @brief printf-style logging into the ring buffer.
/// @param fmt printf format string, followed by its arguments. The
///            formatted output is truncated to the internal line cap.
void wlog_printf(const char *fmt, ...);

/// @brief Total number of lines ever written.
/// @return The sequence number that the next written line will receive.
///         Equivalently, one past the highest seq currently in the buffer.
///         Monotonic; never recycles.
uint32_t wlog_seq();

/// @brief Lowest sequence number still resident in the ring buffer.
/// @return `wlog_seq() - capacity` once the buffer has wrapped, otherwise 0.
///         Lines with seq < this value have been overwritten and are gone.
uint32_t wlog_oldest_seq();

/// @brief Read a buffered line by its absolute sequence number.
/// @param s Sequence number; the caller must ensure
///          `wlog_oldest_seq() <= s < wlog_seq()`. Out-of-range values
///          return data from a wrapped slot (undefined which line).
/// @return Pointer into the ring buffer storage. The contents remain valid
///         only until the next wlog_println() / wlog_printf() call, which
///         may overwrite the slot if `s == wlog_oldest_seq()`.
const char *wlog_line_at(uint32_t s);
