/*
  MultiHeader class test
*/

#include <boost/test/unit_test.hpp>
#include "parsers/multi_header.h"
#include "parsers/ac3/ac3_header.h"
#include "parsers/dts/dts_header.h"
#include "parsers/mpa/mpa_header.h"
#include "auto_file.h"

static const HeaderParser *parsers[] = { &ac3_header, &dts_header, &mpa_header };
static const char *files[] = { "a.ac3.03f.ac3", "a.dts.03f.dts", "a.mp2.005.mp2" };

BOOST_AUTO_TEST_SUITE(multi_header)

BOOST_AUTO_TEST_CASE(constructor)
{
  MultiHeader parser;

  BOOST_CHECK_EQUAL(parser.get_parsers().size(), 0);
  BOOST_CHECK_EQUAL(parser.header_size(), 0);
  BOOST_CHECK_EQUAL(parser.min_frame_size(), 0);
  BOOST_CHECK_EQUAL(parser.max_frame_size(), 0);

  BOOST_CHECK_EQUAL(parser.parse_header(0), false);
  BOOST_CHECK_EQUAL(parser.compare_headers(0, 0), false);
  BOOST_CHECK_EQUAL(parser.header_info(0), string());
}

BOOST_AUTO_TEST_CASE(init_constructor1)
{
  MultiHeader parser(parsers, array_size(parsers));

  MultiHeader::list_t list = parser.get_parsers();
  BOOST_REQUIRE_EQUAL(list.size(), array_size(parsers));
  for (int i = 0; i < array_size(parsers); i++)
    BOOST_CHECK_EQUAL(parsers[i], list[i]);
}

BOOST_AUTO_TEST_CASE(init_constructor2)
{
  MultiHeader parser(MultiHeader::list_t(parsers, parsers + array_size(parsers)));

  MultiHeader::list_t list = parser.get_parsers();
  BOOST_REQUIRE_EQUAL(list.size(), array_size(parsers));
  for (size_t i = 0; i < array_size(parsers); i++)
    BOOST_CHECK_EQUAL(parsers[i], list[i]);
}

BOOST_AUTO_TEST_CASE(set_parsers1)
{
  MultiHeader parser;
  parser.set_parsers(parsers, array_size(parsers));

  MultiHeader::list_t list = parser.get_parsers();
  BOOST_REQUIRE_EQUAL(list.size(), array_size(parsers));
  for (size_t i = 0; i < array_size(parsers); i++)
    BOOST_CHECK_EQUAL(parsers[i], list[i]);
}

BOOST_AUTO_TEST_CASE(set_parsers2)
{
  MultiHeader parser;
  parser.set_parsers(MultiHeader::list_t(parsers, parsers + array_size(parsers)));

  MultiHeader::list_t list = parser.get_parsers();
  BOOST_REQUIRE_EQUAL(list.size(), array_size(parsers));
  for (size_t i = 0; i < array_size(parsers); i++)
    BOOST_CHECK_EQUAL(parsers[i], list[i]);
}

BOOST_AUTO_TEST_CASE(release_parsers)
{
  MultiHeader parser(parsers, array_size(parsers));
  parser.release_parsers();
  BOOST_CHECK_EQUAL(parser.get_parsers().size(), 0);
}

BOOST_AUTO_TEST_CASE(add_remove)
{
  MultiHeader parser;

  parser.add_parser(&ac3_header);
  BOOST_CHECK_EQUAL(parser.get_parsers().size(), 1);
  BOOST_CHECK(parser.can_parse(FORMAT_AC3));
  BOOST_CHECK(!parser.can_parse(FORMAT_DTS));

  parser.add_parser(&dts_header);
  BOOST_CHECK_EQUAL(parser.get_parsers().size(), 2);
  BOOST_CHECK(parser.can_parse(FORMAT_AC3));
  BOOST_CHECK(parser.can_parse(FORMAT_DTS));

  parser.remove_parser(&ac3_header);
  BOOST_CHECK_EQUAL(parser.get_parsers().size(), 1);
  BOOST_CHECK(!parser.can_parse(FORMAT_AC3));
  BOOST_CHECK(parser.can_parse(FORMAT_DTS));

  parser.remove_parser(&dts_header);
  BOOST_CHECK_EQUAL(parser.get_parsers().size(), 0);
  BOOST_CHECK(!parser.can_parse(FORMAT_AC3));
  BOOST_CHECK(!parser.can_parse(FORMAT_DTS));
}

BOOST_AUTO_TEST_CASE(header_size)
{
  MultiHeader parser(parsers, array_size(parsers));
  size_t header_size = parsers[0]->header_size();
  for (size_t i = 1; i < array_size(parsers); i++)
    if (header_size < parsers[i]->header_size())
      header_size = parsers[i]->header_size();
  BOOST_CHECK_EQUAL(header_size, parser.header_size());
}

BOOST_AUTO_TEST_CASE(min_frame_size)
{
  MultiHeader parser(parsers, array_size(parsers));
  size_t min_frame_size = parsers[0]->min_frame_size();
  for (size_t i = 1; i < array_size(parsers); i++)
    if (min_frame_size > parsers[i]->min_frame_size())
      min_frame_size = parsers[i]->min_frame_size();
  BOOST_CHECK_EQUAL(min_frame_size, parser.min_frame_size());
}

BOOST_AUTO_TEST_CASE(max_frame_size)
{
  MultiHeader parser(parsers, array_size(parsers));
  size_t max_frame_size = parsers[0]->max_frame_size();
  for (size_t i = 1; i < array_size(parsers); i++)
    if (max_frame_size < parsers[i]->max_frame_size())
      max_frame_size = parsers[i]->max_frame_size();
  BOOST_CHECK_EQUAL(max_frame_size, parser.max_frame_size());
}

BOOST_AUTO_TEST_CASE(parse_header)
{
  MultiHeader parser(parsers, array_size(parsers));
  for (int i = 0; i < array_size(parsers); i++)
  {
    MemFile f(files[i]);
    BOOST_CHECK(parser.parse_header(f));
  }
}

BOOST_AUTO_TEST_CASE(compare_headers)
{
  MultiHeader parser(parsers, array_size(parsers));
  for (int i = 0; i < array_size(parsers); i++)
    for (int j = 0; j < array_size(parsers); j++)
    {
      MemFile f1(files[i]);
      MemFile f2(files[j]);
      BOOST_CHECK_EQUAL(parser.compare_headers(f1, f2), i == j);
    }
}

BOOST_AUTO_TEST_SUITE_END()
