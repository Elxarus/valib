/*
  ADTSHeader test
*/

#include <boost/test/unit_test.hpp>
#include "parsers/aac/aac_adts_header.h"
#include "parsers/aac/aac_adts_parser.h"
#include "source/file_parser.h"
#include "../../../suite.h"

static const string test_info =
"Format: Unknown 5.1 48000\n"
"Profile: AAC LC\n"
"Frame size: 946\n"
"Bitrate: 354kbps\n";

BOOST_AUTO_TEST_SUITE(adts_parser)

BOOST_AUTO_TEST_CASE(constructor)
{
  ADTSParser parser;
}

BOOST_AUTO_TEST_CASE(info)
{
  Chunk chunk;

  FileParser f;
  f.open("a.aac.03f.adts", &adts_header);
  BOOST_REQUIRE(f.is_open());
  BOOST_REQUIRE(f.get_chunk(chunk));

  ADTSParser parser;
  parser.process(chunk, Chunk());
  BOOST_CHECK_EQUAL(parser.info(), test_info);
}

BOOST_AUTO_TEST_CASE(streams_frames)
{
  FileParser f;
  f.open("a.aac.03f.adts", &adts_header);
  BOOST_REQUIRE(f.is_open());

  ADTSParser adts;
  check_streams_chunks(&f, &adts, 1, 564);
}


BOOST_AUTO_TEST_SUITE_END()
