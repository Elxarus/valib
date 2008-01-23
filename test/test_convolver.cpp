#include <math.h>
#include "suite.h"
#include "source/generator.h"
#include "filters/convolver.h"
#include "fir/param_ir.h"

static const Speakers spk = Speakers(FORMAT_LINEAR, MODE_STEREO, 48000);
static const size_t noise_size = 64 * 1024;
static const int seed = 123123;

///////////////////////////////////////////////////////////////////////////////
// SliceFilter
// This filter cuts a middle of the input stream. It is required for convolver
// test because transient processes at the start and the end of the stream.

class SliceFilter : public NullFilter
{
protected:
  size_t pos;
  size_t start;
  size_t end;

public:
  SliceFilter(size_t _start = 0, size_t _end = 0);
  void init(size_t _start, size_t _end);
  void reset();
  bool process(const Chunk *_chunk);
};

///////////////////////////////////////////////////////////////////////////////

TEST(conv_zero, "Convolve with zero response")
  ZeroIR zero_ir;
  Convolver conv(&zero_ir);

  NoiseGen noise_src(spk, seed, noise_size);
  ZeroGen zero_src(spk, noise_size);

  CHECK(compare(log, &noise_src, &conv, &zero_src, 0) == 0);
TEST_END(conv_zero);

///////////////////////////////////////////////////////////////////////////////

TEST(conv_identity, "Convolve with identity response")
  IdentityIR identity_ir;
  Convolver conv(&identity_ir);

  NoiseGen noise1(spk, seed, noise_size);
  NoiseGen noise2(spk, seed, noise_size);

  CHECK(compare(log, &noise1, &conv, &noise2, 0) == 0);
TEST_END(conv_identity);

///////////////////////////////////////////////////////////////////////////////

TEST(conv_slice, "SliceFilter test")

  int chunk_size = noise_size / 11;
  Chunk chunk;
  NoiseGen noise1;
  NoiseGen noise2;
  SliceFilter slice;

  /////////////////////////////////////////////////////////
  // Slice is equal to the stream

  noise1.init(spk, seed, noise_size, chunk_size);
  noise2.init(spk, seed, noise_size, chunk_size);
  slice.init(0, noise_size);

  CHECK(compare(log, &noise1, &slice, &noise2, 0) == 0);

  /////////////////////////////////////////////////////////
  // Slice is shorter than the stream

  noise1.init(spk, seed, noise_size, chunk_size);
  noise2.init(spk, seed, noise_size / 2, chunk_size);
  slice.init(0, noise_size / 2);

  CHECK(compare(log, &noise1, &slice, &noise2, 0) == 0);

  /////////////////////////////////////////////////////////
  // Slice is longer than the stream

  noise1.init(spk, seed, noise_size, chunk_size);
  noise2.init(spk, seed, noise_size, chunk_size);
  slice.init(0, 2 * noise_size);

  CHECK(compare(log, &noise1, &slice, &noise2, 0) == 0);

  /////////////////////////////////////////////////////////
  // Cut the end of the stream

  noise1.init(spk, seed, noise_size, chunk_size);
  noise2.init(spk, seed, noise_size, chunk_size);
  noise2.get_chunk(&chunk);
  slice.init(chunk_size, noise_size);

  CHECK(compare(log, &noise1, &slice, &noise2, 0) == 0);

  /////////////////////////////////////////////////////////
  // Cut the middle of the stream

  noise1.init(spk, seed, noise_size, chunk_size);
  noise2.init(spk, seed, noise_size / 2, chunk_size);
  noise2.get_chunk(&chunk);
  slice.init(chunk_size, noise_size / 2);

  CHECK(compare(log, &noise1, &slice, &noise2, 0) == 0);

  /////////////////////////////////////////////////////////
  // Cut after the end of the stream 1

  noise1.init(spk, seed, noise_size, chunk_size);
  noise2.init(spk, seed, noise_size, chunk_size);
  noise2.get_chunk(&chunk);
  slice.init(chunk_size, 2 * noise_size);

  CHECK(compare(log, &noise1, &slice, &noise2, 0) == 0);

  /////////////////////////////////////////////////////////
  // Cut after the end of the stream 2

  noise1.init(spk, seed, noise_size, chunk_size);
  noise2.init(spk, seed, 0, chunk_size);
  slice.init(2 * noise_size, 4 * noise_size);

  CHECK(compare(log, &noise1, &slice, &noise2, 0) == 0);

  /////////////////////////////////////////////////////////
  // Cut nothing at the beginning

  noise1.init(spk, seed, noise_size, chunk_size);
  noise2.init(spk, seed, 0, chunk_size);
  slice.init(0, 0);

  CHECK(compare(log, &noise1, &slice, &noise2, 0) == 0);

  /////////////////////////////////////////////////////////
  // Cut nothing in the middle

  noise1.init(spk, seed, noise_size, chunk_size);
  noise2.init(spk, seed, 0, chunk_size);
  slice.init(noise_size / 2, noise_size / 2);

  CHECK(compare(log, &noise1, &slice, &noise2, 0) == 0);

  /////////////////////////////////////////////////////////
  // Cut nothing at the end

  noise1.init(spk, seed, noise_size, chunk_size);
  noise2.init(spk, seed, 0, chunk_size);
  slice.init(noise_size, noise_size);

  CHECK(compare(log, &noise1, &slice, &noise2, 0) == 0);

  /////////////////////////////////////////////////////////
  // Cut nothing after the end

  noise1.init(spk, seed, noise_size, chunk_size);
  noise2.init(spk, seed, 0, chunk_size);
  slice.init(2 * noise_size, 2 * noise_size);

  CHECK(compare(log, &noise1, &slice, &noise2, 0) == 0);

TEST_END(conv_slice);

///////////////////////////////////////////////////////////////////////////////

TEST(conv_param, "Convolve with parametric filter")
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

TEST_END(conv_param);

///////////////////////////////////////////////////////////////////////////////

SliceFilter::SliceFilter(size_t _start, size_t _end): NullFilter(-1), pos(0), start(_start), end(_end) 
{
  assert(start <= end);
}

void
SliceFilter::init(size_t _start, size_t _end)
{
  reset();
  start = _start;
  end = _end;
  assert(start <= end);
}

void
SliceFilter::reset()
{
  NullFilter::reset();
  pos = 0;
}

bool
SliceFilter::process(const Chunk *_chunk)
{
  // ignore dummy chunks
  if (_chunk->is_dummy())
    return true;

  receive_chunk(_chunk);

  // ignore everything after the end (except eos)

  if (pos >= end)
  {
    size = 0;
    return true;
  }

  // ignore everything before the beginning (except eos)
  if (pos + _chunk->size <= start)
  {
    pos += size;
    size = 0;
    return true;
  }

  // cut off the tail
  if (pos + size > end)
    size = end - pos;

  // cut of the head
  if (pos < start)
  {
    if (spk.is_linear())
      drop_samples(start - pos);
    else
      drop_rawdata(start - pos);
    pos = start;
  }

  pos += size;
  return true;
}

///////////////////////////////////////////////////////////////////////////////

SUITE(convolver, "Convolver filter test")
  TEST_FACTORY(conv_zero),
  TEST_FACTORY(conv_identity),
  TEST_FACTORY(conv_slice),
  TEST_FACTORY(conv_param),
SUITE_END;
