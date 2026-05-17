#include "history.h"

#include <Preferences.h>

#include <math.h>
#include <new>
#include <stdio.h>

/// 2024-01-01 UTC — samples with an earlier epoch are assumed to be from
/// before SNTP sync and are dropped. Matches util.cpp's threshold.
static constexpr time_t SYNC_THRESHOLD = 1704067200;

/// On-disk bucket layout. Loaded straight from NVS via getBytes() so the
/// member order, types, and packing must stay stable — if you change them,
/// rev the NVS key name to avoid reading garbage from old firmware.
struct Bucket
{
  uint32_t t_unit;
  uint32_t n;
  float min_v;
  float max_v;
  float sum_v;
};

/// One ring buffer with NVS-backed persistence. A History owns two of these
/// (short and long resolution); the public API is through History.
struct History::Ring
{
  const char *ns;  // NVS namespace ("dist", "flow", …)
  const char *key; // NVS key within the namespace ("s", "l")
  uint32_t period_sec;
  size_t slot_count;
  Bucket *buckets;          // owned heap buffer, slot_count entries
  uint32_t min_persist_sec; // 0 = flush on every rollover
  time_t last_persist_epoch;

  Ring(const char *ns_, const char *key_,
       uint32_t period_sec_, size_t slot_count_, uint32_t min_persist_sec_)
      : ns(ns_), key(key_), period_sec(period_sec_), slot_count(slot_count_), buckets(new (std::nothrow) Bucket[slot_count_]()), min_persist_sec(min_persist_sec_), last_persist_epoch(0)
  {
  }

  ~Ring() { delete[] buckets; }

  void begin()
  {
    if (!buckets)
      return;
    Preferences prefs;
    if (!prefs.begin(ns, /*readOnly=*/true))
      return;
    const size_t expected = slot_count * sizeof(Bucket);
    if (prefs.getBytesLength(key) == expected)
    {
      prefs.getBytes(key, buckets, expected);
    }
    prefs.end();
  }

  /// True if the configured min_persist_sec window hasn't elapsed since
  /// the last successful write. Time going backwards (NTP correction)
  /// counts as "not throttled" so we resync on the next rollover.
  bool throttled(time_t now_epoch) const
  {
    if (min_persist_sec == 0)
      return false;
    if (now_epoch < last_persist_epoch)
      return false;
    return (uint32_t)(now_epoch - last_persist_epoch) < min_persist_sec;
  }

  /// Write the whole ring to NVS. NVS wear-levels across the partition, so
  /// even at ~24 writes/day per ring the device outlives its flash budget.
  void persist()
  {
    if (!buckets)
      return;
    Preferences prefs;
    if (!prefs.begin(ns, /*readOnly=*/false))
      return;
    prefs.putBytes(key, buckets, slot_count * sizeof(Bucket));
    prefs.end();
  }

  void record(uint32_t t_unit_now, float value, time_t now_epoch)
  {
    if (!buckets)
      return;
    Bucket &b = buckets[t_unit_now % slot_count];
    const bool rollover = (b.n > 0 && b.t_unit != t_unit_now);
    if (rollover && !throttled(now_epoch))
    {
      persist();
      last_persist_epoch = now_epoch;
    }
    if (b.n == 0 || b.t_unit != t_unit_now)
    {
      b.t_unit = t_unit_now;
      b.n = 0;
      b.min_v = value;
      b.max_v = value;
      b.sum_v = 0.0f;
    }
    if (value < b.min_v)
      b.min_v = value;
    if (value > b.max_v)
      b.max_v = value;
    b.sum_v += value;
    b.n += 1;
  }

