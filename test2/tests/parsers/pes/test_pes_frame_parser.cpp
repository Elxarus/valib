/*
  PESFrameParser test
*/

#include <boost/test/unit_test.hpp>
#include "parsers/pes/pes_frame_parser.h"

static const uint8_t good[][6] =
{
  { 0x00, 0x00, 0x01, 0xbd, 0x01, 0x00 }, // Private stream 1
  { 0x00, 0x00, 0x01, 0xc0, 0x01, 0x00 }, // MPEG audio
};

static const Speakers good_spk[] =
{
  Speakers(FORMAT_PES, 0, 0),
  Speakers(FORMAT_PES, 0, 0),
};

static const uint8_t bad[][6] =
{
  { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, // null header
  { 0x00, 0x00, 0x01, 0x80, 0x01, 0x00 }, // Bad stream
};

BOOST_AUTO_TEST_SUITE(pes_frame_parser)

BOOST_AUTO_TEST_CASE(can_parse)
{
  PESFrameParser parser;
  BOOST_CHECK(parser.can_parse(FORMAT_PES));
}

BOOST_AUTO_TEST_CASE(sync_info)
{
  SyncInfo sinfo = PESFrameParser().sync_info();

  for (int i = 0; i < array_size(good); i++)
    BOOST_CHECK(sinfo.sync_trie.is_sync(good[i]));

//  for (int i = 0; i < array_size(bad); i++)
//    BOOST_CHECK(!sinfo.sync_trie.is_sync(bad[i]));
}

BOOST_AUTO_TEST_CASE(parse_header)
{
  FrameInfo finfo;
  PESFrameParser parser;

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
  PESFrameParser parser;

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
