#include <sstream>
#include <iomanip>
#include "dither.h"

bool
Dither::process(Chunk &in, Chunk &out)
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

string
Dither::info() const
{
  std::stringstream s;
  s << std::boolalpha << std::fixed << std::setprecision(1);
  s << "Enabled: " << (level > 0) << nl;
  if (level > 0)
    s << "Level: " << value2db(level) << "dB" << nl;
  return s.str();
}
