#include "dejitter.h"

#ifdef DEBUG
#include <streams.h>
#endif

bool 
Dejitter::process(const Chunk *_chunk)
{
  if (!NullFilter::process(_chunk))
    return false;

  if (chunk.is_empty())
    return true;

  if (is_resync)
  {
    if (_chunk->timestamp)
    {
      is_resync = false;
      time = _chunk->time + chunk.size;
#ifdef DEBUG
      DbgLog((LOG_TRACE, 3, "resync"));
#endif
    }
  }
  else
  {
    if (_chunk->timestamp)
    {
      time_t delta;
      if (_chunk->time > time)
        delta = _chunk->time - time;
      else
        delta = time - _chunk->time;

      jitter = jitter * 9 / 10 + float(delta * 1000 / spk.sample_rate / 10);

#ifdef DEBUG
      DbgLog((LOG_TRACE, 3, "time: %.0f\treceived time: %.0f\tdelta: %f", time, _chunk->time, time - _chunk->time));
#endif
      if (delta > threshold)
        // sync lost
        time = _chunk->time;

    }

    chunk.timestamp = true;
    chunk.time = time;
    time += _chunk->size;
  }
  return true;
}
