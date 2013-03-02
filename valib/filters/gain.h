/*
  Simple gain filter
*/

#ifndef VALIB_GAIN_H
#define VALIB_GAIN_H

#include "../filter.h"

class Gain : public SamplesFilter
{
public:
  double gain;

  Gain(): gain(1.0) {};
  Gain(double gain_): gain(gain_) {}

  /////////////////////////////////////////////////////////
  // SamplesFilter overrides

  virtual bool process(Chunk &in, Chunk &out);
  virtual string info() const;
};

#endif
