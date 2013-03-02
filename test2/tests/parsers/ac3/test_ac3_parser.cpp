/*
  AC3Parser test
*/

#include <boost/test/unit_test.hpp>
#include "parsers/ac3/ac3_parser.h"
#include "parsers/ac3/ac3_header.h"
#include "source/file_parser.h"
#include "source/wav_source.h"
#include "../../../suite.h"

static const size_t block_size = 65536;

BOOST_AUTO_TEST_SUITE(ac3_parser)

BOOST_AUTO_TEST_CASE(constructor)
{
  AC3Parser ac3;
}

BOOST_AUTO_TEST_CASE(decode)
{
  // Test chain:
  // FileParser -> AC3Parser

  FileParser f;
  AC3FrameParser frame_parser;
  f.open_probe("a.ac3.03f.ac3", &frame_parser);
  BOOST_REQUIRE(f.is_open());

  AC3Parser ac3;

  // Reference chain:
  // WAVSource

  WAVSource wav("a.ac3.03f.ac3.wav", block_size);
  BOOST_REQUIRE(wav.is_open());

  // Compare
  // 32bit floating-point has 24-bit mantissa,
  // therefore noise level is about -144dB.
  // So 1e-7 (-140dB) is usable threshold value 
  double diff = calc_diff(&f, &ac3, &wav, 0);
  BOOST_CHECK_LE(diff, 1e-7);
}

BOOST_AUTO_TEST_CASE(streams_frames)
{
  FileParser f;
  AC3FrameParser frame_parser;
  f.open_probe("a.ac3.mix.ac3", &frame_parser);
  BOOST_REQUIRE(f.is_open());

  AC3Parser parser;
  parser.open(f.get_output());
  BOOST_CHECK(parser.is_open());

  check_streams_chunks(&f, &parser, 3, 1500);
}

BOOST_AUTO_TEST_SUITE_END()
