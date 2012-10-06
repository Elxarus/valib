/*
  FilterWrapper test
*/

#include <boost/test/unit_test.hpp>
#include "filter.h"
#include "filters/log_filter.h"
#include "../../suite.h"

static const Speakers spk(FORMAT_LINEAR, MODE_STEREO, 48000);

BOOST_AUTO_TEST_SUITE(filter_wrapper)

class TestFilter : public LogFilter
{
public:
  TestFilter()
  {}

  virtual string info() const
  { return "test info"; }

  // TestFilter class name includes boost private namespace.
  // Have to overload this to avoid boost dependence.
  virtual string name() const
  { return "TestFilter"; }
};

BOOST_AUTO_TEST_CASE(constructor)
{
  FilterWrapper f;

  // Test default state. 
  // Filter must not crash on any call.

  BOOST_CHECK(!f.is_open());
  BOOST_CHECK_EQUAL(f.get_input(), spk_unknown);
  BOOST_CHECK_EQUAL(f.get_output(), spk_unknown);

  BOOST_CHECK(!f.can_open(spk));
  BOOST_CHECK(!f.open(spk));
  BOOST_CHECK(!f.process(Chunk(), Chunk()));
  BOOST_CHECK(!f.new_stream());
  BOOST_CHECK(!f.flush(Chunk()));
  f.reset();
  f.close();

  BOOST_CHECK_EQUAL(f.info(), string());
  BOOST_CHECK_EQUAL(f.name(), string("FilterWrapper"));
}

BOOST_AUTO_TEST_CASE(init_constructor)
{
  TestFilter log;
  FilterWrapper f(&log);

  // Test lifecycle.

  BOOST_CHECK(!f.is_open());
  BOOST_CHECK(f.can_open(spk));

  BOOST_CHECK(f.open(spk));
  BOOST_CHECK(f.is_open());
  BOOST_CHECK(!f.new_stream());
  BOOST_CHECK_EQUAL(f.get_input(), spk);
  BOOST_CHECK_EQUAL(f.get_output(), spk);

  f.process(Chunk(), Chunk());
  f.flush(Chunk());
  f.reset();
  f.close();
  BOOST_CHECK(!f.is_open());

  string test_log = 
"open(Linear PCM Stereo 48000)\n"
"process(size = 0)\n"
"flush()\n"
"reset()\n"
"close()";

  BOOST_CHECK_EQUAL(log.print(), test_log);

  BOOST_CHECK_EQUAL(f.info(), string("test info"));
  BOOST_CHECK_EQUAL(f.name(), string("FilterWrapper/TestFilter"));
}

BOOST_AUTO_TEST_SUITE_END()
