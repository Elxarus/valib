/*
*/


#ifndef SyncGen_H
#define SyncGen_H

#include "filter.h"

class Syncer : public NullFilter
{
protected:
  // all time values are in secs
  vtime_t time_shift;
  vtime_t time_factor;

  bool    dejitter;
  vtime_t threshold;
  vtime_t jitter;

public:
  Syncer()
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

  inline vtime_t get_time_shift() const                 { return time_shift; }
  inline void    set_time_shift(vtime_t _time_shift)    { time_shift = _time_shift; }

  inline vtime_t get_time_factor() const                { return time_factor; }
  inline void    set_time_factor(vtime_t _time_factor ) { time_factor = _time_factor; }

  inline bool    get_dejitter() const                   { return dejitter; }
  inline void    set_dejitter(bool _dejitter)           { dejitter = _dejitter; }

  inline vtime_t get_threshold() const                  { return threshold; }
  inline void    set_threshold(vtime_t _threshold)      { threshold = _threshold; }

  inline vtime_t get_jitter() const                     { return jitter; }

  // Filter interface
  void reset()
  {
    NullFilter::reset();
    sync = false;
    time = 0;
    jitter = 0;
  }

  bool process(const Chunk *_chunk);
  bool get_chunk(Chunk *_chunk);
};


#endif
