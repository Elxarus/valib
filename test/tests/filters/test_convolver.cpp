#include <math.h>
#include "source/generator.h"
#include "filters/convolver.h"
#include "filters/slice.h"
#include "fir/param_ir.h"
#include "../../suite.h"

static const Speakers spk = Speakers(FORMAT_LINEAR, MODE_STEREO, 48000);
static const size_t noise_size = 64 * 1024;
static const int seed = 123123;

///////////////////////////////////////////////////////////////////////////////

TEST(conv_test, "Convolver test")

  const double gain = 0.5;

  FIRZero zero_gen;
  FIRIdentity identity_gen;
  FIRGain gain_gen(gain);

  NoiseGen noise1;
  NoiseGen noise2;
  ZeroGen zero;

  Chunk chunk;

  // Default constructor

  Convolver2 conv;
  CHECK(conv.get_gen() == 0);

  // Init constructor

  Convolver2 conv1(&zero_gen);
  CHECK(conv1.get_gen() == &zero_gen);

  /////////////////////////////////////////////////////////
  // Change FIR

  conv.set_gen(&zero_gen);
  CHECK(conv.get_gen() == &zero_gen);

  conv.set_gen(&identity_gen);
  CHECK(conv.get_gen() == &identity_gen);

  /////////////////////////////////////////////////////////
  // Convolve with zero response

  noise1.init(spk, seed, noise_size);
  zero.init(spk, noise_size);

  conv.set_gen(&zero_gen);
  conv.reset();

  CHECK(compare(log, &noise1, &conv, &zero, 0) == 0);

  /////////////////////////////////////////////////////////
  // Convolve with identity response

  noise1.init(spk, seed, noise_size);
  noise2.init(spk, seed, noise_size);

  conv.set_gen(&identity_gen);
  conv.reset();

  CHECK(compare(log, &noise1, &conv, &noise2, 0) == 0);

  /////////////////////////////////////////////////////////
  // Change FIR on the fly
  // TODO

TEST_END(conv_test);

///////////////////////////////////////////////////////////////////////////////

