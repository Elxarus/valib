/*
  MultiFrameParser class test
*/

#include <boost/test/unit_test.hpp>
#include "parsers/multi_header.h"
#include "parsers/ac3/ac3_header.h"
#include "parsers/dts/dts_header.h"
#include "parsers/mpa/mpa_header.h"
#include "auto_file.h"


static AC3FrameParser ac3;
static DTSFrameParser dts;
static MPAFrameParser mpa;

static FrameParser *parsers_with_null[] = { &ac3, 0, &dts, &mpa };
static FrameParser *parsers[] = { &ac3, &dts, &mpa };
static const char *files[] = { "a.ac3.03f.ac3", "a.dts.03f.dts", "a.mp2.005.mp2" };

BOOST_AUTO_TEST_SUITE(multi_frame_parser)

BOOST_AUTO_TEST_CASE(constructor)
{
  MultiFrameParser parser;

  BOOST_CHECK_EQUAL(parser.get_parsers().size(), 0);
  BOOST_CHECK_EQUAL(parser.sync_info().sync_trie.is_empty(), true);
  BOOST_CHECK_EQUAL(parser.sync_info().min_frame_size, 0);
  BOOST_CHECK_EQUAL(parser.sync_info().max_frame_size, 0);
  BOOST_CHECK_EQUAL(parser.header_size(), 0);

  BOOST_CHECK_EQUAL(parser.parse_header(0), false);
  BOOST_CHECK_EQUAL(parser.compare_headers(0, 0), false);

  BOOST_CHECK_EQUAL(parser.first_frame(0, 0), false);
  BOOST_CHECK_EQUAL(parser.next_frame(0, 0), false);

  BOOST_CHECK_EQUAL(parser.in_sync(), false);
  BOOST_CHECK_EQUAL(parser.frame_info().is_good(), false);
  BOOST_CHECK_EQUAL(parser.stream_info(), string());
}

BOOST_AUTO_TEST_CASE(init_constructor1)
{
  MultiFrameParser parser(parsers_with_null, array_size(parsers_with_null));

  MultiFrameParser::list_t list = parser.get_parsers();
  BOOST_REQUIRE_EQUAL(list.size(), array_size(parsers));
  for (int i = 0; i < array_size(parsers); i++)
    BOOST_CHECK_EQUAL(parsers[i], list[i]);
}

BOOST_AUTO_TEST_CASE(init_constructor2)
{
  MultiFrameParser parser(MultiFrameParser::list_t(parsers_with_null, parsers_with_null + array_size(parsers_with_null)));

  MultiFrameParser::list_t list = parser.get_parsers();
  BOOST_REQUIRE_EQUAL(list.size(), array_size(parsers));
  for (size_t i = 0; i < array_size(parsers); i++)
    BOOST_CHECK_EQUAL(parsers[i], list[i]);
}

BOOST_AUTO_TEST_CASE(set_parsers1)
{
  MultiFrameParser parser;
  parser.set_parsers(parsers_with_null, array_size(parsers_with_null));

  MultiFrameParser::list_t list = parser.get_parsers();
  BOOST_REQUIRE_EQUAL(list.size(), array_size(parsers));
  for (size_t i = 0; i < array_size(parsers); i++)
    BOOST_CHECK_EQUAL(parsers[i], list[i]);
}

BOOST_AUTO_TEST_CASE(set_parsers2)
{
  MultiFrameParser parser;
  parser.set_parsers(MultiFrameParser::list_t(parsers_with_null, parsers_with_null + array_size(parsers_with_null)));

  MultiFrameParser::list_t list = parser.get_parsers();
  BOOST_REQUIRE_EQUAL(list.size(), array_size(parsers));
  for (size_t i = 0; i < array_size(parsers); i++)
    BOOST_CHECK_EQUAL(parsers[i], list[i]);
}

