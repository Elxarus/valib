/*
*/


#ifndef SyncGen_H
#define SyncGen_H

#include "filter.h"


class Syncer : public NullFilter
{
protected:
  // linear time transform
  vtime_t time_shift;
  vtime_t time_factor;

  // jitter correction
  bool    dejitter;
  vtime_t threshold;

  // statistics
  class SyncerStat
  {
  protected:
    vtime_t stat[32];

  public:
    SyncerStat();

    void reset();
    void add(vtime_t);
    vtime_t stddev() const;
    vtime_t mean() const;
    int len() const;
  };
  SyncerStat stat;

public:
  Syncer()
  {
    time_shift  = 0;
    time_factor = 1.0;

    dejitter  = true;
    threshold = 0.1;
  }

  /////////////////////////////////////////////////////////
  // Syncer interface

  void    resync()                               { sync = false; }

  // Linear time transform
  vtime_t get_time_shift() const                 { return time_shift; }
  void    set_time_shift(vtime_t _time_shift)    { time_shift = _time_shift; }

  vtime_t get_time_factor() const                { return time_factor; }
  void    set_time_factor(vtime_t _time_factor ) { time_factor = _time_factor; }

  // Jitter
  bool    get_dejitter() const                   { return dejitter; }
  void    set_dejitter(bool _dejitter)           { dejitter = _dejitter; }

  vtime_t get_threshold() const                  { return threshold; }
  void    set_threshold(vtime_t _threshold)      { threshold = _threshold; }

  vtime_t get_jitter() const                     { return stat.stddev() / spk.sample_rate; }
  vtime_t get_drift() const                      { return stat.mean() ; }

  /////////////////////////////////////////////////////////
  // Filter interface

  void reset();
  bool process(const Chunk *_chunk);
  bool get_chunk(Chunk *_chunk);
};


#endif
