#include <math.h>
#include <string.h>
#include "dejitter.h"

// uncomment this to log timing information into DirectShow log
//#define SYNCER_LOG_TIMING

#ifdef SYNCER_LOG_TIMING
#include <streams.h>
#endif

static const int format_mask_dejitter = FORMAT_CLASS_PCM | FORMAT_MASK_LINEAR | FORMAT_MASK_SPDIF;

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
  memmove(stat + 1, stat, sizeof(stat) - sizeof(stat[0]));
  stat[0] = _val;
}

vtime_t 
Syncer::SyncerStat::stddev() const
{
  vtime_t sum = 0;
  vtime_t avg = mean();
  for (int i = 0; i < array_size(stat); i++)
    sum += (stat[i] - avg) * (stat[i] - avg);
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

bool
Syncer::query_input(Speakers _spk) const
{
  if (!_spk.sample_rate)
    return false;

  return (FORMAT_MASK(_spk.format) & format_mask_dejitter) != 0;
}

bool
Syncer::set_input(Speakers _spk)
{
  reset();

  if (!_spk.sample_rate)
    return false;

  switch (_spk.format)
  {
    case FORMAT_LINEAR:
      size2time = 1.0 / _spk.sample_rate;
      break;

    case FORMAT_PCM16:
    case FORMAT_PCM16_BE:
      size2time = 1.0 / 2.0 / _spk.nch()  / _spk.sample_rate;
      break;

    case FORMAT_PCM24:
    case FORMAT_PCM24_BE:
      size2time = 1.0 / 3.0 / _spk.nch()  / _spk.sample_rate; 
      break;

    case FORMAT_PCM32:
    case FORMAT_PCM32_BE:
      size2time = 1.0 / 4.0 / _spk.nch()  / _spk.sample_rate;
      break;

    case FORMAT_PCMFLOAT:
      size2time = 1.0 / sizeof(float) / _spk.nch()  / _spk.sample_rate;
      break;

    case FORMAT_SPDIF:
      size2time = 1.0 / 4.0 / _spk.sample_rate;
      break;

    default:
      return false;
  }

  return NullFilter::set_input(_spk);
}


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
  // we must ignore dummy chunks
  if (_chunk->is_dummy())
    return true;

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
    if (fabs(time - old_time) > threshold)
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
  _chunk->set
  (
    spk, 
    rawdata, samples, size, 
    sync, time * time_factor + time_shift, 
    flushing
  );

  // do not keep sync if we do not do dejitter
  // drop sync in case of flushing
  if (!dejitter || flushing)
    sync = false;

  flushing = false;
  time += size * size2time;
  size = 0;
  return true;
}
