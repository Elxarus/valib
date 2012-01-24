/*
  LogSink class test
*/

#include <boost/test/unit_test.hpp>
#include "sink/sink_log.h"

BOOST_AUTO_TEST_CASE(log_sink)
{
  uint8_t rawdata[] = { 0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7 };

  LogSink sink;
  BOOST_CHECK_EQUAL(sink.print(), "");

  sink.open(Speakers(FORMAT_LINEAR, MODE_STEREO, 48000));
  sink.open(Speakers(FORMAT_RAWDATA, 0, 0));
  sink.process(Chunk());
  sink.process(Chunk(true, 100));
  sink.reset();
  sink.process(Chunk(rawdata, array_size(rawdata)));
  sink.flush();
  sink.process(Chunk(rawdata, array_size(rawdata), true, 100));

  string result =
"open(Linear PCM Stereo 48000)\n"
"open(Raw data - 0)\n"
"process(size = 0)\n"
"process(size = 0, sync = true, time = 100)\n"
"reset()\n"
"process(size = 8, rawdata)\n"
"flush()\n"
"process(size = 8, rawdata, sync = true, time = 100)";

  BOOST_CHECK_EQUAL(sink.print(), result);
}
