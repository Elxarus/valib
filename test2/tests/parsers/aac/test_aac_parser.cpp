/*
  AACParser test
*/

#include <boost/test/unit_test.hpp>
#include "filters/convert.h"
#include "parsers/aac/aac_parser.h"
#include "parsers/aac/aac_adts_parser.h"
#include "parsers/aac/aac_adts_header.h"
#include "source/file_parser.h"
#include "source/source_filter.h"
#include "source/wav_source.h"
#include "../../../suite.h"

const size_t block_size = 65536;

BOOST_AUTO_TEST_SUITE(aac_parser)

BOOST_AUTO_TEST_CASE(constructor)
{
  AACParser aac;
}

BOOST_AUTO_TEST_CASE(decode)
{
  // Test chain:
  // FileParser -> ParserFilter(ADTS) -> AACParser

  FileParser f;
  f.open("a.aac.03f.adts", &adts_header);
  BOOST_REQUIRE(f.is_open());

  ADTSParser adts;
  SourceFilter test_src(&f, &adts);

  AACParser aac;

  // Reference chain:
  // WAVSource -> Converter

  WAVSource wav("a.aac.03f.adts.wav", block_size);
  BOOST_REQUIRE(wav.is_open());

  Converter conv(1024);
  conv.set_format(FORMAT_LINEAR);
  conv.set_order(win_order);

  // Compare
  double diff = calc_diff(&test_src, &aac, &wav, &conv);
  BOOST_CHECK_LE(diff, 1e-10);
}

BOOST_AUTO_TEST_SUITE_END()