  /// Zero the in-memory buckets and remove the NVS key. Subsequent record()
  /// calls start writing into fresh buckets; subsequent persist() will write
  /// a fresh blob to NVS.
  void clear()
  {
    if (buckets)
    {
      for (size_t i = 0; i < slot_count; i++)
      {
        buckets[i] = Bucket{};
      }
    }
    last_persist_epoch = 0;
    Preferences prefs;
    if (prefs.begin(ns, /*readOnly=*/false))
    {
      prefs.remove(key);
      prefs.end();
    }
  }

  /// Emit bucket entries (no surrounding `[]`) to @p out, oldest first.
  void serialize_buckets(String &out, uint32_t current_t_unit) const
  {
    if (!buckets)
      return;
    const uint32_t start = (current_t_unit >= slot_count - 1)
                               ? (current_t_unit - (slot_count - 1))
                               : 0;
    bool first = true;
    for (uint32_t t = start; t <= current_t_unit; t++)
    {
      if (!first)
        out += ',';
      first = false;
      const Bucket &b = buckets[t % slot_count];
      out += "{\"t\":";
      out += (uint32_t)(t * period_sec);
      if (b.n > 0 && b.t_unit == t)
      {
        char tmp[64];
        snprintf(tmp, sizeof(tmp),
                 ",\"min\":%.2f,\"avg\":%.2f,\"max\":%.2f,\"n\":%u",
                 b.min_v, b.sum_v / (float)b.n, b.max_v, (unsigned)b.n);
        out += tmp;
      }
      else
      {
        out += ",\"n\":0";
      }
      out += '}';
    }
  }
};

// ---------------- History ----------------

History::History(const char *signal_id, RingConfig short_cfg, RingConfig long_cfg)
    : short_ring_(new Ring(signal_id, "s",
                           short_cfg.period_sec, short_cfg.slot_count, short_cfg.min_persist_sec)),
      long_ring_(new Ring(signal_id, "l",
                          long_cfg.period_sec, long_cfg.slot_count, long_cfg.min_persist_sec))
{
}

History::~History()
{
  delete short_ring_;
  delete long_ring_;
}

void History::begin()
{
  if (short_ring_)
    short_ring_->begin();
  if (long_ring_)
    long_ring_->begin();
}

void History::clear_short()
{
  if (short_ring_)
    short_ring_->clear();
}

void History::clear_long()
{
  if (long_ring_)
    long_ring_->clear();
}

float History::short_window_sum() const
{
  if (!short_ring_ || !short_ring_->buckets)
    return 0.0f;
  float total = 0.0f;
  for (size_t i = 0; i < short_ring_->slot_count; i++)
  {
    const Bucket &b = short_ring_->buckets[i];
    if (b.n > 0)
      total += b.sum_v;
  }
  return total;
}

void History::record(time_t now_epoch, float value)
{
  if (now_epoch < SYNC_THRESHOLD)
    return;
  if (!isfinite(value))
    return;
  if (short_ring_)
  {
    short_ring_->record((uint32_t)(now_epoch / (time_t)short_ring_->period_sec),
                        value, now_epoch);
  }
  if (long_ring_)
  {
    long_ring_->record((uint32_t)(now_epoch / (time_t)long_ring_->period_sec),
                       value, now_epoch);
  }
}

void History::serialize(String &out) const
{
  const time_t now = time(nullptr);
  const bool synced = (now >= SYNC_THRESHOLD);

  out += "{\"short\":{\"period_sec\":";
  out += short_ring_ ? short_ring_->period_sec : 0u;
  out += ",\"buckets\":[";
  if (synced && short_ring_)
  {
    const uint32_t t = (uint32_t)(now / (time_t)short_ring_->period_sec);
    short_ring_->serialize_buckets(out, t);
  }
  out += "]},\"long\":{\"period_sec\":";
  out += long_ring_ ? long_ring_->period_sec : 0u;
  out += ",\"buckets\":[";
  if (synced && long_ring_)
  {
    const uint32_t t = (uint32_t)(now / (time_t)long_ring_->period_sec);
    long_ring_->serialize_buckets(out, t);
  }
  out += "]}}";
}
