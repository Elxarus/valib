/*
  AACParser test
*/

#include <boost/test/unit_test.hpp>
#include "filters/convert.h"
#include "filters/filter_graph.h"
#include "filters/slice.h"
#include "parsers/aac/aac_parser.h"
#include "parsers/aac/aac_adts_parser.h"
#include "parsers/aac/aac_adts_header.h"
#include "source/file_parser.h"
#include "source/source_filter.h"
#include "source/wav_source.h"
#include "../../../suite.h"

static const size_t block_size = 65536;

BOOST_AUTO_TEST_SUITE(aac_parser)

BOOST_AUTO_TEST_CASE(constructor)
{
  AACParser aac;
}

BOOST_AUTO_TEST_CASE(open_no_format_data)
{
  AACParser aac;
  BOOST_CHECK(aac.open(Speakers(FORMAT_AAC_FRAME, MODE_STEREO, 48000)));
}

BOOST_AUTO_TEST_CASE(decode)
{
  // Test chain:
  // FileParser -> ParserFilter(ADTS) -> AACParser -> Slice
  // Slice is required to cut the first frame because faad utility
  // drops it but AACParser does not.

  FileParser f;
  ADTSFrameParser frame_parser;
  f.open("a.aac.03f.adts", &frame_parser);
  BOOST_REQUIRE(f.is_open());

  ADTSParser adts;
  AACParser aac;
  SliceFilter cut_1st_frame(1024);

  // Reference chain:
  // WAVSource -> Converter

  WAVSource wav("a.aac.03f.adts.wav", block_size);
  BOOST_REQUIRE(wav.is_open());

  Converter conv(1024);
  conv.set_format(FORMAT_LINEAR);
  conv.set_order(win_order);

  // Compare
  double diff = calc_diff(&f, &FilterChain(&adts, &aac, &cut_1st_frame), &wav, &conv);
  BOOST_CHECK_LE(diff, 1e-6);
}

BOOST_AUTO_TEST_CASE(streams_frames)
{
  FileParser f;
  ADTSFrameParser frame_parser;
  f.open("a.aac.03f.adts", &frame_parser);
  BOOST_REQUIRE(f.is_open());

  ADTSParser adts;
  AACParser aac;
  check_streams_chunks(&f, &FilterChain(&adts, &aac), 1, 564);
}

BOOST_AUTO_TEST_SUITE_END()
