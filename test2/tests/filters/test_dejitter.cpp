/*
  Dejitter class test
*/

#include <boost/test/unit_test.hpp>
#include "rng.h"
#include "filters/dejitter.h"
#include "source/generator.h"
#include "../../noise_buf.h"

static const int seed = 9823475;
static const size_t chunk_size = 128;
static const size_t nchunks = 1000;
static const vtime_t jitter = 0.03;

static inline bool compare_time(vtime_t t1, vtime_t t2)
{
  return fabs(t1 - t2) < 1e-6;
}

BOOST_AUTO_TEST_SUITE(dejitter)

BOOST_AUTO_TEST_CASE(constructor)
{
  Dejitter f;
  BOOST_CHECK_EQUAL(f.get_time_shift(), 0);
  BOOST_CHECK_EQUAL(f.get_time_factor(), 1.0);
  BOOST_CHECK_EQUAL(f.get_dejitter(), true);
  BOOST_CHECK_EQUAL(f.get_threshold(), 0.1);
  BOOST_CHECK_EQUAL(f.get_input_mean(), 0);
  BOOST_CHECK_EQUAL(f.get_input_stddev(), 0);
  BOOST_CHECK_EQUAL(f.get_output_mean(), 0);
  BOOST_CHECK_EQUAL(f.get_output_stddev(), 0);
}

BOOST_AUTO_TEST_CASE(get_set)
{
  Dejitter f;
  f.set_time_shift(1.1);
  BOOST_CHECK_EQUAL(f.get_time_shift(), 1.1);
  f.set_time_shift(0);
  BOOST_CHECK_EQUAL(f.get_time_shift(), 0);

  f.set_time_factor(1.5);
  BOOST_CHECK_EQUAL(f.get_time_factor(), 1.5);
  f.set_time_shift(1.0);
  BOOST_CHECK_EQUAL(f.get_time_shift(), 1.0);

  f.set_dejitter(false);
  BOOST_CHECK_EQUAL(f.get_dejitter(), false);
  f.set_dejitter(true);
  BOOST_CHECK_EQUAL(f.get_dejitter(), true);

  f.set_threshold(0.5);
  BOOST_CHECK_EQUAL(f.get_threshold(), 0.5);
  f.set_threshold(1.0);
  BOOST_CHECK_EQUAL(f.get_threshold(), 1.0);
}

BOOST_AUTO_TEST_CASE(dejitter)
{
  // Process chunks with time jitter
  // Get resulting time without jitter

  RNG rng;
  Speakers spk(FORMAT_PCM16, MODE_STEREO, 48000);
  double size2time = 1.0 / 192000;

  Dejitter f;
  NoiseGen noise(spk, seed, chunk_size * nchunks, chunk_size);

  Chunk in, out;
  vtime_t continuous_time = 0;
  BOOST_REQUIRE(f.open(spk));
  while (noise.get_chunk(in))
  {
    // Do not add jitter to the first chunk to let
    // dejitter to catch the correct reference time
    bool sync = rng.get_bool();
    if (continuous_time == 0)
    {
      sync = true;
      in.set_sync(true, continuous_time);
    }
    else if (sync)
      in.set_sync(true, continuous_time + rng.get_double() * jitter);

    BOOST_REQUIRE(f.process(in, out));
    BOOST_REQUIRE_EQUAL(out.sync, sync);
    if (sync)
      BOOST_REQUIRE(compare_time(out.time, continuous_time));
    continuous_time += out.size * size2time;
  }

  vtime_t mean = f.get_input_mean();
  vtime_t sd = f.get_input_stddev();

  BOOST_CHECK_GT(fabs(f.get_input_mean()), 0);
  BOOST_CHECK_LT(fabs(f.get_input_mean()), jitter/10);
  BOOST_CHECK_CLOSE(f.get_input_stddev(), jitter/2, 30.0);
  BOOST_CHECK_EQUAL(f.get_output_mean(), 0);
  BOOST_CHECK_EQUAL(f.get_output_stddev(), 0);
}

BOOST_AUTO_TEST_CASE(no_dejitter)
{
  // Process chunks with time jitter
  // Get the same jittered resulting time

  RNG rng;
  Speakers spk(FORMAT_PCM16, MODE_STEREO, 48000);
  double size2time = 1.0 / 192000;

  Dejitter f;
  f.set_dejitter(false);
  NoiseGen noise(spk, seed, chunk_size * nchunks, chunk_size);

  Chunk in, out;
  vtime_t continuous_time = 0;
  BOOST_REQUIRE(f.open(spk));
  while (noise.get_chunk(in))
  {
    bool sync = rng.get_bool();
    vtime_t jitter_time = continuous_time + rng.get_double() * jitter;
    if (sync)
      in.set_sync(true, jitter_time);

    BOOST_REQUIRE(f.process(in, out));
    BOOST_REQUIRE_EQUAL(out.sync, sync);
    if (sync)
      BOOST_REQUIRE_EQUAL(out.time, jitter_time);
    continuous_time += out.size * size2time;
  }

  BOOST_CHECK_GT(fabs(f.get_input_mean()), 0);
  BOOST_CHECK_LT(fabs(f.get_input_mean()), jitter/10);
  BOOST_CHECK_CLOSE(f.get_input_stddev(), jitter/2, 30.0);
  BOOST_CHECK_GT(fabs(f.get_output_mean()), 0);
  BOOST_CHECK_LT(fabs(f.get_output_mean()), jitter/10);
  BOOST_CHECK_CLOSE(f.get_output_stddev(), jitter/2, 30.0);
}

BOOST_AUTO_TEST_CASE(threshold)
{
  // When time difference exceeds the threshold Dejitter must resync

  Chunk in, out;
  Speakers spk(FORMAT_PCM16, MODE_STEREO, 48000);
  double size2time = 1.0 / 192000;
  vtime_t time, diff;

  Dejitter f;
  RawNoise buf(seed, chunk_size);
  BOOST_REQUIRE(f.open(spk));

  // No resync test
  f.reset();
  in.set_rawdata(buf.begin(), buf.size(), true, 0);
  f.process(in, out);
  time = buf.size() * size2time;
  diff = f.get_threshold() * 0.8;
  in.set_rawdata(buf.begin(), buf.size(), true, time + diff);
  f.process(in, out);
  BOOST_CHECK(out.sync);
  BOOST_CHECK_EQUAL(out.time, time);

  // Do resync test
  f.reset();
  in.set_rawdata(buf.begin(), buf.size(), true, 0);
  f.process(in, out);
  time = buf.size() * size2time;
  diff = f.get_threshold() * 1.2;
  in.set_rawdata(buf.begin(), buf.size(), true, time + diff);
  f.process(in, out);
  BOOST_CHECK(out.sync);
  BOOST_CHECK_EQUAL(out.time, time + diff);
}

BOOST_AUTO_TEST_SUITE_END()
