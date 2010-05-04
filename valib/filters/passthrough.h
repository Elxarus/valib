/*
  Passthrough
  Accept any format and pass the data through.
*/

#ifndef VALIB_PASSTHROUGH_H
#define VALIB_PASSTHROUGH_H

#include "../filter.h"

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
    in.clear();
    return !out.is_dummy();
  }
};

#endif
