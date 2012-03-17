/*
  Dithering filter

  Level means the level of the dithering noise in respect to zero level.

  To dither 16bit PCM with amplitude of 1.0, dithering level should be:
  level = 1.0 (dithering amplitude) / 32768 (zero level) = 0.000030517578125 (-90dB)

  Dithering level of 0.0 means no dithering.
*/

#ifndef VALIB_DITHER_H
#define VALIB_DITHER_H

#include <math.h>
#include "../filter.h"
#include "../rng.h"

class Dither : public SamplesFilter
{
protected:
  RNG rng;

public:
  double level;
  Dither(double level_ = 0.0) {}

  /////////////////////////////////////////////////////////
  // SamplesFilter overrides

  virtual bool process(Chunk &in, Chunk &out);
  virtual string info() const;
};

#endif
