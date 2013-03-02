/*
  MPAFrameParser test
*/

#include <boost/test/unit_test.hpp>
#include "parsers/mpa/mpa_header.h"

static const uint8_t good[][4] =
{                             // ver layer err | bitrate | sf pad priv | mode ext | cpr orig emph
  { 0xff, 0xf2, 0x00, 0x00 }, //  10  01    0     0000     00  0    0     00  00     0   0    00
  { 0xff, 0xf3, 0x10, 0x10 }, //  10  01    1     0001     00  0    0     00  01     0   0    00
  { 0xff, 0xf4, 0x22, 0x20 }, //  10  10    0     0010     00  1    0     00  10     0   0    00
  { 0xff, 0xf5, 0x37, 0x30 }, //  10  10    1     0011     01  1    1     00  11     0   0    00
  { 0xff, 0xf6, 0x45, 0x40 }, //  10  11    0     0100     01  0    1     01  00     0   0    00
  { 0xff, 0xf7, 0x55, 0x50 }, //  10  11    1     0101     01  0    1     01  01     0   0    00
  { 0xff, 0xfa, 0x6a, 0x60 }, //  11  01    0     0110     10  1    0     01  10     0   0    00
  { 0xff, 0xfb, 0x7a, 0x70 }, //  11  01    1     0111     10  1    0     01  11     0   0    00
  { 0xff, 0xfc, 0x88, 0x80 }, //  11  10    0     1000     10  0    0     10  00     0   0    00
  { 0xff, 0xfd, 0x91, 0x90 }, //  11  10    1     1001     00  0    1     10  01     0   0    00
  { 0xff, 0xfe, 0xa7, 0xa0 }, //  11  11    0     1010     01  1    1     10  10     0   0    00
  { 0xff, 0xff, 0xbb, 0xb0 }, //  11  11    1     1011     10  1    1     10  11     0   0    00
  { 0xff, 0xe2, 0xc0, 0xc0 }, //  00  01    0     1100     00  0    0     11  00     0   0    00
  { 0xff, 0xe5, 0xd4, 0xd0 }, //  00  10    1     1101     01  0    0     11  01     0   0    00
  { 0xff, 0xe6, 0xea, 0xe0 }, //  00  11    0     1110     10  1    0     11  10     0   0    00

  { 0xf2, 0xff, 0x00, 0x00 }, //  0   01    0     0000     00  0    0     00  00     0   0    00
  { 0xf3, 0xff, 0x10, 0x10 }, //  0   01    1     0001     00  0    0     00  01     0   0    00
  { 0xf4, 0xff, 0x20, 0x22 }, //  0   10    0     0010     00  1    0     00  10     0   0    00
  { 0xf5, 0xff, 0x30, 0x37 }, //  0   10    1     0011     01  1    1     00  11     0   0    00
  { 0xf6, 0xff, 0x40, 0x45 }, //  0   11    0     0100     01  0    1     01  00     0   0    00
  { 0xf7, 0xff, 0x50, 0x55 }, //  0   11    1     0101     01  0    1     01  01     0   0    00
  { 0xf2, 0xff, 0x60, 0x6a }, //  0   01    0     0110     10  1    0     01  10     0   0    00
  { 0xf5, 0xff, 0x70, 0x7a }, //  0   10    1     0111     10  1    0     01  11     0   0    00
  { 0xfa, 0xff, 0x80, 0x88 }, //  1   01    0     1000     10  0    0     10  00     0   0    00
  { 0xfb, 0xff, 0x90, 0x91 }, //  1   01    1     1001     00  0    1     10  01     0   0    00
  { 0xfc, 0xff, 0xa0, 0xa7 }, //  1   10    0     1010     01  1    1     10  10     0   0    00
  { 0xfd, 0xff, 0xb0, 0xbb }, //  1   10    1     1011     10  1    1     10  11     0   0    00
  { 0xfe, 0xff, 0xc0, 0xc0 }, //  1   11    0     1100     00  0    0     11  00     0   0    00
  { 0xfe, 0xff, 0xd0, 0xd4 }, //  1   11    0     1101     01  0    0     11  01     0   0    00
  { 0xfa, 0xff, 0xe0, 0xea }, //  1   01    0     1110     10  1    0     11  10     0   0    00
};

