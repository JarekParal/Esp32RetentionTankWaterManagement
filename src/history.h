#pragma once

#include <Arduino.h>
#include <time.h>

/// @file history.h
/// @brief Persistent two-resolution time-series history for one signal.
///
/// A History instance owns two ring buffers: a high-resolution "short" ring
/// for recent samples and a low-resolution "long" ring for older samples.
/// Each bucket accumulates min, max, and a running sum/count over its
/// period. Slots are addressed by `t_unit % slot_count`, so when a new
/// period rolls past the far end of the buffer the older period in that
/// slot is silently overwritten.
///
/// On every bucket rollover the affected ring is flushed to NVS via the
/// Arduino Preferences API, so the series survives reboots. The signal_id
/// is used as the NVS namespace key — it must be unique across History
/// instances and ≤14 chars (ESP32 NVS namespace limit).
///
/// Typical use:
/// @code
///   History tank_level(
///     "dist",
///     { /*period_sec=*/ 600,   /*slot_count=*/ 144 },  // 10 min × 144 = 24 h
///     { /*period_sec=*/ 86400, /*slot_count=*/ 30 });  // 1 day  × 30  = 30 d
///
///   void setup()  { tank_level.begin(); }
///   void loop()   { tank_level.record(time(nullptr), distance_cm); }
/// @endcode
class History
{
public:
  /// Bucket period, slot count, and an optional NVS write-throttle for one
  /// resolution level.
  ///
  /// `min_persist_sec` caps how often the ring may flush to NVS — leave at
  /// 0 (the default) to persist on every bucket rollover, or set to a
  /// positive value to skip rollovers that fall within that window of the
  /// last successful write. The completed bucket stays in RAM either way;
  /// the trade-off is purely between NVS wear and how much recent state a
  /// reboot/crash can lose.
  struct RingConfig
  {
    uint32_t period_sec;
    size_t slot_count;
    uint32_t min_persist_sec;

    RingConfig(uint32_t period_sec_, size_t slot_count_, uint32_t min_persist_sec_ = 0)
        : period_sec(period_sec_), slot_count(slot_count_),
          min_persist_sec(min_persist_sec_) {}
  };

  /// @param signal_id NVS namespace; must be unique per instance and live
  ///                  at least as long as the History (use a string literal).
  /// @param short_cfg High-resolution (recent) ring.
  /// @param long_cfg  Low-resolution (older) ring.
  History(const char *signal_id, RingConfig short_cfg, RingConfig long_cfg);
  ~History();

  History(const History &) = delete;
  History &operator=(const History &) = delete;

  /// @brief Load persisted state from NVS. Idempotent; call once at boot.
  void begin();

  /// @brief Fold a sample into both rings and persist any completed bucket.
  /// @param now_epoch Unix timestamp; samples taken before SNTP has produced
  ///                  a real time are dropped silently.
  /// @param value     Sample value; non-finite values are dropped silently.
  ///                  Signal-specific range checks (e.g. distance > 0) are
  ///                  the caller's responsibility.
  void record(time_t now_epoch, float value);

  /// @brief Erase the short (high-resolution) ring's data — both in RAM and
  ///        in NVS. Intended for one-off "reset the last 24 h" requests from
  ///        the UI. The ring stays usable after the call; subsequent record()
  ///        starts filling fresh buckets.
  void clear_short();

  /// @brief Erase the long (low-resolution) ring's data — both in RAM and
  ///        in NVS. See clear_short() for behavior.
  void clear_long();

  /// @brief Sum of all sample values currently held in the short ring.
  ///        For the water meter this equals the total liters consumed in
  ///        the short-ring window (L/min samples × 1 min each); for a
  ///        spot signal like distance it's just a sum and rarely useful.
  ///        Empty buckets contribute 0. Cheap — single pass over slot_count.
  float short_window_sum() const;

  /// @brief Append the signal's series as a JSON object value to @p out.
  ///
  /// Format:
  /// @code
  /// {"short":{"period_sec":600,"buckets":[
  ///    {"t":1715472000,"min":..,"avg":..,"max":..,"n":..}, ... ]},
  ///  "long":{"period_sec":86400,"buckets":[ ... ]}}
  /// @endcode
  /// Empty slots are emitted as `{"t":<unix_seconds>,"n":0}`. Bucket order
  /// is chronological, oldest first.
  void serialize(String &out) const;

private:
  struct Ring; // defined in history.cpp
  Ring *short_ring_;
  Ring *long_ring_;
};
