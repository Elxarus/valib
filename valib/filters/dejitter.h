/*
*/


#ifndef SyncGen_H
#define SyncGen_H

#include "filter.h"

class SyncGen : public NullFilter
{
protected:
  // all values in ms
  float  time_shift;
  float  time_factor;

  bool   dejitter;
  float  threshold;
  float  jitter;

public:
  SyncGen()
  {
    time_shift  = 0;
    time_factor = 1.0;

    dejitter  = true;
    threshold = 200;
    jitter    = 0;
  }

  inline void resync()
  {
    sync = false;
  }

  inline float get_time_shift() const              { return time_shift; }
  inline void  set_time_shift(float _time_shift)   { time_shift = _time_shift; }

  inline float get_time_factor() const             { return time_shift; }
  inline void  set_time_factor(float _time_shift)  { time_shift = _time_shift; }

  inline bool  get_dejitter() const                { return dejitter; }
  inline void  set_dejitter(bool _dejitter)        { dejitter = _dejitter; }

  inline float get_threshold() const               { return threshold; }
  inline void  set_threshold(float _threshold)     { threshold = _threshold; }

  inline float get_jitter() const                  { return jitter; }

  // Filter interface
  void reset()
  {
    NullFilter::reset();
    jitter = 0;
  }

  bool process(const Chunk *_chunk);
  bool get_chunk(Chunk *_chunk);
};


#endif
