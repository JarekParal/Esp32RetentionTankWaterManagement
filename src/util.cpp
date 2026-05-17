#include "util.h"

#include <time.h>
#include <sys/time.h>

#ifndef GIT_HASH
#define GIT_HASH "unknown"
#endif
#ifndef BUILD_DATE
#define BUILD_DATE "unknown"
#endif

/// POSIX TZ string for the device's location. Default is Europe/Prague
/// (CET/CEST with EU DST rules). Change here if the device runs elsewhere.
static const char *const TZ_POSIX = "CET-1CEST,M3.5.0,M10.5.0/3";

/// SNTP servers tried in order. pool.ntp.org rotates between volunteer
/// servers worldwide; the second entry is a stable fallback.
static const char *const NTP_SERVER_1 = "pool.ntp.org";
static const char *const NTP_SERVER_2 = "time.google.com";

/// Epoch threshold (2024-01-01 UTC) used to decide whether SNTP has produced
/// a real time. ESP32 boots with the RTC at 1970-01-01.
static constexpr time_t TIME_SYNCED_THRESHOLD = 1704067200;

void util_init_time()
{
  configTime(0, 0, NTP_SERVER_1, NTP_SERVER_2);
  setenv("TZ", TZ_POSIX, 1);
  tzset();
}

void util_format_timestamp(char *buf, size_t bufsize)
{
  if (bufsize == 0)
    return;

  time_t now = time(nullptr);
  if (now > TIME_SYNCED_THRESHOLD)
  {
    struct tm tm_local;
    localtime_r(&now, &tm_local);
    strftime(buf, bufsize, "%Y-%m-%d %H:%M:%S", &tm_local);
  }
  else
  {
    snprintf(buf, bufsize, "boot+%lus", (unsigned long)(millis() / 1000));
  }
}

const char *util_version_string()
{
  static char buf[80];
  if (buf[0] == '\0')
  {
    snprintf(buf, sizeof(buf), "v%s (%s, %s)", FIRMWARE_VERSION, GIT_HASH, BUILD_DATE);
  }
  return buf;
}
