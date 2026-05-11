#pragma once

#include <Arduino.h>

/// @file util.h
/// @brief Small grab-bag: firmware version string and wall-clock helpers.

/// Manually bumped on each release. Combined with the git hash and build
/// time (both injected via scripts/inject_build_info.py) to form the full
/// version string returned by util_version_string().
constexpr const char *FIRMWARE_VERSION = "0.1.0";

/// @brief Configure SNTP and the local timezone.
///
/// Call once after the network has an IP. SNTP sync happens asynchronously;
/// util_format_timestamp() falls back to a boot-relative format until the
/// first sync arrives.
void util_init_time();

/// @brief Write the current wall-clock timestamp into @p buf.
/// @param buf     Destination buffer; must be at least 20 bytes for the
///                synced format `YYYY-MM-DD HH:MM:SS`.
/// @param bufsize Size of @p buf in bytes; output is NUL-terminated.
///
/// If SNTP has not yet produced a real time, falls back to `boot+<seconds>s`.
void util_format_timestamp(char *buf, size_t bufsize);

/// @brief Composed firmware identity: `vX.Y.Z (gitHash, YYYY-MM-DD HH:MM:SS)`.
/// @return Pointer to a process-lifetime static buffer; safe to log or send
///         in HTTP responses.
const char *util_version_string();
