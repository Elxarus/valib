#include <math.h>
#include "suite.h"
#include "common.h"
#include "source/generator.h"
#include "fir/param_ir.h"
#include "filters/convolver.h"

static const Speakers spk = Speakers(FORMAT_LINEAR, MODE_STEREO, 48000);
static const size_t size = 64 * 1024;
static const int seed = 123123;

double rms(Source *source, Filter *filter, size_t len = 0);

///////////////////////////////////////////////////////////////////////////////

TEST(conv_zero, "Convolve with zero response")
  ZeroIR zero_ir;
  Convolver conv(&zero_ir);

  NoiseGen noise_src(spk, seed, size);
  ZeroGen zero_src(spk, size);

  CHECK(compare(log, &noise_src, &conv, &zero_src, 0) == 0);
TEST_END(conv_zero);

///////////////////////////////////////////////////////////////////////////////

TEST(conv_identity, "Convolve with identity response")
  IdentityIR identity_ir;
  Convolver conv(&identity_ir);

  NoiseGen noise1(spk, seed, size);
  NoiseGen noise2(spk, seed, size);

  CHECK(compare(log, &noise1, &conv, &noise2, 0) == 0);
TEST_END(conv_identity);

///////////////////////////////////////////////////////////////////////////////

TEST(conv_low_pass, "Convolve with low-pass filter")
  ParamIR low_pass(IR_LOW_PASS, 12000, 0, 100, 100, true);
  Convolver conv(&low_pass);

  ToneGen tone_low(spk, 11500, size);
  ToneGen tone_high(spk, 12500, size);
TEST_END(conv_low_pass);

///////////////////////////////////////////////////////////////////////////////

double rms(Source *source, Filter *filter, size_t len)
{
  Chunk chunk;
  double sum;
  int ch, i, n;

  while (1)
  {
    if (source->is_empty())
      return sqrt(sum) / n;

    if (!source->get_chunk(&chunk))
      return 0;

    if (filter)
    {
      if (!filter->process(&chunk))
        return 0;

      while (!filter->is_empty())
      {
        if (!filter->get_chunk(&chunk))
          return 0;

        for (ch = 0; ch < chunk.spk.nch(); ch++)
          for (i = 0; i < chunk.size; i++)
            sum = chunk.samples[ch][i];
        n += chunk.size * chunk.spk.nch();
      }
    }
    else
    {
      for (ch = 0; ch < chunk.spk.nch(); ch++)
        for (i = 0; i < chunk.size; i++)
          sum = chunk.samples[ch][i];
      n += chunk.size * chunk.spk.nch();
    }
  }
}

///////////////////////////////////////////////////////////////////////////////

SUITE(convolver, "Convolver filter test")
  TEST_FACTORY(conv_zero),
  TEST_FACTORY(conv_identity),
SUITE_END;
