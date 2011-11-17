/*
  ParserFilter class test
*/

#include <boost/test/unit_test.hpp>
#include "filters/parser_filter.h"
#include "parsers/ac3/ac3_header.h"
#include "parsers/ac3/ac3_parser.h"
#include "parsers/dts/dts_header.h"
#include "source/file_parser.h"
#include "source/raw_source.h"
#include "../../suite.h"


BOOST_AUTO_TEST_SUITE(parser_filter)

BOOST_AUTO_TEST_CASE(constructor)
{
  ParserFilter dec;
}

BOOST_AUTO_TEST_CASE(add_release)
{
  AC3FrameParser ac3;
  DTSFrameParser dts;
  Speakers spk_ac3(FORMAT_AC3, 0, 0);
  Speakers spk_dts(FORMAT_DTS, 0, 0);

  ParserFilter dec;
  BOOST_CHECK(!dec.can_open(spk_ac3));
  BOOST_CHECK(!dec.can_open(spk_dts));

  dec.add(&ac3, 0);
  BOOST_CHECK(dec.can_open(spk_ac3));
  BOOST_CHECK(!dec.can_open(spk_dts));

  dec.add(&dts, 0);
  BOOST_CHECK(dec.can_open(spk_ac3));
  BOOST_CHECK(dec.can_open(spk_dts));

  dec.release();
  BOOST_CHECK(!dec.can_open(spk_ac3));
  BOOST_CHECK(!dec.can_open(spk_dts));
}

BOOST_AUTO_TEST_CASE(decode)
{
  // Test chain: RAWSource -> ParserFilter(AC3Parser)
  RAWSource raw(Speakers(FORMAT_RAWDATA, 0, 0), "a.ac3.03f.ac3");
  BOOST_REQUIRE(raw.is_open());
  AC3FrameParser ac3_frame_parser;
  AC3Parser ac3_decoder;
  ParserFilter dec;
  dec.add(&ac3_frame_parser, &ac3_decoder);

  // Reference chain: FileParser(AC3Header) -> AC3Parser
  FileParser f;
  AC3FrameParser frame_parser;
  f.open("a.ac3.03f.ac3", &frame_parser);
  BOOST_REQUIRE(f.is_open());
  AC3Parser ac3_ref;

  double diff = calc_diff(&raw, &dec, &f, &ac3_ref);
  BOOST_CHECK_LE(diff, 1e-10);
}

BOOST_AUTO_TEST_SUITE_END()
