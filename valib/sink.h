/*
  Abstract audio sink interface
  NullSink implementation
*/

#ifndef SINK_H
#define SINK_H

#include "spk.h"
#include "filter.h"

class AudioSink;
class NullSink;

// vol is [0..1.0]
// pan is [-1.0..1.0]

class AudioSink : public Sink
{
public:
  // playback control
  virtual bool query(Speakers spk) const = 0;
  virtual bool open(Speakers spk) = 0;
  virtual void close() = 0;
  virtual bool is_open() const = 0;

  virtual void pause() = 0;
  virtual void unpause() = 0;
  virtual bool is_paused() = 0;
 
  virtual void flush() = 0; // reset all internal buffers to start playback from new point

  // timing
  virtual time_t get_output_time() = 0;    // end time of last chunk processed
  virtual time_t get_playback_time() = 0;  // current playback time
  virtual time_t get_lag_time() = 0;       // output and playback time difference

  // volume & pan
  virtual void   set_vol(double vol) = 0;
  virtual double get_vol() = 0;
  virtual void   set_pan(double pan) = 0;
  virtual double get_pan() = 0;

  // data write
  virtual bool can_write_immediate(const Chunk *chunk) = 0;
  virtual bool write(const Chunk *chunk) = 0;

  // Sink interface
  virtual bool query_input(Speakers spk) const { return query(spk); }
  virtual bool set_input(Speakers spk)         { return open(spk);  }
  virtual bool process(const Chunk *chunk)
  {
    if (chunk->is_empty())
      return true;

    if (!is_open() && !open(chunk->spk))
      return false;
    return write(chunk); 
  };
};


class NullSink : public AudioSink
{
protected:
  Speakers spk;

  double vol;
  double pan;

  time_t time;
  bool   opened;
  bool   paused;

public:
  NullSink()
  {
    vol = 1.0;
    pan = 0;

    time   = 0;
    opened = false;
    paused = false;
  }
   
  // playback control
  virtual bool query(Speakers _spk) const { return true;    }
  virtual bool open(Speakers _spk)    
  { 
    opened = query(spk);
    if (opened) spk = _spk;
    return opened; 
  }
  virtual void close()                { opened = false; }
  virtual bool is_open() const        { return opened;  }

  virtual void pause()                { paused = true;  }
  virtual void unpause()              { paused = false; }
  virtual bool is_paused()            { return paused;  }

  virtual void flush()                { time = 0;       }

  // timing
  virtual time_t get_output_time()    { return time;    }
  virtual time_t get_playback_time()  { return time;    }
  virtual time_t get_lag_time()       { return get_output_time() - get_playback_time(); }

  // volume & pan
  virtual void   set_vol(double _vol) { vol = _vol;     }
  virtual double get_vol()            { return vol;     }
  virtual void   set_pan(double _pan) { pan = _pan;     }
  virtual double get_pan()            { return pan;     }

  // data write
  virtual bool can_write_immediate(const Chunk *chunk) { return opened && !paused; }
  virtual bool write(const Chunk *chunk)               
  { 
    if (spk != chunk->spk)
      return false;

    if (chunk->time > 0)
      time = chunk->time + chunk->size;
    else
      time += chunk->size;
    return true;
  }
};

#endif
