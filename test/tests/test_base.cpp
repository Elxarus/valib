/*
  Base filter classes test
  (classes defined at filter.h)

  * Null filter must pass all data through
  * SourceFilter must act as a combination of a source and a filter
  * SinkFilter must act as a combination of a filter and a sink
*/

#include <math.h>
#include "sink.h"
#include "source/source_filter.h"
#include "source/generator.h"
#include "filters/passthrough.h"
#include "sink/sink_filter.h"
#include "sink/sink_null.h"
#include "../suite.h"

static const int seed = 4796;
static const int noise_size = 64 * 1024;


// Filter zeroes all the data

class ZeroFilter : public SimpleFilter
{
public:
  ZeroFilter()
  {}

  /////////////////////////////////////////////////////////
  // SimpleFilter overrides

  virtual bool can_open(Speakers spk) const
  { return true; }

  virtual bool process(Chunk &in, Chunk &out)
  {
    if (spk.is_linear())
      zero_samples(in.samples, spk.nch(), in.size);
    else
      memset(in.rawdata, 0, in.size);

    out = in;
    in.clear();
    return !out.is_dummy();
  }
};

// Sink accepts only zeroes and fails otherwise

class ZeroSink : public SimpleSink
{
public:
  bool zero;

public:
  ZeroSink():
  zero(true)
  {}

  void reset()
  { zero = true; }

  bool is_zero() const
  { return zero; }

  /////////////////////////////////////////////////////////
  // Sink interface

  virtual bool can_open(Speakers new_spk) const
  { return true; }

  virtual void process(const Chunk &chunk)
  {
    if (!zero)
      return;

    if (spk.is_linear())
    {
      for (int ch = 0; ch < spk.nch(); ch++)
        for (size_t i = 0; i < chunk.size; i++)
          if (!EQUAL_SAMPLES(chunk.samples[ch][i], 0.0))
            zero = false;
    }
    else
    {
      for (size_t i = 0; i < chunk.size; i++)
        if (chunk.rawdata[i] != 0)
          zero = false;
    }
  }
};


///////////////////////////////////////////////////////////////////////////////
// Passthrough test
// Passthrough filter must pass all data through
///////////////////////////////////////////////////////////////////////////////

TEST(base_passthrough, "Passthrough")
  Speakers spk;
  NoiseGen src_noise;
  NoiseGen ref_noise;
  Passthrough pass_filter;

  // Linear format test

  spk = Speakers(FORMAT_LINEAR, MODE_STEREO, 48000);
  src_noise.init(spk, seed, noise_size);
  ref_noise.init(spk, seed, noise_size);
  pass_filter.reset();

  CHECK(compare(log, &src_noise, &pass_filter, &ref_noise, 0) == 0);

  // Rawdata format test

  spk = Speakers(FORMAT_PCM16, MODE_STEREO, 48000);
  src_noise.init(spk, seed, noise_size);
  ref_noise.init(spk, seed, noise_size);
  pass_filter.reset();

  CHECK(compare(log, &src_noise, &pass_filter, &ref_noise, 0) == 0);

TEST_END(base_passthrough);

///////////////////////////////////////////////////////////////////////////////
// SourceFilter test
// SourceFilter must act as a combination of a source and a filter
///////////////////////////////////////////////////////////////////////////////

