#include <sstream>
#include <iomanip>
#include <math.h>
#include "gain.h"

bool
Gain::process(Chunk &in, Chunk &out)
{
  out = in;
  in.clear();
  if (out.is_dummy())
    return false;

  const size_t size = out.size;
  if (!EQUAL_SAMPLES(gain, 1.0))
    for (int ch = 0; ch < spk.nch(); ch++)
      for (size_t s = 0; s < size; s++)
        out.samples[ch][s] *= gain;
  return true;
}

string
Gain::info() const
{
  std::stringstream s;
  s << std::boolalpha << std::fixed << std::setprecision(1);
  if (!EQUAL_SAMPLES(gain, 1.0))
    s << "Enabled: " << true << nl
      << "Gain: " << value2db(gain) << nl;
  else 
    s << "Enabled: " << false << nl;
  return s.str();
}
