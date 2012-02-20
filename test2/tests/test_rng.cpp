/*
  RNG class test
  * Quick Park-Miller test
  * Full proof of generator correctness by direct comparison of the entire
    sequence with reference Park-Miller implementation
*/

#include <math.h>
#include "rng.h"
#include "buffer.h"
#include <boost/test/unit_test.hpp>

static const int seed = 860304;

static __forceinline uint32_t park_miller(uint32_t seed)
{
  static const int a = 16807;
  static const int m = 2147483647;
  static const int q = 127773; /* m div a */
  static const int r = 2836;   /* m mod a */

  int32_t hi = seed / q;
  int32_t lo = seed % q;
  int32_t test = a * lo - r * hi;
  return test > 0? test: test + m;
}

BOOST_AUTO_TEST_SUITE(rng)

BOOST_AUTO_TEST_CASE(constructor)
{
  RNG rng1;
  RNG rng2;
  BOOST_CHECK_EQUAL(rng1.next(), rng2.next());
}

BOOST_AUTO_TEST_CASE(init_constructor)
{
  RNG rng1(seed);
  RNG rng2(seed);
  BOOST_CHECK_EQUAL(rng1.next(), rng2.next());
}

BOOST_AUTO_TEST_CASE(seed_test)
{
  RNG rng1(seed);
  RNG rng2(seed + 1);
  BOOST_REQUIRE(rng1.next() != rng2.next());

  rng1.seed(seed);
  rng2.seed(seed);
  BOOST_CHECK_EQUAL(rng1.next(), rng2.next());
}

BOOST_AUTO_TEST_CASE(incorrect_seed)
{
  // Seeding with an incorrect value
  // should not lead to degenerated sequence

  RNG rng;
  uint32_t z;

  rng.seed(0);
  z = rng.next();
  BOOST_CHECK(rng.next() != z);

  rng.seed((1U << 31) - 1);
  z = rng.next();
  BOOST_CHECK(rng.next() != z);

  rng.seed(1U << 31);
  z = rng.next();
  BOOST_CHECK(rng.next() != z);
}

BOOST_AUTO_TEST_CASE(assignment)
{
  RNG rng1(seed);
  RNG rng2(seed + 1);
  BOOST_REQUIRE(rng1.next() != rng2.next());

  rng2 = rng1;
  BOOST_CHECK_EQUAL(rng1.next(), rng2.next());
}

BOOST_AUTO_TEST_CASE(randomize)
{
  RNG rng;
  rng.randomize();
  uint32_t value = rng.next();

  bool equal = true;
  for (int i = 0; i < 10; i++)
  {
    rng.randomize();
    if (rng.next() != value)
      equal = false;
  }
  BOOST_CHECK(!equal);
}

BOOST_AUTO_TEST_CASE(get_range)
{
  int i;
  const int range = 10;
  const int nvalues = 1000;

  // Clear distribution
  int distr[range];
  for (int i = 0; i < range; i++)
    distr[i] = 0;

  // Build distribution
  RNG rng;
  for (i = 0; i < range * nvalues; i++)
  {
    int value = rng.get_range(range);
    if (value < 0 || value >= range)
      BOOST_REQUIRE(value >= 0 && value < range);
    distr[value]++;
  }

  // Find the deviation form the normal distribution
  double stddev = 0;
  for (i = 0; i < range; i++)
    stddev += (distr[i] - nvalues)*(distr[i] - nvalues);
  stddev = sqrt(stddev / range) / nvalues;

  BOOST_CHECK(stddev < 0.03);
}

BOOST_AUTO_TEST_CASE(get_sample)
{
  int i;
  const int range = 10;
  const int nvalues = 1000;

  // Clear distribution
  int distr[range];
  for (int i = 0; i < range; i++)
    distr[i] = 0;

  // Build distribution
  RNG rng;
  for (i = 0; i < range * nvalues; i++)
  {
    sample_t sample = rng.get_sample();
    if (sample <= -1.0 || sample >= 1.0)
      BOOST_REQUIRE(sample > -1.0 && sample < 1.0);

    int value = int((sample + 1.0) / 2.0 * range);
    assert(value < range);
    distr[value]++;
  }

  // Find the deviation form the normal distribution
  double stddev = 0;
  for (i = 0; i < range; i++)
    stddev += (distr[i] - nvalues)*(distr[i] - nvalues);
  stddev = sqrt(stddev / range) / nvalues;

  BOOST_CHECK(stddev < 0.03);
}

BOOST_AUTO_TEST_CASE(fill_raw)
{
  // Ensure no out-of-bound errors

  const int max_offset = 32;
  const int max_size = 256;
  const uint32_t guard = 0x55555555;
  uint32_t *guard1;
  uint32_t *guard2;

  const int buf_size = max_offset + max_size + sizeof(guard) * 2;
  Rawdata buf(buf_size);

  RNG rng;
  for (int offset = 0; offset < max_offset; offset++)
    for (int size = 0; size < max_size; size++)
    {
      uint8_t *data = buf.begin() + offset + sizeof(guard);
      guard1 = (uint32_t *)(buf.begin() + offset);
      guard2 = (uint32_t *)(buf.begin() + offset + size + sizeof(guard));

      *guard1 = guard;
      *guard2 = guard;

      rng.fill_raw(data, size);

      if (*guard1 != guard || *guard2 != guard)
        BOOST_FAIL("Out of bounds at offset = " << offset << " size = " << size);
    }
}

BOOST_AUTO_TEST_CASE(quick_park_miller_test)
{
  // Quick Park&Miller test
  RNG rng(1);
  uint32_t z;
  for (int i = 0; i < 10000; i++)
    z = rng.next();
  BOOST_CHECK_EQUAL(z, 1043618065);
}

/*
// in debug builds it is so
// slooooooooooooooooooooow
BOOST_AUTO_TEST_CASE(full_proof)
{
  RNG rng;
  uint32_t seed;
  int i;

  /////////////////////////////////////////////////////////
  // Test Park-Miller implementation

  seed = 1;
  for (i = 0; i < 10000; i++)
    seed = park_miller(seed);

  BOOST_REQUIRE_EQUAL(seed, 1043618065);

  /////////////////////////////////////////////////////////
  // Compare the entire sequence directly

  seed = 1;
  rng.seed(seed);
  for (i = 0; i < 0x7fffffff; i++)
  {
    seed = park_miller(seed);
    if (seed != rng.next())
      BOOST_FAIL("Sequence fails at element " << i);
  }
}
*/
BOOST_AUTO_TEST_SUITE_END()
