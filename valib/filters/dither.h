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

  virtual bool process(Chunk2 &in, Chunk2 &out)
  {
    out = in;
    in.clear();
    if (out.is_dummy())
      return false;

    if (level > 0.0)
    {
      if (EQUAL_SAMPLES(level * spk.level, 1.0))
      {
        // most probable convert-to-pcm dithering
        for (int ch = 0; ch < spk.nch(); ch++)
          for (size_t s = 0; s < out.size; s++)
            out.samples[ch][s] += rng.get_sample();
      }
      else
      {
        // custom dithering
        double factor = level * spk.level;
        for (int ch = 0; ch < spk.nch(); ch++)
          for (size_t s = 0; s < out.size; s++)
            out.samples[ch][s] += rng.get_sample() * factor;
      }
    }
    return true;
  }
};

#endif