static const Speakers good_spk[] =
{
  Speakers(FORMAT_MPA, MODE_STEREO, 22050),
  Speakers(FORMAT_MPA, MODE_STEREO, 22050),
  Speakers(FORMAT_MPA, MODE_STEREO, 22050),
  Speakers(FORMAT_MPA, MODE_STEREO, 24000),
  Speakers(FORMAT_MPA, MODE_STEREO, 24000),
  Speakers(FORMAT_MPA, MODE_STEREO, 24000),
  Speakers(FORMAT_MPA, MODE_STEREO, 32000),
  Speakers(FORMAT_MPA, MODE_STEREO, 32000),
  Speakers(FORMAT_MPA, MODE_STEREO, 32000),
  Speakers(FORMAT_MPA, MODE_STEREO, 44100),
  Speakers(FORMAT_MPA, MODE_STEREO, 48000),
  Speakers(FORMAT_MPA, MODE_STEREO, 32000),
  Speakers(FORMAT_MPA, MODE_MONO,   11025),
  Speakers(FORMAT_MPA, MODE_MONO,   12000),
  Speakers(FORMAT_MPA, MODE_MONO,    8000),

  Speakers(FORMAT_MPA, MODE_STEREO, 22050),
  Speakers(FORMAT_MPA, MODE_STEREO, 22050),
  Speakers(FORMAT_MPA, MODE_STEREO, 22050),
  Speakers(FORMAT_MPA, MODE_STEREO, 24000),
  Speakers(FORMAT_MPA, MODE_STEREO, 24000),
  Speakers(FORMAT_MPA, MODE_STEREO, 24000),
  Speakers(FORMAT_MPA, MODE_STEREO, 16000),
  Speakers(FORMAT_MPA, MODE_STEREO, 16000),
  Speakers(FORMAT_MPA, MODE_STEREO, 32000),
  Speakers(FORMAT_MPA, MODE_STEREO, 44100),
  Speakers(FORMAT_MPA, MODE_STEREO, 48000),
  Speakers(FORMAT_MPA, MODE_STEREO, 32000),
  Speakers(FORMAT_MPA, MODE_MONO,   44100),
  Speakers(FORMAT_MPA, MODE_MONO,   48000),
  Speakers(FORMAT_MPA, MODE_MONO,   32000),
};

static const uint8_t bad[][4] =
{
  { 0x00, 0x00, 0x00, 0x00 }, // null header

  { 0xff, 0xf9, 0xbb, 0xb0 }, // bad layer
  { 0xff, 0xf5, 0xfa, 0x70 }, // bad bitrate
  { 0xff, 0xf5, 0x7e, 0x70 }, // bad sampling frequency
  { 0xff, 0xeb, 0x7a, 0x70 }, // bad version

  { 0xf9, 0xff, 0xb0, 0xbb }, // bad layer
  { 0xf5, 0xff, 0x70, 0xfa }, // bad bitrate
  { 0xf5, 0xff, 0x70, 0x7e }, // bad sampling frequency
  { 0xe5, 0xff, 0x70, 0x7a }, // bad version
  { 0xed, 0xff, 0x70, 0x7a }, // bad version
};

BOOST_AUTO_TEST_SUITE(mpa_frame_parser)

BOOST_AUTO_TEST_CASE(can_parse)
{
  MPAFrameParser parser;
  BOOST_CHECK(parser.can_parse(FORMAT_MPA));
}

BOOST_AUTO_TEST_CASE(sync_info)
{
  SyncInfo sinfo = MPAFrameParser().sync_info();

  for (int i = 0; i < array_size(good); i++)
    BOOST_CHECK(sinfo.sync_trie.is_sync(good[i]));

//  for (int i = 0; i < array_size(bad); i++)
//    BOOST_CHECK(!sinfo.sync_trie.is_sync(bad[i]));
}

BOOST_AUTO_TEST_CASE(parse_header)
{
  FrameInfo finfo;
  MPAFrameParser parser;

  for (int i = 0; i < array_size(good); i++)
  {
    BOOST_CHECK_MESSAGE(parser.parse_header(good[i], &finfo), "good header N" << i);
    BOOST_CHECK(good_spk[i] == finfo.spk);
  }

  for (int i = 0; i < array_size(bad); i++)
    BOOST_CHECK_MESSAGE(!parser.parse_header(bad[i]), "bad header N" << i);
}

BOOST_AUTO_TEST_CASE(compare_headers)
{
  MPAFrameParser parser;

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
