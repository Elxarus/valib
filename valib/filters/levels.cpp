#include <math.h>
#include "levels.h"

///////////////////////////////////////////////////////////
// LevelsCache

void
LevelsCache::add_levels(time_t _time, sample_t _levels[NCHANNELS])
{
  if (_time < _time) return;

  pos = next_pos(pos);
  if (pos == end)
    end = next_pos(end);

  levels_time[pos] = _time;
  memcpy(levels_cache[pos], _levels, sizeof(sample_t) * NCHANNELS);
}

void 
LevelsCache::get_levels(time_t _time, sample_t _levels[NCHANNELS], bool drop)
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
      int level = -int(value2db(levels[ch])) / dbpb;
      if (level < MAX_HISTOGRAM)
        histogram[ch][level]++;
    }
  n++;
}

void 
LevelsHistogram::get_histogram(int _count, double *_histogram)
{
  memset(_histogram, 0, sizeof(double) * _count);
  if (n)
  {
    double inv_n = 1.0 / n;
    for (int i = 0; i < MAX_HISTOGRAM && i < _count; i++)
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
  chunk.set_empty();

  samples = 0;
  memset(levels, 0, sizeof(sample_t) * NCHANNELS);
  time = -1;

  cache.reset();
  hist.reset();
}

bool
Levels::process(const Chunk *_chunk)
{
  if (!NullFilter::process(_chunk))
    return false;

  if (chunk.timestamp)
    time = chunk.time;

  int n = chunk.size;
  int nch = chunk.spk.nch();
  const int *order = chunk.spk.order();

  int ch, i, block_size;
  sample_t *sptr;
  sample_t max;

  while (n)
  {
    block_size = MIN(n, nsamples - samples);
    for (ch = 0; ch < nch; ch++)
    {
      sptr = chunk.samples[ch];
  
      max = 0;
      i = block_size;
      while (i--)
      {
        if (*sptr > max)
          max = *sptr;
        sptr++;
      }

      max /= chunk.spk.level;
      if (max > levels[order[ch]])
        levels[order[ch]] = max;
    }

    n -= block_size;
    samples += block_size;
    if (time >= 0) 
      time += block_size;

    if (samples >= nsamples)
    {
      add_levels(time, levels);
      memset(levels, 0, sizeof(sample_t) * NCHANNELS);
      samples = 0;
    }
  }

  return true;
}
