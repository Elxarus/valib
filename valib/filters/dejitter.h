/*
  Jitter correction
  See valib/doc/jitter.xls for explanations
*/

#ifndef VALIB_DEJITTER_H
#define VALIB_DEJITTER_H

#include "../filter2.h"

#define STAT_SIZE 64

class Syncer : public SimpleFilter
{
protected:
  double size2time;

  // continious time
  bool    continuous_sync;
  vtime_t continuous_time;

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
    vtime_t stat[STAT_SIZE];

  public:
    SyncerStat();

    void reset();
    void add(vtime_t);
    vtime_t stddev() const;
    vtime_t mean() const;
    int len() const;
  };
  SyncerStat istat;
  SyncerStat ostat;

public:
  Syncer();

  /////////////////////////////////////////////////////////
  // Syncer interface

  void    resync()                               { continuous_sync = false; }

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

  vtime_t get_input_mean() const                 { return istat.mean(); }
  vtime_t get_input_stddev() const               { return istat.stddev(); }
  vtime_t get_output_mean() const                { return ostat.mean(); }
  vtime_t get_output_stddev() const              { return ostat.stddev(); }

  /////////////////////////////////////////////////////////
  // SimpleFilter overrides

  virtual bool can_open(Speakers spk) const;
  virtual bool init(Speakers spk);
  virtual bool process(Chunk2 &in, Chunk2 &out);
  virtual void reset();
};

#endif
