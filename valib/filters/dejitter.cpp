#include "dejitter.h"

#ifdef DEBUG
#include <streams.h>
#endif

bool 
SyncGen::process(const Chunk *_chunk)
{
  time_t to_ms      = 1000 / spk.sample_rate;
  time_t to_samples = spk.sample_rate / 1000;

  bool   old_sync = sync;
  time_t old_time = time;

  // receive chunk
  if (!NullFilter::receive_chunk(_chunk))
    return false;

  // ignore non-sync chunks
  if (!_chunk->is_sync())
    return true;

  // update time
  time = time * time_factor + time_shift * to_samples;
  sync = true;

  // dejitter
  if (!old_sync)
  {
#ifdef DEBUG
    DbgLog((LOG_TRACE, 3, "resync"));
#endif
  }
  else if (dejitter)
  {
    time_t delta;
    if (old_time > time)
      delta = old_time - time;
    else
      delta = time - old_time;

    jitter = (float)(jitter * 0.9 + delta * to_ms * 0.1);

#ifdef DEBUG
    DbgLog((LOG_TRACE, 3, "time: %.0f\treceived: %.0f\tdelta: %.0f\tjitter: %.0f", old_time * to_ms, time * to_ms, (old_time - time) * to_ms, jitter));
#endif
    if (delta > threshold)
    {
#ifdef DEBUG
      DbgLog((LOG_TRACE, 3, "sync lost"));
#endif
    }
    else
      time = old_time;
  }

  return true;
}

bool 
SyncGen::get_chunk(Chunk *_chunk)
{
  time_t add_time = size * time_factor;

  send_chunk_inplace(_chunk, size);

  time += add_time;
  if (dejitter)
    sync = true;

  return true;
}
