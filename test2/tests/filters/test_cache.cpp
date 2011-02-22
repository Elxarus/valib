/*
  Cache class test
*/

#include <boost/test/unit_test.hpp>
#include "filters/cache.h"
#include "source/generator.h"

static const int seed = 34098577;
static const size_t buf_size = 1024;
static const vtime_t cache_size = 0.125; // 6000 samples at 48kHz sample rate
static const Speakers spk(FORMAT_LINEAR, MODE_5_1, 48000);

static void compare_cached_samples(CacheFilter &f, samples_t samples, size_t nsamples, vtime_t time)
{
  Speakers spk = f.get_input();
  Samples buf(nsamples);
  for (int ch = 0; ch < spk.nch(); ch++)
  {
    f.get_samples(std_order[ch], time, buf.begin(), nsamples);
    sample_t diff = peak_diff(buf.begin(), samples[ch], nsamples);
    BOOST_CHECK_EQUAL(diff, 0.0);
  }
}

BOOST_AUTO_TEST_SUITE(cache_filter)

BOOST_AUTO_TEST_CASE(constructor)
{
  CacheFilter f;
  BOOST_CHECK_EQUAL(f.get_size(), 0);
  BOOST_CHECK_EQUAL(f.get_nsamples(), 0);
}

BOOST_AUTO_TEST_CASE(init_constructor)
{
  CacheFilter f(cache_size);
  BOOST_CHECK_EQUAL(f.get_size(), cache_size);
}

BOOST_AUTO_TEST_CASE(set_size)
{
  CacheFilter f;

  f.set_size(2*cache_size);
  BOOST_CHECK_EQUAL(f.get_size(), 2*cache_size);
  BOOST_CHECK_EQUAL(f.get_nsamples(), 0);

  // Cache size change in closed state
  f.set_size(cache_size);
  BOOST_CHECK_EQUAL(f.get_size(), cache_size);
  BOOST_CHECK_EQUAL(f.get_nsamples(), 0);

  // Test buffer initialization on open
  BOOST_REQUIRE(f.open(spk));
  BOOST_CHECK_EQUAL(f.get_size(), cache_size);
  BOOST_CHECK_EQUAL(f.get_nsamples(), round<size_t>(cache_size * spk.sample_rate));

  // Cache size change in open state
  f.set_size(2*cache_size);
  BOOST_CHECK_EQUAL(f.get_size(), 2*cache_size);
  BOOST_CHECK_EQUAL(f.get_nsamples(), round<size_t>(2 * cache_size * spk.sample_rate));

  // Test uninitialization
  f.close();
  BOOST_CHECK_EQUAL(f.get_size(), 2*cache_size);
  BOOST_CHECK_EQUAL(f.get_nsamples(), 0);
}

BOOST_AUTO_TEST_CASE(get_samples)
{
  CacheFilter f(cache_size);
  BOOST_REQUIRE(f.open(spk));

  size_t cache_samples = f.get_nsamples();
  size_t chunk_size = round<size_t>(cache_samples / 1.3);

  Chunk noise_chunk;
  NoiseGen noise(spk, seed, chunk_size * 10, chunk_size);
  while (noise.get_chunk(noise_chunk))
  {
    f.process(Chunk(noise_chunk), Chunk());
    vtime_t time = f.get_time() - (noise_chunk.size / spk.sample_rate);
    compare_cached_samples(f, noise_chunk.samples, noise_chunk.size, time);
  }
}

BOOST_AUTO_TEST_CASE(resize)
{
  Chunk noise_chunk;

  CacheFilter f(cache_size);
  BOOST_REQUIRE(f.open(spk));

  // Make noise buffer
  size_t cache_samples = f.get_nsamples();
  NoiseGen noise(spk, seed, cache_samples, cache_samples);
  BOOST_REQUIRE(noise.get_chunk(noise_chunk));

  // Put noise into the buffer
  BOOST_REQUIRE(f.process(Chunk(noise_chunk), Chunk()));

  // Shrink the cache and compare data
  f.set_size(cache_size / 2);
  cache_samples = f.get_nsamples();
  Samples buf(cache_samples);
  vtime_t time = f.get_time() - cache_samples / spk.sample_rate;
  compare_cached_samples(f, noise_chunk.samples + noise_chunk.size - cache_samples, cache_samples, time);

  // Expand the cache and compare data
  f.set_size(cache_size);
  compare_cached_samples(f, noise_chunk.samples + noise_chunk.size - cache_samples, cache_samples, time);
}

BOOST_AUTO_TEST_SUITE_END()
