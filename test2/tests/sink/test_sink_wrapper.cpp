/*
  SinkWrapper test
*/

#include <boost/test/unit_test.hpp>
#include "sink.h"
#include "sink/sink_log.h"
#include "../../suite.h"

static const Speakers spk(FORMAT_LINEAR, MODE_STEREO, 48000);

BOOST_AUTO_TEST_SUITE(sink_wrapper)

class TestSink : public LoggerSink
{
public:
  TestSink()
  {}

  virtual string info() const
  { return "test info"; }

  // TestSinl class name includes boost private namespace.
  // Have to overload this to avoid boost dependence.
  virtual string name() const
  { return "TestSink"; }
};

BOOST_AUTO_TEST_CASE(constructor)
{
  SinkWrapper s;

  // Test default state. 
  // Sink must not crash on any call.

  BOOST_CHECK(!s.is_open());
  BOOST_CHECK_EQUAL(s.get_input(), spk_unknown);

  BOOST_CHECK(!s.can_open(spk));
  BOOST_CHECK(!s.open(spk));

  s.process(Chunk());
  s.flush();
  s.reset();
  s.close();

  // Memory leak is reported when any typeid() call is done.
  // Known bug of VS:
  // http://stackoverflow.com/questions/8308671/memory-leaks-after-using-typeinfoname
//  BOOST_CHECK_EQUAL(s.info(), string());
//  BOOST_CHECK_EQUAL(s.name(), string("SinkWrapper"));
}

BOOST_AUTO_TEST_CASE(init_constructor)
{
  TestSink log;
  SinkWrapper s(&log);

  // Test lifecycle.

  BOOST_CHECK(!s.is_open());
  BOOST_CHECK(s.can_open(spk));

  BOOST_CHECK(s.open(spk));
  BOOST_CHECK(s.is_open());
  BOOST_CHECK_EQUAL(s.get_input(), spk);

  s.process(Chunk());
  s.flush();
  s.reset();
  s.close();

  BOOST_CHECK(!s.is_open());

  string test_log = 
"open(Linear PCM Stereo 48000)\n"
"process(size = 0)\n"
"flush()\n"
"reset()\n"
"close()";

  BOOST_CHECK_EQUAL(log.print(), test_log);
  
  // Memory leak is reported when any typeid() call is done.
  // Known bug of VS:
  // http://stackoverflow.com/questions/8308671/memory-leaks-after-using-typeinfoname
//  BOOST_CHECK_EQUAL(s.info(), string("test info"));
//  BOOST_CHECK_EQUAL(s.name(), string("SinkWrapper/TestSink"));
}

BOOST_AUTO_TEST_SUITE_END()
