#include "dejitter.h"

#ifdef DEBUG
#include <streams.h>
#endif

bool 
SyncFilter::process(const Chunk *_chunk)
{
  bool   old_sync = sync;
  time_t old_time = time;
  time_t samples2ms = 1000 / spk.sample_rate;

  if (!NullFilter::receive_chunk(_chunk))
    return false;

  // ignore non-sync chunks
  if (!_chunk->is_sync())
    return true;

  if (is_resync)
  {
    is_resync = false;
#ifdef DEBUG
    DbgLog((LOG_TRACE, 3, "resync"));
#endif
  }
  else
  {
    time_t delta;
    if (old_time > time)
      delta = old_time - time;
    else
      delta = time - old_time;

    jitter = jitter * 0.9 + delta * factor * 0.1;

#ifdef DEBUG
    DbgLog((LOG_TRACE, 3, "time: %.0f\treceived time: %.0f\tdelta: %.1f\tjitter: %.0f", old_time, time, old_time - time, jitter));
#endif
    if (delta > threshold)
    {
#ifdef DEBUG
    DbgLog((LOG_TRACE, 3, "sync lost", old_time, time, old_time - time));
#endif
    }
    else
      time = old_time;

    sync = true;
  }
  return true;
}

bool 
SyncFilter::get_chunk(Chunk *_chunk)
{
  time_t add_time = size * time_factor;
  send_chunk_inplace(_chunk, size);
  time += add_time;
  return true;
}
