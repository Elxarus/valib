/*
  PESParser test
*/

#include <boost/test/unit_test.hpp>
#include "parsers/pes/pes_parser.h"
#include "parsers/uni/uni_frame_parser.h"
#include "source/file_parser.h"
#include "source/raw_source.h"
#include "../../../suite.h"

static void compare_file(const char *test_filename, const char *raw_filename)
{
  BOOST_MESSAGE("Transform " << test_filename << " -> " << raw_filename);

  FileParser f_test;
  PESFrameParser test_frame_parser;
  f_test.open_probe(test_filename, &test_frame_parser);
  BOOST_REQUIRE(f_test.is_open());

  RAWSource f_raw(Speakers(FORMAT_RAWDATA, 0, 0), raw_filename);
  BOOST_REQUIRE(f_raw.is_open());

  PESParser test_parser;
  compare(&f_test, &test_parser, &f_raw, 0);
}

static void test_streams_frames(const char *filename, int streams, int frames)
{
  BOOST_MESSAGE("Count frames at " << filename);

  FileParser f;
  PESFrameParser frame_parser;
  f.open_probe(filename, &frame_parser);
  BOOST_REQUIRE(f.is_open());

  PESParser parser;
  parser.open(f.get_output());
  BOOST_CHECK(parser.is_open());

  check_streams_chunks(&f, &parser, streams, frames);
}

BOOST_AUTO_TEST_SUITE(pes_parser)

BOOST_AUTO_TEST_CASE(constructor)
{
  PESParser pes;
}

BOOST_AUTO_TEST_CASE(parse)
{
  compare_file("a.mp2.005.pes", "a.mp2.005.mp2");
  compare_file("a.mp2.002.pes", "a.mp2.002.mp2");
  compare_file("a.mp2.mix.pes", "a.mp2.mix.mp2");
  compare_file("a.ac3.03f.pes", "a.ac3.03f.ac3");
  compare_file("a.ac3.005.pes", "a.ac3.005.ac3");
  compare_file("a.ac3.mix.pes", "a.ac3.mix.ac3");
  compare_file("a.dts.03f.pes", "a.dts.03f.dts");
  compare_file("a.pcm.005.pes", "a.pcm.005.lpcm");
  compare_file("a.mad.mix.pes", "a.mad.mix.mad");
  compare_file("a.madp.mix.pes", "a.madp.mix.madp");
}

BOOST_AUTO_TEST_CASE(streams_frames)
{
  test_streams_frames("a.mp2.005.pes", 1, 286);
  test_streams_frames("a.mp2.002.pes", 1, 143);
  test_streams_frames("a.mp2.mix.pes", 3, 715);
  test_streams_frames("a.ac3.03f.pes", 1, 334);
  test_streams_frames("a.ac3.005.pes", 1, 239);
  test_streams_frames("a.ac3.mix.pes", 4, 1241);
  test_streams_frames("a.dts.03f.pes", 1, 564);
  test_streams_frames("a.pcm.005.pes", 1, 1148);
  test_streams_frames("a.mad.mix.pes", 7, 2702);
  test_streams_frames("a.madp.mix.pes", 13, 7330);
}

BOOST_AUTO_TEST_SUITE_END()