TEST(base_source_filter, "SourceFilter")

  Speakers spk(FORMAT_LINEAR, MODE_STEREO, 48000);
  
  Passthrough pass_filter;
  ZeroFilter  zero_filter;

  NoiseGen src_noise;
  NoiseGen ref_noise;
  ZeroGen  ref_zero;

  // Init constructor test

  SourceFilter src_filter(&src_noise, &pass_filter);
  CHECK(src_filter.get_source() == &src_noise);
  CHECK(src_filter.get_filter() == &pass_filter);

  // Init test

  src_filter.set(&ref_zero, &zero_filter);
  CHECK(src_filter.get_source() == &ref_zero);
  CHECK(src_filter.get_filter() == &zero_filter);

  // Noise source == Noise source (no filter)

  src_noise.init(spk, seed, noise_size);
  ref_noise.init(spk, seed, noise_size);

  src_filter.set(&src_noise, 0);
  CHECK(compare(log, &src_filter, &ref_noise) == 0);

  // Noise source + Passthrough == Noise source

  src_noise.init(spk, seed, noise_size);
  ref_noise.init(spk, seed, noise_size);
  pass_filter.reset();

  src_filter.set(&src_noise, &pass_filter);
  CHECK(compare(log, &src_filter, &ref_noise) == 0);

  // Noise source + ZeroFilter == Zero source

  src_noise.init(spk, seed, noise_size);
  ref_zero.init(spk, noise_size);
  zero_filter.reset();

  src_filter.set(&src_noise, &zero_filter);
  CHECK(compare(log, &src_filter, &ref_zero) == 0);
  
TEST_END(base_source_filter);

///////////////////////////////////////////////////////////////////////////////
// SinkFilter test
// SinkFilter must act as a combination of a filter and a sink
///////////////////////////////////////////////////////////////////////////////

TEST(base_sink_filter, "SinkFilter")
  Speakers spk(FORMAT_LINEAR, MODE_STEREO, 48000);
  
  Passthrough pass_filter;
  ZeroFilter  zero_filter;
  NullSink    null_sink;
  ZeroSink    zero_sink;

  NoiseGen src_noise(spk, seed, noise_size);
  ZeroGen  src_zero(spk, noise_size);

  Chunk zero_chunk;
  Chunk noise_chunk;

  // Init constructor test

  SinkFilter sink_filter(&null_sink, &pass_filter);
  CHECK(sink_filter.get_sink() == &null_sink);
  CHECK(sink_filter.get_filter() == &pass_filter);

  // Init test

  sink_filter.set(&zero_sink, &zero_filter);
  CHECK(sink_filter.get_sink() == &zero_sink);
  CHECK(sink_filter.get_filter() == &zero_filter);

  // Fail without a sink

  CHECK(sink_filter.set(0, 0) == false);

  // Zero source == zero (no filter)
  // Noise source != zero (no filter)

  src_zero.get_chunk(zero_chunk);
  src_noise.get_chunk(noise_chunk);
  zero_sink.reset();

  CHECK(sink_filter.set(&zero_sink, 0));
  CHECK(sink_filter.open(spk));

  sink_filter.process(zero_chunk);
  CHECK(zero_sink.is_zero());

  sink_filter.process(noise_chunk);
  CHECK(!zero_sink.is_zero());

  // Zero source + NullFilter == zero
  // Noise source + NullFilter != zero

  src_zero.get_chunk(zero_chunk);
  src_noise.get_chunk(noise_chunk);
  zero_sink.reset();

  CHECK(sink_filter.set(&zero_sink, &pass_filter));
  CHECK(sink_filter.open(spk));

  sink_filter.process(zero_chunk);
  CHECK(zero_sink.is_zero());

  sink_filter.process(noise_chunk);
  CHECK(!zero_sink.is_zero());

  // Zero source + ZeroFilter == zero
  // Noise source + ZeroFilter == zero

  src_zero.get_chunk(zero_chunk);
  src_noise.get_chunk(noise_chunk);
  zero_sink.reset();

  CHECK(sink_filter.set(&zero_sink, &zero_filter));
  CHECK(sink_filter.open(spk));

  sink_filter.process(zero_chunk);
  CHECK(zero_sink.is_zero());

  sink_filter.process(noise_chunk);
  CHECK(zero_sink.is_zero());

TEST_END(base_sink_filter);

///////////////////////////////////////////////////////////////////////////////
// Test suite
///////////////////////////////////////////////////////////////////////////////

SUITE(base, "Base filter classes test")
  TEST_FACTORY(base_passthrough),
  TEST_FACTORY(base_source_filter),
  TEST_FACTORY(base_sink_filter),
SUITE_END;
