/*
  Simple gain filter
*/

#ifndef VALIB_GAIN_H
#define VALIB_GAIN_H

#include "../filter2.h"

class Gain : public SamplesFilter
{
public:
  double gain;

  Gain(): gain(1.0) {};
  Gain(double gain_): gain(gain_) {}

  /////////////////////////////////////////////////////////
  // SamplesFilter overrides

  virtual bool process(Chunk2 &in, Chunk2 &out)
  {
    out = in;
    in.set_empty();
    if (out.is_dummy())
      return false;

    const size_t size = out.size;
    if (!EQUAL_SAMPLES(gain, 1.0))
      for (int ch = 0; ch < spk.nch(); ch++)
        for (size_t s = 0; s < size; s++)
          out.samples[ch][s] *= gain;
    return true;
  }
};

#endif
