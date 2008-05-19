#include <math.h>
#include "source/generator.h"
#include "filters/convolver.h"
#include "../../suite.h"

static const Speakers spk = Speakers(FORMAT_LINEAR, MODE_STEREO, 48000);
static const size_t noise_size = 64 * 1024;
static const int seed = 123123;

TEST(convolver, "Convolver test")

  const double gain = 0.5;

  FIRZero zero_fir;
  FIRIdentity identity_fir;
  FIRGain gain_fir(gain);

  NoiseGen noise1;
  NoiseGen noise2;
  ZeroGen zero;

  Chunk chunk;

  // Default constructor

  Convolver conv;
  CHECK(conv.get_fir() == 0);

  // Init constructor

  Convolver conv1(&zero_fir);
  CHECK(conv1.get_fir() == &zero_fir);

  /////////////////////////////////////////////////////////
  // Change FIR

  conv.set_fir(&zero_fir);
  CHECK(conv.get_fir() == &zero_fir);

  conv.set_fir(&identity_fir);
  CHECK(conv.get_fir() == &identity_fir);

  /////////////////////////////////////////////////////////
  // Convolve with zero response

  noise1.init(spk, seed, noise_size);
  zero.init(spk, noise_size);

  conv.set_fir(&zero_fir);
  conv.reset();

  CHECK(compare(log, &noise1, &conv, &zero, 0) == 0);

  /////////////////////////////////////////////////////////
  // Convolve with identity response

  noise1.init(spk, seed, noise_size);
  noise2.init(spk, seed, noise_size);

  conv.set_fir(&identity_fir);
  conv.reset();

  CHECK(compare(log, &noise1, &conv, &noise2, 0) == 0);

  /////////////////////////////////////////////////////////
  // Change FIR on the fly
  // TODO

TEST_END(convolver);
