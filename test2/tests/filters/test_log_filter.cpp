/*
  LogFilter class test
*/

#include <boost/test/unit_test.hpp>
#include "filters/log_filter.h"

BOOST_AUTO_TEST_CASE(log_filter)
{
  uint8_t rawdata[] = { 0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7 };

  LogFilter filter;
  BOOST_CHECK_EQUAL(filter.print(), "");

  filter.open(Speakers(FORMAT_LINEAR, MODE_STEREO, 48000));
  filter.open(Speakers(FORMAT_RAWDATA, 0, 0));
  filter.process(Chunk(), Chunk());
  filter.process(Chunk(true, 100), Chunk());
  filter.reset();
  filter.process(Chunk(rawdata, array_size(rawdata)), Chunk());
  filter.flush(Chunk());
  filter.process(Chunk(rawdata, array_size(rawdata), true, 100), Chunk());

  string result =
"open(Linear PCM Stereo 48000)\n"
"open(Raw data - 0)\n"
"process(size = 0)\n"
"process(size = 0, sync = true, time = 100)\n"
"reset()\n"
"process(size = 8, rawdata)\n"
"flush()\n"
"process(size = 8, rawdata, sync = true, time = 100)";

  BOOST_CHECK_EQUAL(filter.print(), result);
}
