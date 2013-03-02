/*
  DTSParser test
*/

#include <boost/test/unit_test.hpp>
#include "parsers/dts/dts_parser.h"
#include "parsers/dts/dts_header.h"
#include "source/file_parser.h"
#include "source/wav_source.h"
#include "../../../suite.h"

static const size_t block_size = 65536;

BOOST_AUTO_TEST_SUITE(dts_parser)

BOOST_AUTO_TEST_CASE(constructor)
{
  DTSParser dts;
}

BOOST_AUTO_TEST_CASE(decode)
{
  // Test chain:
  // FileParser -> DTSParser

  FileParser f;
  DTSFrameParser frame_parser;
  f.open_probe("a.dts.03f.dts", &frame_parser);
  BOOST_REQUIRE(f.is_open());

  DTSParser dts;

  // Reference chain:
  // WAVSource

  WAVSource wav("a.dts.03f.dts.wav", block_size);

  // Compare
  // 32bit floating-point has 24-bit mantissa,
  // therefore noise level is about -144dB.
  // So 1e-7 (-140dB) is usable threshold value 
  double diff = calc_diff(&f, &dts, &wav, 0);
  BOOST_CHECK_LE(diff, 1e-7);
}

BOOST_AUTO_TEST_CASE(streams_frames)
{
  FileParser f;
  DTSFrameParser frame_parser;
  f.open_probe("a.dts.03f.mix", &frame_parser);
  BOOST_REQUIRE(f.is_open());

  DTSParser parser;
  parser.open(f.get_output());
  BOOST_CHECK(parser.is_open());

  check_streams_chunks(&f, &parser, 3, 3375);
}

BOOST_AUTO_TEST_SUITE_END()