TEST(conv_param, "Convolve with parametric filter")
  const double att = 100; // 100dB attenuation
  const int trans = 100; // 100Hz transition bandwidth
  double diff, level;
  int len;

  const FIRInstance *fir;

  // FIRs

  int freq = spk.sample_rate / 4;
  ParamFIR low_pass(IR_LOW_PASS, freq, 0, trans, att);
  ParamFIR high_pass(IR_HIGH_PASS, freq, 0, trans, att);
  ParamFIR band_pass(IR_BAND_PASS, freq - trans, freq + trans, trans, att);
  ParamFIR band_stop(IR_BAND_STOP, freq - trans, freq + trans, trans, att);

  // Test source test_src (tone -> convolver -> slice)

  ToneGen tone;
  SliceFilter slice;
  Convolver2 conv;
  SourceFilter conv_src(&tone, &conv);
  SourceFilter test_src(&conv_src, &slice);

  // Reference source ref_src (tone -> slice)

  ToneGen ref_tone;
  SliceFilter ref_slice;
  SourceFilter ref_src(&ref_tone, &ref_slice);

  /////////////////////////////////////////////////////////
  // Low pass
  /////////////////////////////////////////////////////////

  fir = low_pass.make(spk.sample_rate);
  CHECK(fir != 0);
  len = 2 * fir->length;
  delete fir;

  conv.set_gen(&low_pass);

  /////////////////////////////////////////////////////////
  // Tone in the pass band must remain unchanged

  tone.init(spk, freq - trans, noise_size + 2 * len);
  slice.init(len, noise_size + len);
  ref_tone.init(spk, freq - trans, noise_size + 2 * len);
  ref_slice.init(len, len + noise_size);
  conv.reset();

  diff = calc_diff(&test_src, &ref_src);
  CHECK(diff > 0);
  CHECK(value2db(diff) < -att);

  /////////////////////////////////////////////////////////
  // Tone in the stop band must be filtered out

  tone.init(spk, freq + trans, noise_size + 2 * len);
  slice.init(len, noise_size + len);
  conv.reset();

  level = calc_peak(&test_src);
  CHECK(level > 0);
  CHECK(value2db(level) < -att);

  /////////////////////////////////////////////////////////
  // High pass
  /////////////////////////////////////////////////////////

  fir = high_pass.make(spk.sample_rate);
  CHECK(fir != 0);
  len = 2 * fir->length;
  delete fir;

  conv.set_gen(&high_pass);

  /////////////////////////////////////////////////////////
  // Tone in the pass band must remain unchanged

  tone.init(spk, freq + trans, noise_size + 2 * len);
  slice.init(len, noise_size + len);
  ref_tone.init(spk, freq + trans, noise_size + 2 * len);
  ref_slice.init(len, len + noise_size);
  conv.reset();

  diff = calc_diff(&test_src, &ref_src);
  CHECK(diff > 0);
  CHECK(value2db(diff) < -att);

  /////////////////////////////////////////////////////////
  // Tone in the stop band must be filtered out

  tone.init(spk, freq - trans, noise_size + 2 * len);
  slice.init(len, noise_size + len);
  conv.reset();

  level = calc_peak(&test_src);
  CHECK(level > 0);
  CHECK(value2db(level) < -att);

  /////////////////////////////////////////////////////////
  // BandPass
  /////////////////////////////////////////////////////////

  fir = band_pass.make(spk.sample_rate);
  CHECK(fir != 0);
  len = 2 * fir->length;
  delete fir;

  conv.set_gen(&band_pass);

  /////////////////////////////////////////////////////////
  // Tone in the pass band must remain unchanged

  tone.init(spk, freq, noise_size + 2 * len);
  slice.init(len, noise_size + len);
  ref_tone.init(spk, freq, noise_size + 2 * len);
  ref_slice.init(len, len + noise_size);
  conv.reset();

  diff = calc_diff(&test_src, &ref_src);
  CHECK(diff > 0);
  CHECK(value2db(diff) < -att);

  /////////////////////////////////////////////////////////
  // Tones at stop bands must be filtered out

  tone.init(spk, freq - 2 * trans, noise_size + 2 * len);
  slice.init(len, noise_size + len);
  conv.reset();

  level = calc_peak(&test_src);
  CHECK(level > 0);
  CHECK(value2db(level) < -att);

  tone.init(spk, freq + 2 * trans, noise_size + 2 * len);
  slice.init(len, noise_size + len);
  conv.reset();

  level = calc_peak(&test_src);
  CHECK(level > 0);
  CHECK(value2db(level) < -att);

  /////////////////////////////////////////////////////////
  // BandStop
  /////////////////////////////////////////////////////////

  fir = band_stop.make(spk.sample_rate);
  CHECK(fir != 0);
  len = 2 * fir->length;
  delete fir;

  conv.set_gen(&band_stop);

  /////////////////////////////////////////////////////////
  // Tones at pass bands must remain unchanged

  tone.init(spk, freq - 2 * trans, noise_size + 2 * len);
  slice.init(len, noise_size + len);
  ref_tone.init(spk, freq - 2 * trans, noise_size + 2 * len);
  ref_slice.init(len, len + noise_size);
  conv.reset();

  diff = calc_diff(&test_src, &ref_src);
  CHECK(diff > 0);
  CHECK(value2db(diff) < -att);

  tone.init(spk, freq + 2 * trans, noise_size + 2 * len);
  slice.init(len, noise_size + len);
  ref_tone.init(spk, freq + 2 * trans, noise_size + 2 * len);
  ref_slice.init(len, len + noise_size);
  conv.reset();

  diff = calc_diff(&test_src, &ref_src);
  CHECK(diff > 0);
  CHECK(value2db(diff) < -att);

  /////////////////////////////////////////////////////////
  // Tone in the stop band must be filtered out

  tone.init(spk, freq, noise_size + 2 * len);
  slice.init(len, noise_size + len);
  conv.reset();

  level = calc_peak(&test_src);
  CHECK(level > 0);
  CHECK(value2db(level) < -att);

TEST_END(conv_param);

///////////////////////////////////////////////////////////////////////////////

