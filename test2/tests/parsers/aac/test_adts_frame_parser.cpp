/*
  ADTSFrameParser test
*/

#include <boost/test/unit_test.hpp>
#include "parsers/aac/aac_adts_header.h"

static const uint8_t good[][8] =
{                                                     // Profile SRate Pr  Ch
  { 0xff, 0xf9, 0x00, 0x40, 0x76, 0x5f, 0xfc, 0xde }, //      00  0000  0  001
  { 0xff, 0xf9, 0x44, 0x80, 0x76, 0x5f, 0xfc, 0xde }, //      01  0001  0  010
  { 0xff, 0xf9, 0x88, 0xc0, 0x76, 0x5f, 0xfc, 0xde }, //      10  0010  0  011
  { 0xff, 0xf9, 0xcd, 0x00, 0x76, 0x5f, 0xfc, 0xde }, //      11  0011  0  100
  { 0xff, 0xf9, 0x11, 0x40, 0x76, 0x5f, 0xfc, 0xde }, //      00  0100  0  101
  { 0xff, 0xf9, 0x55, 0x80, 0x76, 0x5f, 0xfc, 0xde }, //      01  0101  0  110
  { 0xff, 0xf9, 0x99, 0xc0, 0x76, 0x5f, 0xfc, 0xde }, //      10  0110  0  111
  { 0xff, 0xf9, 0xdc, 0x40, 0x76, 0x5f, 0xfc, 0xde }, //      11  0111  0  001
  { 0xff, 0xf9, 0x20, 0x80, 0x76, 0x5f, 0xfc, 0xde }, //      00  1000  0  010
  { 0xff, 0xf9, 0x64, 0xc0, 0x76, 0x5f, 0xfc, 0xde }, //      01  1001  0  011
  { 0xff, 0xf9, 0xa9, 0x00, 0x76, 0x5f, 0xfc, 0xde }, //      10  1010  0  100
  { 0xff, 0xf9, 0xed, 0x40, 0x76, 0x5f, 0xfc, 0xde }, //      11  1011  0  101
};                                                               

static const Speakers good_spk[] =
{
  Speakers(FORMAT_AAC_ADTS, MODE_1_0,     96000),
  Speakers(FORMAT_AAC_ADTS, MODE_2_0,     88200),
  Speakers(FORMAT_AAC_ADTS, MODE_3_0,     64000),
  Speakers(FORMAT_AAC_ADTS, MODE_3_1,     48000),
  Speakers(FORMAT_AAC_ADTS, MODE_3_2,     44100),
  Speakers(FORMAT_AAC_ADTS, MODE_3_2_LFE, 32000),
  Speakers(FORMAT_AAC_ADTS, MODE_5_2_LFE, 24000),
  Speakers(FORMAT_AAC_ADTS, MODE_1_0,     22050),
  Speakers(FORMAT_AAC_ADTS, MODE_2_0,     16000),
  Speakers(FORMAT_AAC_ADTS, MODE_3_0,     12000),
  Speakers(FORMAT_AAC_ADTS, MODE_3_1,     11025),
  Speakers(FORMAT_AAC_ADTS, MODE_3_2,      8000),
};

static const uint8_t bad[][8] =
{
  { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, // null header
  { 0xff, 0xfb, 0xcd, 0x00, 0x76, 0x5f, 0xfc, 0xde }, // bad layer
  { 0xff, 0xfd, 0xcd, 0x00, 0x76, 0x5f, 0xfc, 0xde }, // bad layer
  { 0xff, 0xff, 0xcd, 0x00, 0x76, 0x5f, 0xfc, 0xde }, // bad layer
  { 0xff, 0xf9, 0xf1, 0x00, 0x76, 0x5f, 0xfc, 0xde }, // bad srate
  { 0xff, 0xf9, 0xf5, 0x00, 0x76, 0x5f, 0xfc, 0xde }, // bad srate
  { 0xff, 0xf9, 0xf9, 0x00, 0x76, 0x5f, 0xfc, 0xde }, // bad srate
  { 0xff, 0xf9, 0xfd, 0x00, 0x76, 0x5f, 0xfc, 0xde }, // bad srate
  { 0xff, 0xf9, 0xcc, 0x00, 0x76, 0x5f, 0xfc, 0xde }, // bad channel config
};

BOOST_AUTO_TEST_SUITE(adts_frame_parser)

BOOST_AUTO_TEST_CASE(can_parse)
{
  ADTSFrameParser parser;
  BOOST_CHECK(parser.can_parse(FORMAT_AAC_ADTS));
}

BOOST_AUTO_TEST_CASE(sync_info)
{
  SyncInfo sinfo = ADTSFrameParser().sync_info();

  for (int i = 0; i < array_size(good); i++)
    BOOST_CHECK(sinfo.sync_trie.is_sync(good[i]));

  for (int i = 0; i < array_size(bad); i++)
    BOOST_CHECK(!sinfo.sync_trie.is_sync(bad[i]));
}

BOOST_AUTO_TEST_CASE(parse_header)
{
  FrameInfo finfo;
  ADTSFrameParser parser;

  for (int i = 0; i < array_size(good); i++)
  {
    BOOST_CHECK(parser.parse_header(good[i], &finfo));
    BOOST_CHECK(good_spk[i] == finfo.spk);
  }

  for (int i = 0; i < array_size(bad); i++)
    BOOST_CHECK(!parser.parse_header(bad[i]));
}

BOOST_AUTO_TEST_CASE(compare_headers)
{
  ADTSFrameParser parser;

  // Compare equal good headers
  for (int i = 0; i < array_size(good); i++)
    BOOST_CHECK(parser.compare_headers(good[i], good[i]));

  // Compare different good headers
  for (int i = 0; i < array_size(good); i++)
    for (int j = i+1; j < array_size(good); j++)
    {
      BOOST_CHECK(!parser.compare_headers(good[i], good[j]));
      BOOST_CHECK(!parser.compare_headers(good[j], good[i]));
    }

  // Compare good with bad
  for (int i = 0; i < array_size(good); i++)
    for (int j = 0; j < array_size(bad); j++)
    {
      BOOST_CHECK(!parser.compare_headers(good[i], bad[j]));
      BOOST_CHECK(!parser.compare_headers(bad[j], good[i]));
    }

  // Compare bad headers
  for (int i = 0; i < array_size(bad); i++)
    for (int j = 0; j < array_size(bad); j++)
      BOOST_CHECK(!parser.compare_headers(bad[i], bad[j]));
}

BOOST_AUTO_TEST_SUITE_END()
