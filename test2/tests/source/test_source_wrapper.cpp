/*
  SourceWrapper class test
*/

#include <boost/test/unit_test.hpp>
#include "source.h"
#include "source/generator.h"
#include "../../suite.h"

static const Speakers spk(FORMAT_LINEAR, MODE_STEREO, 48000);
static const int seed = 3487593476;
static const size_t len = 100;
static const size_t chunk_size = 20;

BOOST_AUTO_TEST_SUITE(source_wrapper)

BOOST_AUTO_TEST_CASE(constructor)
{
  SourceWrapper s;

  BOOST_CHECK_EQUAL(s.get_output(), spk_unknown);
  BOOST_CHECK(!s.new_stream());
  BOOST_CHECK(!s.get_chunk(Chunk()));
  s.reset();

  BOOST_CHECK_EQUAL(s.info(), string());
  BOOST_CHECK_EQUAL(s.name(), string("SourceWrapper"));
}

BOOST_AUTO_TEST_CASE(init_constructor)
{
  NoiseGen noise(spk, seed, len, chunk_size);
  NoiseGen test(spk, seed, len, chunk_size);
  SourceWrapper s(&test);

  BOOST_CHECK_EQUAL(s.get_output(), spk);
  BOOST_CHECK(!s.new_stream());
  compare(&s, &noise);

  s.reset();
  noise.reset();
  compare(&s, &noise);

  BOOST_CHECK_EQUAL(s.info(), string());
  BOOST_CHECK_EQUAL(s.name(), string("SourceWrapper/NoiseGen"));
}

BOOST_AUTO_TEST_SUITE_END()