TEST(conv_test_old, "Convolver test (to remove)")
  ZeroIR zero_ir;
  IdentityIR identity_ir;
  NoiseGen noise1;
  NoiseGen noise2;
  ZeroGen zero;

  Chunk chunk;

  // Default constructor

  Convolver conv;
  CHECK(conv.get_ir() == 0);

  // Init constructor

  Convolver conv1(&zero_ir);
  CHECK(conv1.get_ir() == &zero_ir);

  /////////////////////////////////////////////////////////
  // Change IR

  conv.set_ir(&zero_ir);
  CHECK(conv.get_ir() == &zero_ir);

  conv.set_ir(&identity_ir);
  CHECK(conv.get_ir() == &identity_ir);

  /////////////////////////////////////////////////////////
  // Convolve with zero response

  noise1.init(spk, seed, noise_size);
  zero.init(spk, noise_size);

  conv.set_ir(&zero_ir);
  conv.reset();

  CHECK(compare(log, &noise1, &conv, &zero, 0) == 0);

  /////////////////////////////////////////////////////////
  // Convolve with identity response

  noise1.init(spk, seed, noise_size);
  noise2.init(spk, seed, noise_size);

  conv.set_ir(&identity_ir);
  conv.reset();

  CHECK(compare(log, &noise1, &conv, &noise2, 0) == 0);

  /////////////////////////////////////////////////////////
  // Change IR on the fly

  noise1.init(spk, seed, noise_size);
  conv.set_ir(&identity_ir);
  conv.reset();

  while (conv.is_empty())
  {
    CHECK(!noise1.is_empty())
    noise1.get_chunk(&chunk);
    CHECK(conv.process(&chunk) == true);
  }

  // Now convolver is full and must generate noise chunks
  // even if we change the response right now. After this
  // convolver must end the stream and switch to the new
  // response.

  conv.set_ir(&zero_ir);
  do {
    conv.get_chunk(&chunk);
    if (chunk.size)
      CHECK(chunk.samples[0][0] != 0);
  } while (!chunk.eos);

  // Now stream was ended and zero response will be in
  // action.

  while (conv.is_empty())
  {
    CHECK(!noise1.is_empty())
    noise1.get_chunk(&chunk);
    CHECK(conv.process(&chunk) == true);
  }

  // Now convolver is full and must generate zero chunks
  // even if we change the response right now. After this
  // convolver must end the stream and switch to the new
  // response.

  conv.set_ir(&identity_ir);
  do {
    conv.get_chunk(&chunk);
    if (chunk.size)
      CHECK(chunk.samples[0][0] == 0);
  } while (!chunk.eos);

  // Now stream was ended and identity response will be
  // back again.

  while (conv.is_empty())
  {
    CHECK(!noise1.is_empty())
    noise1.get_chunk(&chunk);
    CHECK(conv.process(&chunk) == true);
  }

  conv.set_ir(&zero_ir);
  do {
    conv.get_chunk(&chunk);
    if (chunk.size)
      CHECK(chunk.samples[0][0] != 0);
  } while (!chunk.eos);

TEST_END(conv_test_old);

///////////////////////////////////////////////////////////////////////////////

