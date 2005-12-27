#include <math.h>
#include "dejitter.h"

// uncomment this to log timing information into DirectShow log
//#define SYNCER_LOG_TIMING

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
  istat.reset();
  ostat.reset();
  sync = false;
  time = 0;
}

bool 
Syncer::process(const Chunk *_chunk)
{
  bool    old_sync = sync;
  vtime_t old_time = time;

  // resync on format change
  if (spk != _chunk->spk)
    old_sync = false;

  // receive chunk
  FILTER_SAFE(receive_chunk(_chunk));

  // skip non-sync
  if (!_chunk->sync)
    return true;

  // resync after reset
  if (!old_sync && dejitter)
  {
    old_time = time;
//    istat.reset();
//    ostat.reset();
#ifdef SYNCER_LOG_TIMING
    DbgLog((LOG_TRACE, 3, "resync"));
#endif
  }

  // dejitter
  if (dejitter)
  {
    if (fabs(time - old_time) / spk.sample_rate > threshold)
    {
//      istat.reset();
//      ostat.reset();
#ifdef SYNCER_LOG_TIMING
      DbgLog((LOG_TRACE, 3, "sync lost"));
#endif
    }
    else
    {
      // use continuous time scale, but compensate 
      // nearly-constant time shift and slow time drift
      time = old_time + istat.mean() / istat.len() / 2; 
    }
  }

  istat.add(_chunk->time - old_time);
  ostat.add(time - old_time);

#ifdef SYNCER_LOG_TIMING
  DbgLog((LOG_TRACE, 3, "base: %-6.0f", old_time));
  DbgLog((LOG_TRACE, 3, "input:  %-6.0f delta: %-6.0f stddev: %-6.0f mean: %-6.0f", _chunk->get_time(), (_chunk->get_time() - old_time), istat.stddev(), istat.mean()));
  DbgLog((LOG_TRACE, 3, "output: %-6.0f delta: %-6.0f stddev: %-6.0f mean: %-6.0f", time, (time - old_time), ostat.stddev(), ostat.mean()));
#endif

  return true;
}

bool 
Syncer::get_chunk(Chunk *_chunk)
{
  _chunk->set(spk, samples, size, sync, time * time_factor + time_shift * spk.sample_rate, flushing);

  // do not keep sync if we do not do dejitter
  // drop sync in case of flushing
  if (!dejitter || flushing)
    sync = false;

  flushing = false;
  time += size;
  size = 0;
  return true;
}
