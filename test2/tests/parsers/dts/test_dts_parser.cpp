/*
  DTSParser test
*/

#include <boost/test/unit_test.hpp>
#include "filters/convert.h"
#include "parsers/dts/dts_parser.h"
#include "parsers/dts/dts_header.h"
#include "source/file_parser.h"
#include "source/wav_source.h"
#include "../../../suite.h"

const size_t block_size = 65536;

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
  f.open_probe("a.dts.03f.dts", &dts_header);
  BOOST_REQUIRE(f.is_open());

  DTSParser dts;

  // Reference chain:
  // WAVSource -> Converter

  WAVSource wav("a.dts.03f.dts.wav", block_size);
  BOOST_REQUIRE(wav.is_open());

  Converter conv(1024);
  conv.set_format(FORMAT_LINEAR);
  conv.set_order(win_order);

  // Compare
  // 32bit floating-point has 24-bit mantissa,
  // therefore noise level is about -144dB.
  // So 1e-7 (-140dB) is usable threshold value 
  double diff = calc_diff(&f, &dts, &wav, &conv);
  BOOST_CHECK_LE(diff, 1e-7);
}

BOOST_AUTO_TEST_CASE(streams_frames)
{
  FileParser f;
  f.open_probe("a.dts.03f.mix", &dts_header);
  BOOST_REQUIRE(f.is_open());

  DTSParser parser;
  parser.open(f.get_output());
  BOOST_CHECK(parser.is_open());

  check_streams_chunks(&f, &parser, 3, 3375);
}

BOOST_AUTO_TEST_SUITE_END()