TEST(conv_param_old, "Convolve with parametric filter (to remove)")
  const double att = 100; // 100dB attenuation
  const int trans = 100; // 100Hz transition bandwidth
  double diff, level;
  int len;

  // Impulse responses

  int freq = spk.sample_rate / 4;
  ParamIR low_pass(IR_LOW_PASS, freq, 0, trans, att);
  ParamIR high_pass(IR_HIGH_PASS, freq, 0, trans, att);
  ParamIR band_pass(IR_BAND_PASS, freq - trans, freq + trans, trans, att);
  ParamIR band_stop(IR_BAND_STOP, freq - trans, freq + trans, trans, att);

  // Test source test_src (tone -> convolver -> slice)

  ToneGen tone;
  SliceFilter slice;
  Convolver conv;
  SourceFilter conv_src(&tone, &conv);
  SourceFilter test_src(&conv_src, &slice);

  // Reference source ref_src (tone -> slice)

  ToneGen ref_tone;
  SliceFilter ref_slice;
  SourceFilter ref_src(&ref_tone, &ref_slice);

  /////////////////////////////////////////////////////////
  // Low pass
  /////////////////////////////////////////////////////////

  conv.set_ir(&low_pass);
  len = 2 * low_pass.min_length(spk.sample_rate);

  /////////////////////////////////////////////////////////
  // Tone in the pass band must remain unchanged

  tone.init(spk, freq - trans, noise_size + 2 * len);
  slice.init(len, noise_size + len);
  ref_tone.init(spk, freq - trans, noise_size + 2 * len);
  ref_slice.init(len, len + noise_size);
  conv.reset();

  diff = calc_diff(&test_src, &ref_src);
  CHECK(diff > 0);
  CHECK(log10(diff) * 20 < -att);

  /////////////////////////////////////////////////////////
  // Tone in the stop band must be filtered out

  tone.init(spk, freq + trans, noise_size + 2 * len);
  slice.init(len, noise_size + len);
  conv.reset();

  level = calc_peak(&test_src);
  CHECK(level > 0);
  CHECK(log10(level) * 20 < -att);

  /////////////////////////////////////////////////////////
  // High pass
  /////////////////////////////////////////////////////////

  conv.set_ir(&high_pass);
  len = 2 * high_pass.min_length(spk.sample_rate);

  /////////////////////////////////////////////////////////
  // Tone in the pass band must remain unchanged

  tone.init(spk, freq + trans, noise_size + 2 * len);
  slice.init(len, noise_size + len);
  ref_tone.init(spk, freq + trans, noise_size + 2 * len);
  ref_slice.init(len, len + noise_size);
  conv.reset();

  diff = calc_diff(&test_src, &ref_src);
  CHECK(diff > 0);
  CHECK(log10(diff) * 20 < -att);

  /////////////////////////////////////////////////////////
  // Tone in the stop band must be filtered out

  tone.init(spk, freq - trans, noise_size + 2 * len);
  slice.init(len, noise_size + len);
  conv.reset();

  level = calc_peak(&test_src);
  CHECK(level > 0);
  CHECK(log10(level) * 20 < -att);

  /////////////////////////////////////////////////////////
  // BandPass
  /////////////////////////////////////////////////////////

  conv.set_ir(&band_pass);
  len = 2 * band_pass.min_length(spk.sample_rate);

  /////////////////////////////////////////////////////////
  // Tone in the pass band must remain unchanged

  tone.init(spk, freq, noise_size + 2 * len);
  slice.init(len, noise_size + len);
  ref_tone.init(spk, freq, noise_size + 2 * len);
  ref_slice.init(len, len + noise_size);
  conv.reset();

  diff = calc_diff(&test_src, &ref_src);
  CHECK(diff > 0);
  CHECK(log10(diff) * 20 < -att);

  /////////////////////////////////////////////////////////
  // Tones at stop bands must be filtered out

  tone.init(spk, freq - 2 * trans, noise_size + 2 * len);
  slice.init(len, noise_size + len);
  conv.reset();

  level = calc_peak(&test_src);
  CHECK(level > 0);
  CHECK(log10(level) * 20 < -att);

  tone.init(spk, freq + 2 * trans, noise_size + 2 * len);
  slice.init(len, noise_size + len);
  conv.reset();

  level = calc_peak(&test_src);
  CHECK(level > 0);
  CHECK(log10(level) * 20 < -att);

  /////////////////////////////////////////////////////////
  // BandStop
  /////////////////////////////////////////////////////////

  conv.set_ir(&band_stop);
  len = 2 * band_stop.min_length(spk.sample_rate);

  /////////////////////////////////////////////////////////
  // Tones at pass bands must remain unchanged

  tone.init(spk, freq - 2 * trans, noise_size + 2 * len);
  slice.init(len, noise_size + len);
  ref_tone.init(spk, freq - 2 * trans, noise_size + 2 * len);
  ref_slice.init(len, len + noise_size);
  conv.reset();

  diff = calc_diff(&test_src, &ref_src);
  CHECK(diff > 0);
  CHECK(log10(diff) * 20 < -att);

  tone.init(spk, freq + 2 * trans, noise_size + 2 * len);
  slice.init(len, noise_size + len);
  ref_tone.init(spk, freq + 2 * trans, noise_size + 2 * len);
  ref_slice.init(len, len + noise_size);
  conv.reset();

  diff = calc_diff(&test_src, &ref_src);
  CHECK(diff > 0);
  CHECK(log10(diff) * 20 < -att);

  /////////////////////////////////////////////////////////
  // Tone in the stop band must be filtered out

  tone.init(spk, freq, noise_size + 2 * len);
  slice.init(len, noise_size + len);
  conv.reset();

  level = calc_peak(&test_src);
  CHECK(level > 0);
  CHECK(log10(level) * 20 < -att);

TEST_END(conv_param_old);

///////////////////////////////////////////////////////////////////////////////

SUITE(convolver, "Convolver filter test")
  TEST_FACTORY(conv_test),
  TEST_FACTORY(conv_param),
  TEST_FACTORY(conv_test_old),
  TEST_FACTORY(conv_param_old),
SUITE_END;
