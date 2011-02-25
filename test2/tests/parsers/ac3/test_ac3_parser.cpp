/*
  AC3Parser test
*/

#include <boost/test/unit_test.hpp>
#include "filters/convert.h"
#include "parsers/ac3/ac3_parser.h"
#include "parsers/ac3/ac3_header.h"
#include "source/file_parser.h"
#include "source/wav_source.h"
#include "../../../suite.h"

const size_t block_size = 65536;

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
  f.open_probe("a.ac3.03f.ac3", &ac3_header);
  BOOST_REQUIRE(f.is_open());

  AC3Parser ac3;

  // Reference chain:
  // WAVSource -> Converter

  WAVSource wav("a.ac3.03f.ac3.wav", block_size);
  BOOST_REQUIRE(wav.is_open());

  Converter conv(1024);
  conv.set_format(FORMAT_LINEAR);
  conv.set_order(win_order);

  // Compare
  // 32bit floating-point value has 24-bit mantissa,
  // therefore noise level is about -144dB.
  // So 1e-7 (-140dB) is usable threshold value 
  double diff = calc_diff(&f, &ac3, &wav, &conv);
  BOOST_CHECK_LE(diff, 1e-7);
}

BOOST_AUTO_TEST_SUITE_END()
