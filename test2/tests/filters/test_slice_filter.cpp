/*
  SliceFilter test
*/

#include <boost/test/unit_test.hpp>
#include "filters/slice.h"
#include "source/generator.h"
#include "../../suite.h"

static const int seed = 2345346;
static const size_t chunk_size1 = 128;
static const size_t chunk_size2 = 120;
static const size_t noise_size = 16 * chunk_size1;
static const Speakers spk(FORMAT_LINEAR, MODE_STEREO, 48000);

BOOST_AUTO_TEST_SUITE(slice_filter)

BOOST_AUTO_TEST_CASE(constructor)
{
  SliceFilter f;
  BOOST_CHECK_EQUAL(f.get_pos(), 0);
  BOOST_CHECK_EQUAL(f.get_start(), SliceFilter::not_specified);
  BOOST_CHECK_EQUAL(f.get_end(), SliceFilter::not_specified);
}

BOOST_AUTO_TEST_CASE(init_constructor)
{
  SliceFilter f(100, 200);
  BOOST_CHECK_EQUAL(f.get_pos(), 0);
  BOOST_CHECK_EQUAL(f.get_start(), 100);
  BOOST_CHECK_EQUAL(f.get_end(), 200);
}

BOOST_AUTO_TEST_CASE(init)
{
  SliceFilter f;
  f.init(100, 200);
  BOOST_CHECK_EQUAL(f.get_pos(), 0);
  BOOST_CHECK_EQUAL(f.get_start(), 100);
  BOOST_CHECK_EQUAL(f.get_end(), 200);
}

BOOST_AUTO_TEST_CASE(slice)
{
  /////////////////////////////////////////////////////////
  //  start | unspec |   0  |   x1   | end
  // end    |        |      |        |
  // -------+--------+------+--------+------
  // unspec |  pass  | pass | x1-end | none
  // 0      |  none  | none |   --   |  --
  // x1     |  0-x1  | 0-x1 |  none  |  --
  // x2     |  0-x2  | 0-x2 |  x1-x2 |  --
  // end    |  none  |  all | x1-end | none
  //
  // x1 = chunk_size
  // x2 = noise_size - chunk_size
  // pass - filter does not cut any data
  // none - filter drops all data
  //
  // We should check the filter with 2 chunk sizes:
  // * stream length is a multiple of the chunk size
  // * stream length is not a multiple of the chunk size

  static struct {
    size_t chunk_size;
    size_t start;
    size_t end;
  } bounds[] =
  {
    // chunk_size1
    { chunk_size1, SliceFilter::not_specified, SliceFilter::not_specified },
    { chunk_size1, SliceFilter::not_specified, 0 },
    { chunk_size1, SliceFilter::not_specified, chunk_size1 },
    { chunk_size1, SliceFilter::not_specified, noise_size - chunk_size1 },
    { chunk_size1, SliceFilter::not_specified, noise_size},

    { chunk_size1, 0, SliceFilter::not_specified },
    { chunk_size1, 0, 0 },
    { chunk_size1, 0, chunk_size1 },
    { chunk_size1, 0, noise_size - chunk_size1 },
    { chunk_size1, 0, noise_size},

    { chunk_size1, chunk_size1, SliceFilter::not_specified },
    { chunk_size1, chunk_size1, chunk_size1 },
    { chunk_size1, chunk_size1, noise_size - chunk_size1 },
    { chunk_size1, chunk_size1, noise_size},

    { chunk_size1, noise_size, SliceFilter::not_specified },
    { chunk_size1, noise_size, noise_size},

    // chunk_size2
    { chunk_size2, SliceFilter::not_specified, SliceFilter::not_specified },
    { chunk_size2, SliceFilter::not_specified, 0 },
    { chunk_size2, SliceFilter::not_specified, chunk_size2 },
    { chunk_size2, SliceFilter::not_specified, noise_size - chunk_size2 },
    { chunk_size2, SliceFilter::not_specified, noise_size},

    { chunk_size2, 0, SliceFilter::not_specified },
    { chunk_size2, 0, 0 },
    { chunk_size2, 0, chunk_size2 },
    { chunk_size2, 0, noise_size - chunk_size2 },
    { chunk_size2, 0, noise_size},

    { chunk_size2, chunk_size2, SliceFilter::not_specified },
    { chunk_size2, chunk_size2, chunk_size2 },
    { chunk_size2, chunk_size2, noise_size - chunk_size2 },
    { chunk_size2, chunk_size2, noise_size},

    { chunk_size2, noise_size, SliceFilter::not_specified },
    { chunk_size2, noise_size, noise_size},
  };

  NoiseGen noise1;
  NoiseGen noise2;
  SliceFilter slice;

  for (int i = 0; i < array_size(bounds); i++)
  {
    noise1.init(spk, seed, noise_size, bounds[i].chunk_size);
    if (bounds[i].start != SliceFilter::not_specified && (bounds[i].start == bounds[i].end || bounds[i].start == noise_size))
      // no output data
      noise2.init(spk, seed, 0, bounds[i].chunk_size);
    else if (bounds[i].end != SliceFilter::not_specified)
      // stream is limited at the end
      noise2.init(spk, seed, bounds[i].end, bounds[i].chunk_size);
    else
      // end is not specified
      noise2.init(spk, seed, noise_size, bounds[i].chunk_size);

    if (bounds[i].start != SliceFilter::not_specified && bounds[i].start == bounds[i].chunk_size)
      // Cut the start of the reference stream
      noise2.get_chunk(Chunk());

    slice.init(bounds[i].start, bounds[i].end);
    compare(&noise1, &slice, &noise2, 0);
  }
}

BOOST_AUTO_TEST_SUITE_END()
