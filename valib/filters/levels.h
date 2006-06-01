/*
  Levels & Histogram classes and filter
  Reports about current audio levels (with syncronization with ext. clock)
  Levels histogram 

  Speakers: unchanged
  Input formats: Linear
  Buffering: no
  Timing: unchanged
  Parameters:
    nsamples - number of samples for averaging
    dbpb     - dB per bin (for level histogram)
    levels   - levels
*/


#ifndef LEVELS_H
#define LEVELS_H

#include <string.h>
#include "filter.h"

class LevelsCache;
class LevelsHistogram;
class Levels;


#define MAX_LEVELS_CACHE 256
#define MAX_HISTOGRAM    128

///////////////////////////////////////////////////////////////////////////////
// LevelsCache calss
///////////////////////////////////////////////////////////////////////////////

class LevelsCache
{
protected:
  sample_t levels_cache[MAX_LEVELS_CACHE][NCHANNELS];
  vtime_t  levels_time[MAX_LEVELS_CACHE];

  int pos;
  int end;

  inline int next_pos(int p);
  inline int prev_pos(int p);

public:
  LevelsCache()
  { reset(); }

  inline void reset();

  void add_levels(vtime_t time, sample_t levels[NCHANNELS]);
  void get_levels(vtime_t time, sample_t levels[NCHANNELS], bool drop = true);
};

///////////////////////////////////////////////////////////////////////////////
// LevelsHistogram calss
///////////////////////////////////////////////////////////////////////////////

class LevelsHistogram
{
protected:
  int histogram[NCHANNELS][MAX_HISTOGRAM];
  int n;
  int dbpb; // dB per bin

public:
  LevelsHistogram(int _dbpb = 5)
  {
    set_dbpb(_dbpb);
    reset();
  };

  inline void reset();

  inline int  get_dbpb() const;
  inline void set_dbpb(int dbpb);

  void add_levels(sample_t levels[NCHANNELS]);
  void get_histogram(double *histogram, size_t count) const;
  void get_histogram(int ch, double *histogram, size_t count) const;
};

///////////////////////////////////////////////////////////////////////////////
// Levels filter calss
///////////////////////////////////////////////////////////////////////////////

class Levels : public NullFilter
{
protected:
  LevelsCache cache;
  LevelsHistogram hist;

  sample_t levels[NCHANNELS]; // currently filling 

  size_t nsamples; // number of samples per measure block
  size_t sample;   // current sample
 
public:
  Levels(int _nsamples = 1024, int _dbpb = 5)
  :NullFilter(FORMAT_MASK_LINEAR)
  {
    set_nsamples(_nsamples);
    set_dbpb(_dbpb);
    reset();
  }

  /////////////////////////////////////////////////////////
  // Levels interface

  inline int  get_nsamples() const;
  inline void set_nsamples(int);

  inline int  get_dbpb() const;
  inline void set_dbpb(int dbpb);

  inline void add_levels(vtime_t time, sample_t levels[NCHANNELS]);
  inline void get_levels(vtime_t time, sample_t levels[NCHANNELS], bool drop = true);
  inline void get_histogram(double *histogram, size_t count) const;
  inline void get_histogram(int ch, double *histogram, size_t count) const;

  /////////////////////////////////////////////////////////
  // Filter interface

  virtual void reset();
  virtual bool get_chunk(Chunk *chunk);
};

///////////////////////////////////////////////////////////
// LevelsCache inlines

int 
LevelsCache::next_pos(int p)
{
  p++;
  if (p >= MAX_LEVELS_CACHE)
    p = 0;
  return p;
}

void
LevelsCache::reset()
{
  pos = 0;
  end = 0;

  memset(levels_cache[0], 0, sizeof(sample_t) * NCHANNELS);
  levels_time[0] = -1;
}

///////////////////////////////////////////////////////////
// LevelsHistogram inlines

void 
LevelsHistogram::reset()
{
  n = 0;
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

///////////////////////////////////////////////////////////
// Levels inlines

int
Levels::get_nsamples() const
{
  return nsamples;
}

void
Levels::set_nsamples(int _nsamples)
{
  nsamples = _nsamples;
}

int 
Levels::get_dbpb() const
{
  return hist.get_dbpb();
}

void 
Levels::set_dbpb(int _dbpb)
{
  hist.set_dbpb(_dbpb);
}

void 
Levels::add_levels(vtime_t _time, sample_t _levels[NCHANNELS])
{
  cache.add_levels(_time, _levels);
  hist.add_levels(_levels);
}

void 
Levels::get_levels(vtime_t _time, sample_t _levels[NCHANNELS], bool _drop)
{
  cache.get_levels(_time, _levels, _drop);
}

void 
Levels::get_histogram(double *_histogram, size_t _count) const
{
  hist.get_histogram(_histogram, _count);
}

void 
Levels::get_histogram(int _ch, double *_histogram, size_t _count) const
{
  hist.get_histogram(_ch, _histogram, _count);
}


#endif
