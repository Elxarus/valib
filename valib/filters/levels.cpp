#include <math.h>
#include "levels.h"

///////////////////////////////////////////////////////////
// LevelsCache

LevelsCache::LevelsCache()
{
  reset();
}

void
LevelsCache::reset()
{
  pos = 0;
  end = 0;

  memset(levels_cache[0], 0, sizeof(sample_t) * CH_NAMES);
  levels_time[0] = -1;
}

inline int 
LevelsCache::next_pos(int p)
{
  p++;
  if (p >= MAX_LEVELS_CACHE)
    p = 0;
  return p;
}

void
LevelsCache::add_levels(vtime_t _time, sample_t _levels[CH_NAMES])
{
  pos = next_pos(pos);
  if (pos == end)
    end = next_pos(end);

  levels_time[pos] = _time;
  memcpy(levels_cache[pos], _levels, sizeof(sample_t) * CH_NAMES);
}

void 
LevelsCache::get_levels(vtime_t _time, sample_t _levels[CH_NAMES], bool drop)
{
  memcpy(_levels, levels_cache[end], sizeof(sample_t) * CH_NAMES);
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
    for (ch = 0; ch < CH_NAMES; ch++)
      if (levels_cache[i][ch] > _levels[ch])
        _levels[ch] = levels_cache[i][ch];

    i = next_pos(i);
  }

  if (drop)
    end = i;
}

///////////////////////////////////////////////////////////
// Histogram

LevelsHistogram::LevelsHistogram(int _dbpb)
{
  set_dbpb(_dbpb);
  reset();
};

void 
LevelsHistogram::reset()
{
  n = 0;
  memset(max_level, 0, sizeof(max_level));
  memset(histogram, 0, sizeof(histogram));
}

int 
LevelsHistogram::get_dbpb() const
{
  return dbpb;
}

void 
LevelsHistogram::set_dbpb(int _dbpb)
{
  dbpb = _dbpb;
  reset();
}

void 
LevelsHistogram::add_levels(sample_t levels[CH_NAMES])
{
  for (int ch = 0; ch < CH_NAMES; ch++)
    if (levels[ch] > 1e-50)
    {
      if (levels[ch] > max_level[ch])
        max_level[ch] = levels[ch];

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
      for (int ch = 0; ch < CH_NAMES; ch++)
        _histogram[i] += double(histogram[ch][i]);
      _histogram[i] *= inv_n;
    }
  }
}

void 
LevelsHistogram::get_histogram(int _ch, double *_histogram, size_t _count) const
{
  memset(_histogram, 0, sizeof(double) * _count);
  if (n && _ch >= 0 && _ch < CH_NAMES)
  {
    double inv_n = 1.0 / n;
    for (size_t i = 0; i < MAX_HISTOGRAM && i < _count; i++)
      _histogram[i] += double(histogram[_ch][i]) * inv_n;
  }
}

sample_t
LevelsHistogram::get_max_level() const
{
  sample_t max = max_level[0];
  for (int ch = 1; ch < CH_NAMES; ch++)
    if (max_level[ch] > max)
      max = max_level[ch];
  return max;
}

sample_t
LevelsHistogram::get_max_level(int _ch) const
{
  if (_ch >= 0 && _ch < CH_NAMES)
    return max_level[_ch];
  return 0.0;
}


///////////////////////////////////////////////////////////
// Levels

Levels::Levels(size_t _nsamples, int _dbpb)
{
  set_nsamples(_nsamples);
  set_dbpb(_dbpb);
  reset();
}

void
Levels::reset()
{
  sample = 0;
  memset(levels, 0, sizeof(sample_t) * CH_NAMES);
  continuous_time = 0;

  cache.reset();
  hist.reset();
}

bool
Levels::process(Chunk &in, Chunk &out)
{
  out = in;
  in.clear();
  if (out.is_dummy())
    return false;

  if (out.sync)
    continuous_time = out.time;

  /////////////////////////////////////////////////////////
  // Find peak-levels

  sample_t max;
  sample_t *sptr;
  sample_t *send;

  int nch = spk.nch();
  sample_t spk_level = 1.0 / spk.level;

  order_t order;
  spk.get_order(order);

  size_t n = out.size;
  while (n)
  {
    size_t block_size = MIN(n, nsamples - sample);
    n -= block_size;
    sample += block_size;

    for (int ch = 0; ch < nch; ch++)
    {
      max = 0;
      sptr = out.samples[ch];
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
      if (max > levels[order[ch]])
        levels[order[ch]] = max;
    }

    if (sample >= nsamples)
    {
      add_levels(continuous_time, levels);
      memset(levels, 0, sizeof(sample_t) * CH_NAMES);
      sample = 0;
    }

    continuous_time += vtime_t(block_size) / spk.sample_rate;
  }

  return true;
}
