/*
  Sample rate conversion test (StreamingSRC, BufferSRC)
*/

#include <boost/test/unit_test.hpp>
#include "source/generator.h"
#include "source/chunk_source.h"
#include "filters/filter_graph.h"
#include "filters/convolver.h"
#include "filters/slice.h"
#include "fir/param_fir.h"
#include "dsp/src.h"
#include "../../suite.h"

static const int seed = 9423785;
static const size_t noise_size = 65536;

static SRCParams params1(44100, 48000, 100, 0.9);
static SRCParams params2(48000, 96000, 120, 0.99);
static SRCParams bad_list[] =
{
  SRCParams(44100, 44100, 100, 0.9),  // fs = fd
  SRCParams(0, 48000, 100, 0.9),      // fs = 0
  SRCParams(-100, 48000, 100, 0.9),   // incorrect fs
  SRCParams(44100, 0, 100, 0.9),      // fd = 0
  SRCParams(44100, -100, 100, 0.9),   // incorrect fd
  SRCParams(0, 0, 100, 0.9),          // fs = fd = 0
  SRCParams(44100, 48000, 3, 0.9),    // attenuation is too low
  SRCParams(44100, 48000, 500, 0.9),  // attenuation is too high
  SRCParams(44100, 48000, 100, 0.01), // quality is too low
  SRCParams(44100, 48000, 100, 1.0 - 1e-12), // quality is too high
  SRCParams(44100, 48000, 100, 1.0),  // bad quality
  SRCParams(44100, 48000, 100, 2.0),  // bad quality
};