BOOST_AUTO_TEST_CASE(release_parsers)
{
  MultiFrameParser parser(parsers, array_size(parsers));
  parser.release_parsers();
  BOOST_CHECK_EQUAL(parser.get_parsers().size(), 0);
}

BOOST_AUTO_TEST_CASE(add_remove)
{
  MultiFrameParser parser;

  parser.add_parser(0);
  BOOST_CHECK_EQUAL(parser.get_parsers().size(), 0);

  parser.add_parser(&ac3);
  BOOST_CHECK_EQUAL(parser.get_parsers().size(), 1);
  BOOST_CHECK(parser.can_parse(FORMAT_AC3));
  BOOST_CHECK(!parser.can_parse(FORMAT_DTS));

  parser.add_parser(&dts);
  BOOST_CHECK_EQUAL(parser.get_parsers().size(), 2);
  BOOST_CHECK(parser.can_parse(FORMAT_AC3));
  BOOST_CHECK(parser.can_parse(FORMAT_DTS));

  parser.remove_parser(&ac3);
  BOOST_CHECK_EQUAL(parser.get_parsers().size(), 1);
  BOOST_CHECK(!parser.can_parse(FORMAT_AC3));
  BOOST_CHECK(parser.can_parse(FORMAT_DTS));

  parser.remove_parser(&dts);
  BOOST_CHECK_EQUAL(parser.get_parsers().size(), 0);
  BOOST_CHECK(!parser.can_parse(FORMAT_AC3));
  BOOST_CHECK(!parser.can_parse(FORMAT_DTS));
}

BOOST_AUTO_TEST_CASE(sync_info)
{
  MultiFrameParser parser(parsers, array_size(parsers));

  SyncTrie sync_trie = parsers[0]->sync_info().sync_trie;
  size_t min_frame_size = parsers[0]->sync_info().min_frame_size;
  size_t max_frame_size = parsers[0]->sync_info().max_frame_size;
  for (size_t i = 1; i < array_size(parsers); i++)
  {
    sync_trie |= parsers[i]->sync_info().sync_trie;
    if (min_frame_size > parsers[i]->sync_info().min_frame_size)
      min_frame_size = parsers[i]->sync_info().min_frame_size;

    if (max_frame_size < parsers[i]->sync_info().max_frame_size)
      max_frame_size = parsers[i]->sync_info().max_frame_size;
  }
  sync_trie.optimize();

  BOOST_CHECK_EQUAL(sync_trie.serialize(), parser.sync_info().sync_trie.serialize());
  BOOST_CHECK_EQUAL(min_frame_size, parser.sync_info().min_frame_size);
  BOOST_CHECK_EQUAL(max_frame_size, parser.sync_info().max_frame_size);
}

BOOST_AUTO_TEST_CASE(header_size)
{
  MultiFrameParser parser(parsers, array_size(parsers));
  size_t header_size = parsers[0]->header_size();
  for (size_t i = 1; i < array_size(parsers); i++)
    if (header_size < parsers[i]->header_size())
      header_size = parsers[i]->header_size();
  BOOST_CHECK_EQUAL(header_size, parser.header_size());
}

BOOST_AUTO_TEST_CASE(parse_header)
{
  MultiFrameParser parser(parsers, array_size(parsers));
  for (int i = 0; i < array_size(parsers); i++)
  {
    MemFile f(files[i]);
    BOOST_REQUIRE(f);
    BOOST_CHECK(parser.parse_header(f));
  }
}

BOOST_AUTO_TEST_CASE(compare_headers)
{
  MultiFrameParser parser(parsers, array_size(parsers));
  for (int i = 0; i < array_size(parsers); i++)
    for (int j = 0; j < array_size(parsers); j++)
    {
      MemFile f1(files[i]);
      MemFile f2(files[j]);
      BOOST_REQUIRE(f1);
      BOOST_REQUIRE(f2);
      BOOST_CHECK_EQUAL(parser.compare_headers(f1, f2), i == j);
    }
}

BOOST_AUTO_TEST_SUITE_END()
