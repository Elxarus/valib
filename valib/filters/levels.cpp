#include <math.h>
#include "levels.h"

///////////////////////////////////////////////////////////
// LevelsCache

void
LevelsCache::add_levels(vtime_t _time, sample_t _levels[NCHANNELS])
{
  pos = next_pos(pos);
  if (pos == end)
    end = next_pos(end);

  levels_time[pos] = _time;
  memcpy(levels_cache[pos], _levels, sizeof(sample_t) * NCHANNELS);
}

void 
LevelsCache::get_levels(vtime_t _time, sample_t _levels[NCHANNELS], bool drop)
{
  memcpy(_levels, levels_cache[end], sizeof(sample_t) * NCHANNELS);
  if (_time < 0)
    _time = levels_time[pos];

  int b, e, ch;
  int i;

  b = end;
  e = end;
  while (levels_time[b] < _time && b != pos)
  {
    e = b;
    b = next_pos(b);
  };

  i = end;
  while (i != e)
  {
    for (ch = 0; ch < NCHANNELS; ch++)
      if (levels_cache[i][ch] > _levels[ch])
        _levels[ch] = levels_cache[i][ch];

    i = next_pos(i);
  }

  if (drop)
    end = i;
}

///////////////////////////////////////////////////////////
// Histogram

void 
LevelsHistogram::add_levels(sample_t levels[NCHANNELS])
{
  for (int ch = 0; ch < NCHANNELS; ch++)
    if (levels[ch] > 1e-50)
    {
      int level = -int(value2db(levels[ch]) / dbpb);

      if (level < 0) 
        level = 0;

      if (level < MAX_HISTOGRAM)
        histogram[ch][level]++;
    }
  n++;
}

void 
LevelsHistogram::get_histogram(double *_histogram, size_t _count) const
{
  memset(_histogram, 0, sizeof(double) * _count);
  if (n)
  {
    double inv_n = 1.0 / n;
    for (size_t i = 0; i < MAX_HISTOGRAM && i < _count; i++)
    {
      for (int ch = 0; ch < NCHANNELS; ch++)
        _histogram[i] += double(histogram[ch][i]);
      _histogram[i] *= inv_n;
    }
  }
}


///////////////////////////////////////////////////////////
// Levels

void
Levels::reset()
{
  NullFilter::reset();

  sample = 0;
  memset(levels, 0, sizeof(sample_t) * NCHANNELS);
  time = 0;

  cache.reset();
  hist.reset();
}

bool
Levels::get_chunk(Chunk *_chunk)
{
  size_t n = size;

  /////////////////////////////////////////////////////////
  // Fill output chunk

  _chunk->set(spk, samples, size, sync, time, flushing);
  sync = false;
  flushing = false;

  /////////////////////////////////////////////////////////
  // Find peak-levels

  sample_t max;
  sample_t *sptr;
  sample_t *send;

  int nch = spk.nch();
  sample_t spk_level = 1 / spk.level;
  const int *spk_order = spk.order();

  while (n)
  {
    size_t block_size = MIN(n, nsamples - sample);
    n -= block_size;
    sample += block_size;
    time += block_size;

    for (int ch = 0; ch < nch; ch++)
    {
      max = 0;
      sptr = samples[ch];
      send = sptr + block_size - 7;
      while (sptr < send)
      {
        if (fabs(sptr[0]) > max) max = fabs(sptr[0]);
        if (fabs(sptr[1]) > max) max = fabs(sptr[1]);
        if (fabs(sptr[2]) > max) max = fabs(sptr[2]);
        if (fabs(sptr[3]) > max) max = fabs(sptr[3]);
        if (fabs(sptr[4]) > max) max = fabs(sptr[4]);
        if (fabs(sptr[5]) > max) max = fabs(sptr[5]);
        if (fabs(sptr[6]) > max) max = fabs(sptr[6]);
        if (fabs(sptr[7]) > max) max = fabs(sptr[7]);
        sptr += 8;
      }
      send += 7;
      while (sptr < send)
      {
        if (fabs(sptr[0]) > max) max = fabs(sptr[0]);
        sptr++;
      }

      max *= spk_level;
      if (max > levels[spk_order[ch]])
        levels[spk_order[ch]] = max;
    }

    if (sample >= nsamples)
    {
      add_levels(time, levels);
      memset(levels, 0, sizeof(sample_t) * NCHANNELS);
      sample = 0;
    }
  }

  size = 0;
  return true;
/*

  sample_t levels_loc[NCHANNELS];
  sample_t max;
  sample_t *sptr;
  sample_t *send;

  while (n)
  {
    block_size = MIN(n, nsamples - sample);

    for (ch = 0; ch < nch; ch++)
    {
      max = 0;
      sptr = samples[ch];
      send = sptr + block_size;
      while (sptr < send)
        if *

      i = block_size;
      while (i--)
      {
        if (*sptr > max)
          max = *sptr;
        sptr++;
      }

      max /= spk.level;
      if (max > levels[order[ch]])
        levels[order[ch]] = max;
    }

    n -= block_size;
    sample += block_size;
    if (time >= 0) 
      time += block_size;

    if (sample >= nsamples)
    {
      add_levels(time, levels);
      memset(levels, 0, sizeof(sample_t) * NCHANNELS);
      sample = 0;
    }
  }


  size = 0;
  return true;
*/
}
