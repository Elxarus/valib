/*
  SPDIFParser test
*/

#include <boost/test/unit_test.hpp>
#include "filters/filter_graph.h"
#include "parsers/spdif/spdif_header.h"
#include "parsers/spdif/spdif_parser.h"
#include "parsers/spdif/spdif_wrapper.h"
#include "parsers/spdif/spdifable_header.h"
#include "source/file_parser.h"
#include "../../../suite.h"
#include "dts_frame_resize.h"

static void compare_file(const char *spdif_file, const char *raw_file)
{
  BOOST_MESSAGE("Transform " << spdif_file << " -> " << raw_file);

  FileParser f_spdif;
  SPDIFFrameParser spdif_frame_parser;
  f_spdif.open_probe(spdif_file, &spdif_frame_parser);
  BOOST_REQUIRE(f_spdif.is_open());

  FileParser f_raw;
  SpdifableFrameParser spdifable_frame_parser;
  f_raw.open_probe(raw_file, &spdifable_frame_parser);
  BOOST_REQUIRE(f_raw.is_open());

  SPDIFParser spdif;
  compare(&f_spdif, &spdif, &f_raw, 0);
}

static void test_streams_frames(const char *filename, int streams, int frames)
{
  BOOST_MESSAGE("Count frames at " << filename);

  FileParser f;
  SPDIFFrameParser frame_parser;
  f.open_probe(filename, &frame_parser);
  BOOST_REQUIRE(f.is_open());

  SPDIFParser parser;
  parser.open(f.get_output());
  BOOST_CHECK(parser.is_open());

  check_streams_chunks(&f, &parser, streams, frames);
}

BOOST_AUTO_TEST_SUITE(spdif_parser)

BOOST_AUTO_TEST_CASE(constructor)
{
  SPDIFParser spdif;
}

BOOST_AUTO_TEST_CASE(parse)
{
  compare_file("a.mp2.005.spdif", "a.mp2.005.mp2");
  compare_file("a.mp2.002.spdif", "a.mp2.002.mp2");
  compare_file("a.ac3.03f.spdif", "a.ac3.03f.ac3");
  compare_file("a.ac3.005.spdif", "a.ac3.005.ac3");
  compare_file("a.dts.03f.spdif", "a.dts.03f.dts");
  compare_file("a.mad.mix.spdif", "a.mad.mix.mad");
}

BOOST_AUTO_TEST_CASE(dts_spdif)
{
  // Test chain: FileParser -> (DTS) -> SPDIFWrapper -> (SPDIF/DTS) -> SPDIFParser
  FileParser f;
  DTSFrameParser frame_parser;
  f.open_probe("a.dts.03f.dts", &frame_parser);
  BOOST_REQUIRE(f.is_open());

  SPDIFWrapper spdifer(DTS_MODE_PADDED);
  SPDIFParser parser;

  // Test chain: FileParser -> (DTS) -> DTSFrameResize
  FileParser f_ref;
  DTSFrameParser frame_parser_ref;
  f_ref.open_probe("a.dts.03f.dts", &frame_parser_ref);
  BOOST_REQUIRE(f_ref.is_open());
  DTSFrameResize resize(2048);

  compare(&f, &FilterChain(&spdifer, &parser), &f_ref, &resize);
}

BOOST_AUTO_TEST_CASE(streams_frames)
{
  test_streams_frames("a.mp2.005.spdif", 1, 500);
  test_streams_frames("a.mp2.002.spdif", 1, 500);
  test_streams_frames("a.ac3.03f.spdif", 1, 375);
  test_streams_frames("a.ac3.005.spdif", 1, 375);
  test_streams_frames("a.dts.03f.spdif", 1, 1125);
  test_streams_frames("a.mad.mix.spdif", 7, 4375);
}

BOOST_AUTO_TEST_SUITE_END()
