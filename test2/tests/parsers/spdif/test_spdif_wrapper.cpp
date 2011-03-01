/*
  SPDIFWrapper test
*/

#include <boost/test/unit_test.hpp>
#include "parsers/spdif/spdif_header.h"
#include "parsers/spdif/spdif_wrapper.h"
#include "parsers/spdif/spdifable_header.h"
#include "source/file_parser.h"
#include "../../../suite.h"

BOOST_AUTO_TEST_SUITE(spdif_wrapper)

BOOST_AUTO_TEST_CASE(constructor)
{
  SPDIFWrapper spdifer;
}

BOOST_AUTO_TEST_CASE(parse)
{
  FileParser f_raw;
  f_raw.open_probe("a.mad.mix.mad", &spdifable_header);
  BOOST_REQUIRE(f_raw.is_open());

  FileParser f_spdif;
  f_spdif.open_probe("a.mad.mix.spdif", &spdif_header);
  BOOST_REQUIRE(f_spdif.is_open());

  SPDIFWrapper spdifer;
  compare(&f_raw, &spdifer, &f_spdif, 0);
}

BOOST_AUTO_TEST_CASE(streams_frames)
{
  FileParser f;
  f.open_probe("a.mad.mix.mad", &spdifable_header);
  BOOST_REQUIRE(f.is_open());

  SPDIFWrapper spdifer;
  spdifer.open(f.get_output());
  BOOST_CHECK(spdifer.is_open());

  Chunk chunk;
  int streams = 0;
  int frames = 0;
  while (f.get_chunk(chunk))
  {
    if (f.new_stream())
    {
      spdifer.open(f.get_output());
      BOOST_CHECK(spdifer.is_open());
    }
    while (spdifer.process(chunk, Chunk()))
    {
      if (spdifer.new_stream())
        streams++;
      frames++;
    }
  }

  BOOST_CHECK_EQUAL(streams, 7);
  BOOST_CHECK_EQUAL(frames, 4375);
}

BOOST_AUTO_TEST_SUITE_END()
