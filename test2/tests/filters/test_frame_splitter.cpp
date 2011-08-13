/*
  FrameSplitter test
*/

#include <boost/test/unit_test.hpp>
#include "filters/frame_splitter.h"
#include "parsers/ac3/ac3_header.h"
#include "source/raw_source.h"
#include "../../suite.h"

BOOST_AUTO_TEST_SUITE(frame_splitter)

BOOST_AUTO_TEST_CASE(constructor)
{
  FrameSplitter f;
  BOOST_CHECK(f.get_parser() == 0);
  BOOST_CHECK_EQUAL(f.get_frames(), 0);
  BOOST_CHECK(!f.can_open(Speakers(FORMAT_RAWDATA, 0, 0)));
}

BOOST_AUTO_TEST_CASE(init_constructor)
{
  FrameSplitter f(&ac3_header);
  BOOST_CHECK_EQUAL(f.get_parser(), &ac3_header);
  BOOST_CHECK_EQUAL(f.get_frames(), 0);
  BOOST_CHECK(f.can_open(Speakers(FORMAT_RAWDATA, 0, 0)));
  BOOST_CHECK(f.can_open(Speakers(FORMAT_AC3, 0, 0)));
}

BOOST_AUTO_TEST_CASE(set_parser)
{
  FrameSplitter f;
  f.set_parser(&ac3_header);
  BOOST_CHECK_EQUAL(f.get_parser(), &ac3_header);
  BOOST_CHECK_EQUAL(f.get_frames(), 0);
}

// Ensure, that get_chunk() returns frames
// Count frames and streams in a file
BOOST_AUTO_TEST_CASE(get_chunk)
{
  const Speakers spk(FORMAT_RAWDATA, 0, 0);
  Chunk raw, frame;

  RAWSource raw_file(spk, "a.ac3.mix.ac3");
  BOOST_REQUIRE(raw_file.is_open());

  FrameSplitter f(&ac3_header);
  f.open(spk);
  BOOST_REQUIRE(f.is_open());
  BOOST_CHECK(f.get_output().is_unknown());

  int streams = 0;
  int frames = 0;
  while (raw_file.get_chunk(raw))
  {
    while (f.process(raw, frame))
    {
      if (f.new_stream())
      {
        streams++;
        BOOST_CHECK_EQUAL(f.get_output().format, FORMAT_AC3);
      }
      if (!ac3_header.parse_header(frame.rawdata))
        BOOST_FAIL("Not a frame output");
      frames++;
    }
  }
  BOOST_CHECK_EQUAL(frames, 1500);
  BOOST_CHECK_EQUAL(streams, 3);
}

BOOST_AUTO_TEST_CASE(passthrough)
{
  RAWSource raw(Speakers(FORMAT_RAWDATA, 0, 0), "a.ac3.mix.ac3");
  RAWSource ref(Speakers(FORMAT_RAWDATA, 0, 0), "a.ac3.mix.ac3");
  FrameSplitter f(&ac3_header);
  compare(&raw, &f, &ref, 0);
}

BOOST_AUTO_TEST_SUITE_END()
