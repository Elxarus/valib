/*
  MlpParser test
*/

#include <boost/test/unit_test.hpp>
#include "filters/convert.h"
#include "parsers/mlp/mlp_parser.h"
#include "parsers/mlp/mlp_header.h"
#include "source/file_parser.h"
#include "source/wav_source.h"
#include "../../../suite.h"

static const size_t block_size = 65536;

BOOST_AUTO_TEST_SUITE(mlp_parser)

BOOST_AUTO_TEST_CASE(constructor)
{
  MlpParser mlp;
}

BOOST_AUTO_TEST_CASE(decode)
{
  // Decoding test does not pass.
  // Not a lossless decoding?
/*
  // Test chain:
  // FileParser -> MlpParser
  FileParser f;
  MlpFrameParser frame_parser;
  MlpParser mlp;
  f.open_probe("test.mlp.527.mlp", &frame_parser);
  BOOST_REQUIRE(f.is_open());

  // Reference:
  // WAVSource
  WAVSource wav("test.pcm16.527.wav", block_size);
  BOOST_REQUIRE(wav.is_open());

  // Compare
  compare(&f, &mlp, &wav, 0);
*/
}

BOOST_AUTO_TEST_CASE(streams_frames)
{
  FileParser f;
  MlpFrameParser frame_parser;
  f.open_probe("test.mlp.527.mlp", &frame_parser);
  BOOST_REQUIRE(f.is_open());
  BOOST_CHECK_EQUAL(f.get_output(), Speakers(FORMAT_MLP, MODE_3_0_2_LFE, 48000));

  MlpParser parser;
  parser.open(f.get_output());
  BOOST_CHECK(parser.is_open());

  check_streams_chunks(&f, &parser, 1, 14400);
  BOOST_CHECK_EQUAL(parser.get_output(), Speakers(FORMAT_PCM16, MODE_3_0_2_LFE, 48000));
}

BOOST_AUTO_TEST_SUITE_END()
