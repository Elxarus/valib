/*
  Log test
*/

#include <boost/test/unit_test.hpp>
#include "log.h"

using std::string;
BOOST_TEST_DONT_PRINT_LOG_VALUE(LogEntry);

class LogTest : public LogSink
{
public:
  LogEntry last_entry;

  LogTest(LogDispatcher *source = 0): LogSink(source)
  {}

  virtual void receive(const LogEntry &entry)
  { last_entry = entry; }
};

static const LogEntry entry1(1, 2, "module1", "test1");
static const LogEntry entry2(2, 1, "module2", "test2");
static const char *log_test_file = "log_test_file.log";

BOOST_AUTO_TEST_SUITE(test_log)

BOOST_AUTO_TEST_CASE(sink_constructor)
{
  LogDispatcher source;

  LogTest *sink = new LogTest(&source);
  BOOST_CHECK(source.is_subscribed(sink));
  BOOST_CHECK(sink->is_subscribed());
  source.log(entry1);
  BOOST_CHECK(sink->last_entry == entry1);
  delete sink;

  // Now sink is destructed
  // Source must be unsubscribed
  BOOST_CHECK(!source.is_subscribed(sink));
  source.log(entry2);
}

BOOST_AUTO_TEST_CASE(destruct_dispatcher_first)
{
  LogTest sink;

  LogDispatcher *source = new LogDispatcher();
  sink.subscribe(source);
  BOOST_CHECK(source->is_subscribed(&sink));
  BOOST_CHECK(sink.is_subscribed());
  source->log(entry1);
  BOOST_CHECK(sink.last_entry == entry1);
  delete source;

  // Now sink is destructed
  // Source must be unsubscribed
  BOOST_CHECK(!sink.is_subscribed());
}

BOOST_AUTO_TEST_CASE(subscribe)
{
  LogDispatcher source;
  LogTest sink;

  sink.subscribe(&source);
  BOOST_CHECK(source.is_subscribed(&sink));
  BOOST_CHECK(sink.is_subscribed());
  source.log(entry1);
  BOOST_CHECK(sink.last_entry == entry1);

  sink.unsubscribe();
  BOOST_CHECK(!source.is_subscribed(&sink));
  BOOST_CHECK(!sink.is_subscribed());
  source.log(entry2);
  BOOST_CHECK(sink.last_entry == entry1);
}

BOOST_AUTO_TEST_CASE(log_funcs)
{
  LogDispatcher source;
  LogTest sink(&source);

  source.log(entry1);
  BOOST_CHECK_EQUAL(sink.last_entry, entry1);
  source.log(entry2);
  BOOST_CHECK_EQUAL(sink.last_entry, entry2);

  source.log(entry1.level, entry1.module, entry1.message);
  BOOST_CHECK(local_time() - sink.last_entry.timestamp < 1);
  BOOST_CHECK_EQUAL(sink.last_entry.level, entry1.level);
  BOOST_CHECK_EQUAL(sink.last_entry.module, entry1.module);
  BOOST_CHECK_EQUAL(sink.last_entry.message, entry1.message);

  source.log(entry2.level, entry2.module, entry2.message.c_str());
  BOOST_CHECK(local_time() - sink.last_entry.timestamp < 1);
  BOOST_CHECK_EQUAL(sink.last_entry.level, entry2.level);
  BOOST_CHECK_EQUAL(sink.last_entry.module, entry2.module);
  BOOST_CHECK_EQUAL(sink.last_entry.message, entry2.message);

  // formatted string
  source.log(entry1.level, entry1.module, "%s%i", "test", 1);
  BOOST_CHECK_EQUAL(sink.last_entry.message, string("test1"));

  // very long string
  string s = string("L") + string(8192, 'o') + string("ngcat");
  source.log(entry1.level, entry1.module, "%s", s.c_str());
  BOOST_CHECK_EQUAL(sink.last_entry.message, s);
}

BOOST_AUTO_TEST_CASE(valib_log)
{
  LogTest sink(&valib_log_dispatcher);

  ::valib_log(entry1.level, entry1.module, entry1.message);
  BOOST_CHECK(local_time() - sink.last_entry.timestamp < 1);
  BOOST_CHECK_EQUAL(sink.last_entry.level, entry1.level);
  BOOST_CHECK_EQUAL(sink.last_entry.module, entry1.module);
  BOOST_CHECK_EQUAL(sink.last_entry.message, entry1.message);

  ::valib_log(entry2.level, entry2.module, entry2.message.c_str());
  BOOST_CHECK(local_time() - sink.last_entry.timestamp < 1);
  BOOST_CHECK_EQUAL(sink.last_entry.level, entry2.level);
  BOOST_CHECK_EQUAL(sink.last_entry.module, entry2.module);
  BOOST_CHECK_EQUAL(sink.last_entry.message, entry2.message);
}

BOOST_AUTO_TEST_CASE(log_file)
{
  LogDispatcher source;
  LogFile sink(log_test_file, &source);
  source.log(entry1);
  sink.close();

  MemFile f(log_test_file);
  string s((const char *)(uint8_t *)f, f.size());
  s.resize(s.find_first_of("\n\r"));
  BOOST_CHECK_EQUAL(s, entry1.print());

  remove(log_test_file);
}

BOOST_AUTO_TEST_SUITE_END()
