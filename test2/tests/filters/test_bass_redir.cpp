/*
  BassRedir test
*/

#include <math.h>
#include <boost/test/unit_test.hpp>
#include "fir/param_fir.h"
#include "filters/bass_redir.h"
#include "filters/convolver.h"
#include "filters/filter_graph.h"
#include "filters/mixer.h"
#include "filters/slice.h"
#include "source/generator.h"
#include "../../suite.h"

static const int seed = 349857983;
static const size_t block_size = 65536;
static const int freq = 100;
static const int sample_rate = 48000;
static const Speakers spk_2_1(FORMAT_LINEAR, MODE_STEREO | CH_MASK_LFE, sample_rate);
static const Speakers spk_2_0(FORMAT_LINEAR, MODE_STEREO, sample_rate);
static const Speakers spk_0_1(FORMAT_LINEAR, CH_MASK_LFE, sample_rate);


BOOST_AUTO_TEST_SUITE(bass_redir)

BOOST_AUTO_TEST_CASE(constructor)
{
  BassRedir f;
  BOOST_CHECK_EQUAL(f.get_enabled(), false);
  BOOST_CHECK_EQUAL(f.get_level(), 0);
  BOOST_CHECK_EQUAL(f.get_freq(), 80);
  BOOST_CHECK_EQUAL(f.get_gain(), 1.0);
  BOOST_CHECK_EQUAL(f.get_channels(), CH_MASK_LFE);
}

BOOST_AUTO_TEST_CASE(get_set)
{
  BassRedir f;

  f.set_enabled(true);
  BOOST_CHECK_EQUAL(f.get_enabled(), true);
  f.set_enabled(false);
  BOOST_CHECK_EQUAL(f.get_enabled(), false);

  f.set_freq(100);
  BOOST_CHECK_EQUAL(f.get_freq(), 100);
  f.set_freq(200);
  BOOST_CHECK_EQUAL(f.get_freq(), 200);

  f.set_gain(2.0);
  BOOST_CHECK_EQUAL(f.get_gain(), 2.0);
  f.set_gain(1.0);
  BOOST_CHECK_EQUAL(f.get_gain(), 1.0);

  f.set_channels(CH_MASK_L | CH_MASK_R);
  BOOST_CHECK_EQUAL(f.get_channels(), CH_MASK_L | CH_MASK_R);
  f.set_channels(CH_MASK_LFE);
  BOOST_CHECK_EQUAL(f.get_channels(), CH_MASK_LFE);
}

BOOST_AUTO_TEST_CASE(is_active)
{
  BassRedir f;
  BOOST_CHECK_EQUAL(f.is_active(), false);

  f.open(Speakers(FORMAT_LINEAR, MODE_5_1, 48000));
  BOOST_CHECK_EQUAL(f.is_active(), false);

  f.set_enabled(true);
  BOOST_CHECK_EQUAL(f.is_active(), true);

  f.open(Speakers(FORMAT_LINEAR, MODE_QUADRO, 48000));
  BOOST_CHECK_EQUAL(f.is_active(), false);

  f.set_channels(MODE_STEREO);
  f.open(Speakers(FORMAT_LINEAR, MODE_STEREO, 48000));
  BOOST_CHECK_EQUAL(f.is_active(), false);

  f.open(Speakers(FORMAT_LINEAR, MODE_QUADRO, 48000));
  BOOST_CHECK_EQUAL(f.is_active(), true);
}

BOOST_AUTO_TEST_CASE(get_level)
{
  Chunk chunk;
  BassRedir f;

  // Get level in redirection mode
  f.set_enabled(true);
  f.set_freq(freq);
  f.open(spk_2_1);

  NoiseGen noise(spk_2_1, seed, block_size);
  while (noise.get_chunk(chunk))
    while (f.process(chunk, Chunk()))
    {}

  BOOST_CHECK_GT(f.get_level(), 0.001);

  // Get level in passthrough mode
  // Note that level must be dropped to zero
  // when filter is switched to passthrough mode.
  f.set_enabled(false);
  noise.init(spk_2_1, seed, block_size);
  while (noise.get_chunk(chunk))
    while (f.process(chunk, Chunk()))
    {}

  BOOST_CHECK_EQUAL(f.get_level(), 0);
}