static const int transform_rates[][2] =
{
  {  64000,  4000 }, // 16/1




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

static bool operator ==(const SRCParams &a, const SRCParams &b)
{
  return a.fs == b.fs && a.fd == b.fd && a.a == b.a && a.q == b.q;
}

static size_t resample(StreamingSRC &src, const sample_t *in, size_t in_size, sample_t *out, size_t out_size)
{
  size_t result_size = 0;
  size_t gone = 0;
  while (gone < in_size)
  {
    gone += src.fill(in + gone, in_size - gone);
    if (src.can_process())
    {
      src.process();
      if (result_size + src.size() > out_size)
        BOOST_FAIL("Not enough buffer size");
      copy_samples(out, result_size, src.result(), 0, src.size());
      result_size += src.size();
    }
  }

  while (src.need_flushing())
  {
    src.flush();
    if (result_size + src.size() > out_size)
      BOOST_FAIL("Not enough buffer size");
    copy_samples(out, result_size, src.result(), 0, src.size());
    result_size += src.size();
  }

  return result_size;
}

static sample_t diff_resampled(const SRCParams &params, sample_t *buf1, sample_t *buf2, size_t size)
{
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

  ParamFIR low_pass(ParamFIR::low_pass, params.q-0.5, 0, 0.5*(1-params.q), params.a + 10, true);

  const FIRInstance *fir = low_pass.make(params.fs);
  BOOST_REQUIRE(fir != 0);
  int trans_len = fir->length * 2;
  delete fir;

  Speakers spk(FORMAT_LINEAR, MODE_MONO, params.fs);
  samples_t s1, s2;
  s1.zero(); s1[0] = buf1;
  s2.zero(); s2[0] = buf2;

  ChunkSource src1(spk, Chunk(s1, size));
  ChunkSource src2(spk, Chunk(s2, size));
  Convolver   conv1(&low_pass);
  Convolver   conv2(&low_pass);
  SliceFilter slice1(trans_len, size - trans_len);
  SliceFilter slice2(trans_len, size - trans_len);
  FilterChain chain1(&conv1, &slice1);
  FilterChain chain2(&conv2, &slice2);
  return calc_diff(&src1, &chain1, &src2, &chain2);
}

static void streaming_up_down_test(const SRCParams &params)
{
  Chunk chunk;
  NoiseGen noise_gen(Speakers(FORMAT_LINEAR, MODE_MONO, params.fs), seed, noise_size, noise_size);
  noise_gen.get_chunk(chunk);
  sample_t *noise = chunk.samples[0];
  size_t noise_size = chunk.size;

  // Upsampled buffer
  Samples buf1(size_t(double(noise_size + 1) * params.fd / params.fs) + 1);
  // Downsampled buffer
  Samples buf2(noise_size + 100);

  StreamingSRC src;

  BOOST_REQUIRE(src.open(params));
  size_t buf1_data = resample(src, noise, noise_size, buf1, buf1.size());

  BOOST_REQUIRE(src.open(SRCParams(params.fd, params.fs, params.a, params.q)));
  size_t buf2_data = resample(src, buf1, buf1_data, buf2, buf2.size());

  BOOST_CHECK(abs(int(buf2_data) - int(noise_size)) <= 1);

  // Resample introduces not more than -A dB of noise.
  // 2 resamples introduces twice more noise, -A + 6dB
  sample_t diff = diff_resampled(params, noise, buf2, MIN(noise_size, buf2_data));
  BOOST_MESSAGE("Transform: " << params.fs << "Hz <-> " << params.fd << "Hz Diff: " << value2db(diff) << " dB");
  BOOST_CHECK_LE(value2db(diff), -params.a + 7);
}

static void equivalence_test(const SRCParams &params)
{
  Chunk chunk;
  NoiseGen noise_gen(Speakers(FORMAT_LINEAR, MODE_MONO, params.fs), seed, noise_size, noise_size);
  noise_gen.get_chunk(chunk);
  sample_t *noise = chunk.samples[0];
  size_t noise_size = chunk.size;

  // Upsampled buffer
  Samples buf1(size_t(double(noise_size + 1) * params.fd / params.fs) + 1);
  // Downsampled buffer
  Samples buf2(noise_size + 100);

  StreamingSRC src;
  BufferSRC test;
  BOOST_MESSAGE("Transform: " << params.fs << "Hz <-> " << params.fd);

  /////////////////////////////////////////////////////////
  // Resample using StreamingSRC

  BOOST_REQUIRE(src.open(params));
  size_t buf1_data = resample(src, noise, noise_size, buf1, buf1.size());

  BOOST_REQUIRE(src.open(SRCParams(params.fd, params.fs, params.a, params.q)));
  size_t buf2_data = resample(src, buf1, buf1_data, buf2, buf2.size());

  /////////////////////////////////////////////////////////
  // Resample using BufferSRC and check the difference

  BOOST_REQUIRE(test.open(params));
  test.process(noise, noise_size);
  BOOST_REQUIRE_EQUAL(test.size(), buf1_data);
  sample_t diff = peak_diff(test.result(), buf1, buf1_data);
  BOOST_CHECK_LE(diff, SAMPLE_THRESHOLD);

  BOOST_REQUIRE(test.open(SRCParams(params.fd, params.fs, params.a, params.q)));
  test.process(buf1, buf1_data);
  BOOST_REQUIRE_EQUAL(test.size(), buf2_data);
  diff = peak_diff(test.result(), buf2, buf2_data);
  BOOST_CHECK_LE(diff, SAMPLE_THRESHOLD);
}

///////////////////////////////////////////////////////////////////////////////

BOOST_AUTO_TEST_SUITE(src)

BOOST_AUTO_TEST_SUITE(streaming_src)

BOOST_AUTO_TEST_CASE(default_constructor)
{
  StreamingSRC src;
  BOOST_CHECK(!src.is_open());
  BOOST_CHECK(!src.can_process());
  BOOST_CHECK(!src.need_flushing());
  BOOST_CHECK(src.result() == 0);
  BOOST_CHECK(src.size() == 0);
}

BOOST_AUTO_TEST_CASE(init_constructor1)
{
  StreamingSRC src(params1);
  BOOST_CHECK(src.is_open());
  BOOST_CHECK(!src.can_process());
  BOOST_CHECK(!src.need_flushing());
  BOOST_CHECK(src.result() == 0);
  BOOST_CHECK(src.size() == 0);
  BOOST_CHECK(src.params() == params1);
}

BOOST_AUTO_TEST_CASE(init_constructor2)
{
  StreamingSRC src(params1.fs, params1.fd, params1.a, params1.q);
  BOOST_CHECK(src.is_open());
  BOOST_CHECK(!src.can_process());
  BOOST_CHECK(!src.need_flushing());
  BOOST_CHECK(src.result() == 0);
  BOOST_CHECK(src.size() == 0);
  BOOST_CHECK(src.params() == params1);
}

BOOST_AUTO_TEST_CASE(open_close)
{
  StreamingSRC src;

  BOOST_CHECK(src.open(params1));
  BOOST_CHECK(src.is_open());
  BOOST_CHECK(!src.can_process());
  BOOST_CHECK(!src.need_flushing());
  BOOST_CHECK(src.result() == 0);
  BOOST_CHECK(src.size() == 0);
  BOOST_CHECK(src.params() == params1);

  BOOST_CHECK(src.open(params2.fs, params2.fd, params2.a, params2.q));
  BOOST_CHECK(src.is_open());
  BOOST_CHECK(src.params() == params2);

  src.close();
  BOOST_CHECK(!src.is_open());
  BOOST_CHECK(!src.is_open());
  BOOST_CHECK(!src.can_process());
  BOOST_CHECK(!src.need_flushing());
  BOOST_CHECK(src.result() == 0);
  BOOST_CHECK(src.size() == 0);
}

BOOST_AUTO_TEST_CASE(open_bad)
{
  StreamingSRC src;
  for (size_t i = 0; i < array_size(bad_list); i++)
    BOOST_CHECK(!src.open(bad_list[i]));
}

BOOST_AUTO_TEST_CASE(up_down)
{
  ///////////////////////////////////////////////////////////////////////////////
  // Resample reverse transform test
  // Resample is reversible when:
  // * we have no frequencies above the pass band
  // * we throw out transient processes at stream ends

  for (size_t i = 0; i < array_size(transform_rates); i++)
    streaming_up_down_test(SRCParams(transform_rates[i][1], transform_rates[i][0], 106, 0.99));
}

BOOST_AUTO_TEST_SUITE_END()

///////////////////////////////////////////////////////////////////////////////

BOOST_AUTO_TEST_SUITE(buffer_src)

BOOST_AUTO_TEST_CASE(default_constructor)
{
  BufferSRC src;
  BOOST_CHECK(!src.is_open());
  BOOST_CHECK(src.result() == 0);
  BOOST_CHECK(src.size() == 0);
}

BOOST_AUTO_TEST_CASE(init_constructor1)
{
  BufferSRC src(params1);
  BOOST_CHECK(src.is_open());
  BOOST_CHECK(src.result() == 0);
  BOOST_CHECK(src.size() == 0);
  BOOST_CHECK(src.params() == params1);
}

BOOST_AUTO_TEST_CASE(init_constructor2)
{
  BufferSRC src(params1.fs, params1.fd, params1.a, params1.q);
  BOOST_CHECK(src.is_open());
  BOOST_CHECK(src.result() == 0);
  BOOST_CHECK(src.size() == 0);
  BOOST_CHECK(src.params() == params1);
}

BOOST_AUTO_TEST_CASE(open_close)
{
  BufferSRC src;

  BOOST_CHECK(src.open(params1));
  BOOST_CHECK(src.is_open());
  BOOST_CHECK(src.result() == 0);
  BOOST_CHECK(src.size() == 0);
  BOOST_CHECK(src.params() == params1);

  BOOST_CHECK(src.open(params2.fs, params2.fd, params2.a, params2.q));
  BOOST_CHECK(src.is_open());
  BOOST_CHECK(src.params() == params2);

  src.close();
  BOOST_CHECK(!src.is_open());
  BOOST_CHECK(!src.is_open());
  BOOST_CHECK(src.result() == 0);
  BOOST_CHECK(src.size() == 0);
}

BOOST_AUTO_TEST_CASE(open_bad)
{
  BufferSRC src;
  for (size_t i = 0; i < array_size(bad_list); i++)
    BOOST_CHECK(!src.open(bad_list[i]));
}

BOOST_AUTO_TEST_CASE(equivalence)
{
  // BufferSRC result must be equavalent to StreamingSRC
  for (size_t i = 0; i < array_size(transform_rates); i++)
    equivalence_test(SRCParams(transform_rates[i][1], transform_rates[i][0]));
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
