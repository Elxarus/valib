/*
  Passthrough
  Accept any format and pass the data through.
*/

#ifndef VALIB_COUNTER_H
#define VALIB_COUNTER_H

#include "../filter2.h"

class Passthrough : public SimpleFilter
{
public:
  Passthrough()
  {}

  /////////////////////////////////////////////////////////
  // SimpleFilter overrides

  virtual bool can_open(Speakers spk) const
  { return true; }

  virtual bool process(Chunk2 &in, Chunk2 &out)
  {
    out = in;
    in.set_empty();
    return !out.is_dummy();
  }
};

#endif