BOOST_AUTO_TEST_CASE(filter_main_channels)
{
  // Bass must be filtered out from main channels.
  // 2.1 noise -> Bass redirection -> Low-pass filter -> Cut subwoofer
  // channel -> Find RMS (must be low).

  sample_t gains[CH_NAMES];

  ParamFIR low_pass(ParamFIR::low_pass, 3 * freq / 8, 0, freq / 4, 100);
  const FIRInstance *fir = low_pass.make(sample_rate);
  BOOST_REQUIRE(fir != 0);
  int trans_len = fir->length;
  delete fir;

  NoiseGen noise(spk_2_1, seed, block_size + trans_len * 2);
  BassRedir bass_redir;
  Convolver conv(&low_pass);
  SliceFilter slice(trans_len, block_size - trans_len);
  Mixer mixer(1024);

  bass_redir.set_enabled(true);
  bass_redir.set_freq(freq);

  mixer.set_output(spk_2_0);
  mixer.get_input_gains(gains);
  gains[CH_LFE] = 0;
  mixer.set_input_gains(gains);

  double rms = calc_rms(&noise, &FilterChain(&bass_redir, &conv, &slice, &mixer));
  BOOST_CHECK_GT(rms, 0.0);
  BOOST_CHECK_LT(rms, db2value(-48.0));
}

BOOST_AUTO_TEST_CASE(bass_at_subwoofer)
{
  // Subwoofer must be filled with bass.
  // 2.0 noise -> Add subwoofer -> Bass redirection -> Cut main channels ->
  // -> Find RMS (must be high).

  NoiseGen noise(spk_2_0, seed, block_size);
  BassRedir bass_redir;
  Mixer add_sub(1024);
  Mixer sub_only(1024);

  bass_redir.set_enabled(true);
  bass_redir.set_freq(freq);

  add_sub.set_output(spk_2_1);
  sub_only.set_output(spk_0_1);

  // RMS of the uniform nosie in range (-1..1) is sqrt(1/3)
  // RMS of the filtered signal must about rms_noise * sqrt(freq / nyquist) =
  // = sqrt(2 * freq / 3 * srate).
  // Note, that actual integral of LR4 low-pass filter does not equal to
  // freq/nyquist, therefore 20% threshold is used.
  double rms = calc_rms(&noise, &FilterChain(&add_sub, &bass_redir, &sub_only));
  double test_rms = sqrt(double(2 * freq) / (3 * sample_rate));
  BOOST_CHECK_CLOSE(rms, test_rms, 20);
}

BOOST_AUTO_TEST_CASE(equal_power)
{
  // Resulting signal power must be equal to the original power.
  // 2.0 noise -> Add subwoofer -> Bass redirection -> Find RMS
  // RMS of the uniform nosie in range (-1..1) is sqrt(1/3).
  // Original signal has 2 channels, but result has 3 channels. Therefore,
  // resulting RMS equals to original * sqrt(2/3) = sqrt(1/3) * sqrt(2/3)

  NoiseGen noise(spk_2_0, seed, block_size);
  BassRedir bass_redir;
  Mixer add_sub(1024);

  bass_redir.set_enabled(true);
  bass_redir.set_freq(freq);

  add_sub.set_output(spk_2_1);

  double rms = calc_rms(&noise, &FilterChain(&add_sub, &bass_redir));
  double test_rms = sqrt(2.0/9);
  BOOST_CHECK_CLOSE(rms, test_rms, 0.5);
}

BOOST_AUTO_TEST_SUITE_END()
