/*
  Resample test
*/

#include <boost/test/unit_test.hpp>
#include "fir/param_fir.h"
#include "filters/convolver.h"
#include "filters/filter_graph.h"
#include "filters/resample.h"
#include "filters/slice.h"
#include "source/generator.h"
#include "../../suite.h"

static const int seed = 894756987;
static const size_t block_size = 65536;

static const int transform_rates[][2] =
{
  { 192000, 11025 }, // 2560/147
  { 192000,  4000 }, // 48/1

  { 176400,  6000 }, // 147/5
  { 176400,  4000 }, // 441/10

  { 128000, 11025 }, // 5120/441
  { 128000,  6000 }, // 64/3
  { 128000,  4000 }, // 32/1

  {  96000, 11025 }, // 1280/147
  {  96000,  4000 }, // 24/1

  {  88200,  6000 }, // 147/10
  {  88200,  4000 }, // 441/20

  {  64000, 11025 }, // 2560/441
  {  64000,  6000 }, // 32/3
  {  64000,  4000 }, // 16/1

  {  48000, 44100 }, // 160/147
  {  48000, 32000 }, // 3/2
  {  48000, 24000 }, // 2/1
  {  48000, 22050 }, // 320/147
  {  48000, 16000 }, // 3/1
  {  48000, 12000 }, // 4/1
  {  48000, 11025 }, // 640/147
  {  48000,  8000 }, // 6/1
  {  48000,  6000 }, // 8/1
  {  48000,  4000 }, // 12/1

  {  44100, 32000 }, // 441/320
  {  44100, 24000 }, // 147/80
  {  44100, 16000 }, // 441/160
  {  44100, 12000 }, // 147/40
  {  44100,  8000 }, // 441/80
  {  44100,  6000 }, // 147/20
  {  44100,  4000 }, // 441/40

  {  32000, 24000 }, // 4/3
  {  32000, 22050 }, // 640/441
  {  32000, 12000 }, // 8/3
  {  32000, 11025 }, // 1280/441
  {  32000,  6000 }, // 16/3
};

void up_down(int rate1, int rate2, double a, double q)
{
  if (rate1 > rate2)
  {
    int temp = rate1;
    rate1 = rate2;
    rate2 = temp;
  }

  /////////////////////////////////////////////////////////////////////////////
  // After resample only q*nyquist of the bandwidth is preserved. Therefore,
  // to compare output of the resampler with the original signal we must feed
  // the resampler with the bandlimited signal. Bandlimiting filter has a
  // transition band and we must guarantee:
  // passband + transition_band <= q*nyquist.
  //
  // It looks reasonable to make the transition band of the bandlimiting filter
  // to be equal to the transition band of the resampler. In this case we have:
  // passband + (1-q)*nyquist <= q*nyquist
  // passband <= (2q - 1)*nyquist
  //
  // In normalized form nyquist = 0.5, so we have following parameters of the
  // bandlimiting filter: passband = q-0.5, transition band = 0.5*(1-q)

  ParamFIR low_pass(ParamFIR::low_pass, q-0.5, 0, 0.5*(1-q), a + 10, true);
  const FIRInstance *fir = low_pass.make(rate1);
  BOOST_REQUIRE(fir != 0);
  int trans_len = fir->length * 2;
  delete fir;

  Speakers spk(FORMAT_LINEAR, MODE_STEREO, rate1);
  NoiseGen tst_noise(spk, seed, block_size + trans_len * 2);
  NoiseGen ref_noise(spk, seed, block_size + trans_len * 2);
  Convolver tst_conv(&low_pass);
  Convolver ref_conv(&low_pass);
  Resample res1(rate2, a, q);
  Resample res2(rate1, a, q);
  SliceFilter tst_slice(trans_len, block_size - trans_len);
  SliceFilter ref_slice(trans_len, block_size - trans_len);

  FilterChain tst_chain(&tst_conv, &res1, &res2, &tst_slice);
  FilterChain ref_chain(&ref_conv, &ref_slice);

  // Resample introduces not more than -A dB of noise.
  // 2 resamples introduces twice more noise, -A + 6dB
  double diff = calc_diff(&tst_noise, &tst_chain, &ref_noise, &ref_chain);
  BOOST_MESSAGE("Transform: " << rate1 << "Hz -> " << rate2 << "Hz Diff: " << value2db(diff));
  BOOST_CHECK_GT(diff, 0);
  BOOST_CHECK_LE(value2db(diff),  -a + 7);
}



BOOST_AUTO_TEST_SUITE(resample)

BOOST_AUTO_TEST_CASE(constructor)
{
  Resample f;
  BOOST_CHECK_EQUAL(f.get_sample_rate(), 0);
  BOOST_CHECK_EQUAL(f.get_attenuation(), 100);
  BOOST_CHECK_EQUAL(f.get_quality(), 0.99);
}

BOOST_AUTO_TEST_CASE(init_constructor)
{
  Resample f(44100, 120, 0.999);
  BOOST_CHECK_EQUAL(f.get_sample_rate(), 44100);
  BOOST_CHECK_EQUAL(f.get_attenuation(), 120);
  BOOST_CHECK_EQUAL(f.get_quality(), 0.999);
}

BOOST_AUTO_TEST_CASE(get_set)
{
  Resample f;

  int sample_rate;
  double a, q;

  f.set(44100, 120, 0.999);
  f.get(&sample_rate, &a, &q);
  BOOST_CHECK_EQUAL(sample_rate, 44100);
  BOOST_CHECK_EQUAL(a, 120);
  BOOST_CHECK_EQUAL(q, 0.999);

  // Do not crash
  f.get(0, 0, 0);

  f.set_sample_rate(48000);
  BOOST_CHECK_EQUAL(f.get_sample_rate(), 48000);

  f.set_attenuation(100);
  BOOST_CHECK_EQUAL(f.get_attenuation(), 100);

  f.set_quality(0.98);
  BOOST_CHECK_EQUAL(f.get_quality(), 0.98);
}

BOOST_AUTO_TEST_CASE(passthrough)
{
  Speakers spk(FORMAT_LINEAR, MODE_STEREO, 48000);
  Resample f;

  NoiseGen noise(spk, seed, block_size);
  NoiseGen ref(spk, seed, block_size);

  // Passthrough without sample rate set
  f.open(spk);
  BOOST_REQUIRE(f.is_open());
  BOOST_CHECK(f.passthrough());
  compare(&noise, &f, &ref);

  // Passthrough mode with sample rate set
  f.set_sample_rate(spk.sample_rate);
  f.open(spk);
  noise.reset();
  ref.reset();
  BOOST_REQUIRE(f.is_open());
  BOOST_CHECK(f.passthrough());
  compare(&noise, &f, &ref);
}

///////////////////////////////////////////////////////////////////////////////
// Resample reverse transform test
// Resample is reversible when:
// * we have no frequencies above the pass band
// * we throw out transient processes at stream ends

BOOST_AUTO_TEST_CASE(resample_reverse)
{
  const double q = 0.99; // quality
  const double a = 106;  // attenuation

  // upsample -> downsample
  for (int i = 0; i < array_size(transform_rates); i++)
    up_down(transform_rates[i][0], transform_rates[i][1], a, q);
}

BOOST_AUTO_TEST_SUITE_END()
