#include <math.h>
#include "dejitter.h"

// uncomment this to log timing information into DirectShow log
#define SYNCER_LOG_TIMING

#ifdef SYNCER_LOG_TIMING
#include <streams.h>
#endif

///////////////////////////////////////////////////////////
// SyncerStat

Syncer::SyncerStat::SyncerStat()
{
  reset();
}

void   
Syncer::SyncerStat::reset()
{
  memset(stat, 0, sizeof(stat));
}

void   
Syncer::SyncerStat::add(vtime_t _val)
{
  for (int i = array_size(stat)-1; i > 0; i--)
    stat[i] = stat[i-1];
  stat[0] = _val;
}

vtime_t 
Syncer::SyncerStat::stddev() const
{
  vtime_t sum = 0;
  for (int i = 0; i < array_size(stat); i++)
    sum += stat[i] * stat[i];
  return sqrt(sum/array_size(stat));
}

vtime_t 
Syncer::SyncerStat::mean() const
{
  vtime_t sum = 0;
  for (int i = 0; i < array_size(stat); i++)
    sum += stat[i];
  return sum/array_size(stat);
}

int 
Syncer::SyncerStat::len() const
{
  return array_size(stat);
}

///////////////////////////////////////////////////////////
// Filter interface

void 
Syncer::reset()
{
  NullFilter::reset();
  stat.reset();
  sync = false;
  time = 0;
}

bool 
Syncer::process(const Chunk *_chunk)
{
  bool    old_sync = sync;
  vtime_t old_time = time;

  // receive chunk
  FILTER_SAFE(receive_chunk(_chunk));

  // skip non-sync
  if (!_chunk->is_sync())
    return true;

  // dejitter
  if (dejitter)
  {
    if (fabs(time - old_time) / spk.sample_rate > threshold)
    {
      stat.reset();
#ifdef SYNCER_LOG_TIMING
      DbgLog((LOG_TRACE, 3, "sync lost"));
#endif
    }
    else
    {
      // use continuous time scale, but compensate nearly-constant time shift
      // (also prevent feedback oscillations)
      time = old_time + stat.mean() / stat.len() / 2; 
    }
  }

  stat.add(time - old_time);

#ifdef SYNCER_LOG_TIMING
  DbgLog((LOG_TRACE, 3, "tb: %-7.0f  tin: %-7.0f  dt: %-7.0f  s: %-7.0f  m: %-7.0fsm  tout:%-7.0f", old_time, _chunk->get_time(), (time - old_time), stat.stddev(), stat.mean(), time));
#endif

  return true;
}

bool 
Syncer::get_chunk(Chunk *_chunk)
{
  _chunk->set(spk, samples, size, sync, time * time_factor + time_shift * spk.sample_rate, flushing);
  flushing = false;
  if (!dejitter)
    sync = false;
  time += size;
  size = 0;
  return true;
}
