/*
  SampleBuf test
*/


#include <limits>
#include <boost/test/unit_test.hpp>
#include "buffer.h"

// bad_size must be less than a half of address space
// otherwise VC++ throws "bad allocation size" exception
// instead of std::bad_alloc in debug builds
static const size_t bad_size = std::numeric_limits<size_t>::max() / sizeof(sample_t) / 2 - 1;
static const size_t size1 = 100;
static const size_t size2 = 200;
static const size_t nch1 = 2;
static const size_t nch2 = NCHANNELS;

static bool is_deallocated(SampleBuf &buf)
{ return !buf.is_allocated() && buf.nch() == 0 && buf.nsamples() == 0; }

static bool is_allocated(SampleBuf &buf, unsigned nch, unsigned nsamples)
{ return buf.is_allocated() && buf.nch() == nch && buf.nsamples() == nsamples; }

BOOST_AUTO_TEST_SUITE(sample_buf)

BOOST_AUTO_TEST_CASE(default_constructor)
{
  SampleBuf buf;
  BOOST_CHECK(is_deallocated(buf));

  // Destructor of an default constructed buffer
  // should not fail after leaving of this block
}

BOOST_AUTO_TEST_CASE(init_constructor)
{
  SampleBuf buf(nch1, size1);
  BOOST_CHECK(is_allocated(buf, nch1, size1));

  // Test automatic cast to samples_t and sample_t *
  samples_t s = buf;
  sample_t *ps = buf[0];

  // Destructor of an init constructed buffer
  // should not fail after leaving of this block
}

BOOST_AUTO_TEST_CASE(init_constructor_failure)
{
  BOOST_CHECK_THROW( SampleBuf buf(1, bad_size), std::bad_alloc );
}

BOOST_AUTO_TEST_CASE(allocate)
{
  SampleBuf buf;

  // Just allocate a buffer
  buf.allocate(nch2, size1);
  BOOST_CHECK(is_allocated(buf, nch2, size1));

  // Allocate more data
  buf.allocate(nch2, size2);
  BOOST_CHECK(is_allocated(buf, nch2, size2));

  // Allocate less data
  buf.allocate(nch1, size1);
  BOOST_CHECK(is_allocated(buf, nch1, size1));

  // Free buffer
  buf.free();
  BOOST_CHECK(is_deallocated(buf));
}

BOOST_AUTO_TEST_CASE(allocate_failure)
{
  SampleBuf buf;

  // Allocate too much (fail allocation)
  // when buffer is not allocated
  BOOST_CHECK_THROW( buf.allocate(1, bad_size), std::bad_alloc);

  // Allocate too much (fail allocation)
  // when buffer is allocated
  buf.allocate(nch1, size1);
  BOOST_CHECK_THROW( buf.allocate(1, bad_size), std::bad_alloc);
}

BOOST_AUTO_TEST_CASE(reallocate)
{
  const struct { int nch, nsamples; } states[] =
  { { nch1, size1 }, { nch1, size2 }, { nch2, size1 }, { nch2, size2 } };

  for (int i = 0; i < array_size(states); i++)
    for (int j = i + 1; j < array_size(states); j++)
    {
      // Allocate and fill the buffer
      SampleBuf buf(states[i].nch, states[i].nsamples);
      BOOST_REQUIRE(buf.is_allocated());

      // Mark borders
      int nch = MIN(states[i].nch, states[j].nch);
      int nsamples = MIN(states[i].nsamples, states[j].nsamples);
      buf[0][0] = 0;
      buf[0][nsamples-1] = nsamples-1;
      buf[nch-1][0] = (nch-1)*nsamples;
      buf[nch-1][nsamples-1] = (nch-1)*nsamples + nsamples-1;

      // Reallocate
      buf.reallocate(states[j].nch, states[j].nsamples);
      BOOST_REQUIRE(buf.is_allocated());

      // Check borders
      BOOST_CHECK(buf[0][0] == 0);
      BOOST_CHECK(buf[0][nsamples-1] == nsamples-1);
      BOOST_CHECK(buf[nch-1][0] == (nch-1)*nsamples);
      BOOST_CHECK(buf[nch-1][nsamples-1] == (nch-1)*nsamples + nsamples-1);

      // Check zero fill
      if (states[j].nsamples > states[i].nsamples)
      {
        BOOST_CHECK(buf[0][nsamples] == 0);
        BOOST_CHECK(buf[0][states[j].nsamples-1] == 0);
        BOOST_CHECK(buf[nch-1][nsamples] == 0);
        BOOST_CHECK(buf[nch-1][states[j].nsamples-1] == 0);
      }

      if (states[j].nch > states[i].nch)
      {
        BOOST_CHECK(buf[nch][0] == 0);
        BOOST_CHECK(buf[nch][states[j].nsamples-1] == 0);
        BOOST_CHECK(buf[states[j].nch-1][0] == 0);
        BOOST_CHECK(buf[states[j].nch-1][states[j].nsamples-1] == 0);
      }
    }
}

BOOST_AUTO_TEST_CASE(zero)
{
  SampleBuf buf(nch1, size1);
  BOOST_REQUIRE(buf.is_allocated());

  // Write borders
  buf[0][0] = 1;
  buf[0][size1-1] = 1;
  buf[nch1-1][0] = 1;
  buf[nch1-1][size1-1] = 1;

  // Zero the buffer
  buf.zero();

  // Check borders
  BOOST_CHECK(buf[0][0] == 0);
  BOOST_CHECK(buf[0][size1-1] == 0);
  BOOST_CHECK(buf[nch1-1][0] == 0);
  BOOST_CHECK(buf[nch1-1][size1-1] == 0);
}

BOOST_AUTO_TEST_SUITE_END()
