/*
*/


#ifndef DEJITTER_H
#define DEJITTER_H

#include "filter.h"

class Dejitter : public NullFilter
{
protected:
  bool   is_resync;
  time_t time;
  time_t threshold;
  float  jitter;     // ms

public:
  Dejitter()
  {
    is_resync = true;
    threshold = 5000;
    jitter = 0;
  }
                       
  inline void   resync();

  inline time_t get_threshold();
  inline void   set_threshold(time_t _threshold);
  inline float  get_jitter();


  // Filter interface
  void reset()
  {
    chunk.set_empty();
    is_resync = true;
    jitter = 0;
  }

  bool process(const Chunk *_chunk);
};


///////////////////////////////////////////////////////////////////////////////
// Dejitter inlines

inline void   Dejitter::resync()                         { is_resync = true; }

inline time_t Dejitter::get_threshold()                  { return threshold; }
inline void   Dejitter::set_threshold(time_t _threshold) { threshold = _threshold; }
inline float  Dejitter::get_jitter()                     { return jitter; }

#endif
