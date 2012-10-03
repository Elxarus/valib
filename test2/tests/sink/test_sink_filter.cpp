/*
  SinkFilter class test
*/

#include <boost/test/unit_test.hpp>
#include "filters/log_filter.h"
#include "sink/sink_filter.h"
#include "sink/sink_log.h"
#include "../../suite.h"

static const Speakers spk1(FORMAT_LINEAR, MODE_5_1, 48000);
static const Speakers spk1_out(FORMAT_LINEAR, MODE_5_1, 96000);
static const Speakers spk2(FORMAT_RAWDATA, MODE_STEREO, 44100);
static uint8_t rawdata[] = { 0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7 };

BOOST_AUTO_TEST_SUITE(sink_filter)

// For testing purposes filter should change the output format
// and set new stream flag.
class FormatChangeFilter : public LogFilter
{
public:
  FormatChangeFilter()
  { new_stream_state = state_none; }

  void start_new_stream()
  { new_stream_state = state_send; }

  virtual Speakers get_output() const
  {
    Speakers spk2 = LogFilter::get_output();
    spk2.sample_rate *= 2;
    return spk2;
  }

  virtual bool process(Chunk &in, Chunk &out)
  {
    if (new_stream_state == state_send)
      new_stream_state = state_clear;
    else if (new_stream_state == state_clear)
      new_stream_state = state_none;
    return LogFilter::process(in, out);
  }

  virtual bool new_stream() const
  { return new_stream_state != state_none; }

protected:
  enum { state_none, state_send, state_clear } new_stream_state;
};

// Sink that accepts single format only
class FixedFormatSink : public LoggerSink
{
public:
  Speakers acceptable_spk;
  FixedFormatSink(Speakers spk): acceptable_spk(spk)
  {}

  virtual bool can_open(Speakers spk) const
  { return spk == acceptable_spk; }
};

// Filter that accepts single format only
class FixedFormatFilter : public FormatChangeFilter
{
public:
  Speakers acceptable_spk;
  FixedFormatFilter(Speakers spk): acceptable_spk(spk)
  {}

  virtual bool can_open(Speakers spk) const
  { return spk == acceptable_spk; }
};

BOOST_AUTO_TEST_CASE(constructor)
{
  const Speakers spk(FORMAT_LINEAR, MODE_STEREO, 48000);

  SinkFilter test;
  BOOST_CHECK(test.get_sink() == 0);
  BOOST_CHECK(test.get_filter() == 0);
  BOOST_CHECK(!test.is_open());
}

BOOST_AUTO_TEST_CASE(init_constructor)
{
  LoggerSink sink;
  FormatChangeFilter filter;
  SinkFilter test(&sink, &filter);

  BOOST_CHECK(test.get_sink() == &sink);
  BOOST_CHECK(test.get_filter() == &filter);
  BOOST_CHECK(!test.is_open());
}

BOOST_AUTO_TEST_CASE(set_release)
{
  FormatChangeFilter filter;
  LoggerSink sink;
  SinkFilter test;

  // Closed sink only
  test.set(&sink, 0);
  BOOST_CHECK(test.get_sink() == &sink);
  BOOST_CHECK(test.get_filter() == 0);
  BOOST_CHECK(!test.is_open());

  BOOST_CHECK_EQUAL(sink.print(), "");

  // Open sink only
  sink.open(spk1);
  sink.log.clear();
  test.set(&sink, 0);
  BOOST_CHECK(test.get_sink() == &sink);
  BOOST_CHECK(test.get_filter() == 0);
  BOOST_CHECK(test.is_open());
  BOOST_CHECK_EQUAL(test.get_input(), spk1);

  BOOST_CHECK_EQUAL(sink.print(), "");

  // Closed filter only
  test.set(0, &filter);
  BOOST_CHECK(test.get_sink() == 0);
  BOOST_CHECK(test.get_filter() == &filter);
  BOOST_CHECK(!test.is_open());

  BOOST_CHECK_EQUAL(filter.print(), "");

  // Open filter only
  filter.open(spk1);
  filter.log.clear();
  test.set(0, &filter);
  BOOST_CHECK(test.get_sink() == 0);
  BOOST_CHECK(test.get_filter() == &filter);
  BOOST_CHECK(!test.is_open());

  BOOST_CHECK_EQUAL(filter.print(), "");

  // Closed filter and closed sink
  sink.close();
  sink.log.clear();
  filter.close();
  filter.log.clear();
  test.set(&sink, &filter);
  BOOST_CHECK(test.get_sink() == &sink);
  BOOST_CHECK(test.get_filter() == &filter);
  BOOST_CHECK(!test.is_open());

  BOOST_CHECK_EQUAL(sink.print(), "");
  BOOST_CHECK_EQUAL(filter.print(), "");

  // Open filter and closed sink
  // SinkFilter must open the sink and do not touch the filter
  sink.close();
  sink.log.clear();
  filter.open(spk1);
  filter.log.clear();
  test.set(&sink, &filter);
  BOOST_CHECK(test.get_sink() == &sink);
  BOOST_CHECK(test.get_filter() == &filter);
  BOOST_CHECK(test.is_open());
  BOOST_CHECK_EQUAL(test.get_input(), spk1);

  BOOST_CHECK_EQUAL(sink.print(), "open(Linear PCM 5.1 96000)");
  BOOST_CHECK_EQUAL(filter.print(), "");

  // Closed filter and open sink
  filter.close();
  filter.log.clear();
  sink.open(spk2);
  sink.log.clear();
  test.set(&sink, &filter);
  BOOST_CHECK(test.get_sink() == &sink);
  BOOST_CHECK(test.get_filter() == &filter);
  BOOST_CHECK(!test.is_open());

  BOOST_CHECK_EQUAL(sink.print(), "");
  BOOST_CHECK_EQUAL(filter.print(), "");

  // Open filter and open sink with matched formats
  // Do not reopen the sink
  filter.open(spk1);
  filter.log.clear();
  sink.open(spk1_out);
  sink.log.clear();
  test.set(&sink, &filter);
  BOOST_CHECK(test.get_sink() == &sink);
  BOOST_CHECK(test.get_filter() == &filter);
  BOOST_CHECK(test.is_open());
  BOOST_CHECK_EQUAL(test.get_input(), spk1);

  BOOST_CHECK_EQUAL(sink.print(), "");
  BOOST_CHECK_EQUAL(filter.print(), "");

  // Open filter and open sink with mismatched formats
  // SinkFilter must reopen the sink with the correct format.
  filter.open(spk1);
  filter.log.clear();
  sink.open(spk2);
  sink.log.clear();
  test.set(&sink, &filter);
  BOOST_CHECK(test.get_sink() == &sink);
  BOOST_CHECK(test.get_filter() == &filter);
  BOOST_CHECK(test.is_open());
  BOOST_CHECK_EQUAL(test.get_input(), spk1);

  BOOST_CHECK_EQUAL(sink.print(), "open(Linear PCM 5.1 96000)");
  BOOST_CHECK_EQUAL(filter.print(), "");

  // Release using set(0, 0)
  filter.log.clear();
  sink.log.clear();
  test.set(&sink, &filter);
  test.set(0, 0);
  BOOST_CHECK(test.get_sink() == 0);
  BOOST_CHECK(test.get_filter() == 0);
  BOOST_CHECK(!test.is_open());

  BOOST_CHECK_EQUAL(sink.print(), "");
  BOOST_CHECK_EQUAL(filter.print(), "");

  // Release using release()
  filter.log.clear();
  sink.log.clear();
  test.set(&sink, &filter);
  test.set(0, 0);
  BOOST_CHECK(test.get_sink() == 0);
  BOOST_CHECK(test.get_filter() == 0);
  BOOST_CHECK(!test.is_open());

  BOOST_CHECK_EQUAL(sink.print(), "");
  BOOST_CHECK_EQUAL(filter.print(), "");
}

BOOST_AUTO_TEST_CASE(set_throw)
{
  // SinkFilter::set() may throw if filter's
  // output format is not compatible with sink
  FixedFormatFilter filter(spk1);
  FixedFormatSink sink(spk2);
  SinkFilter test;

  filter.open(spk1);
  BOOST_CHECK_THROW(test.set(&sink, &filter), EOpenSink);
  BOOST_CHECK(test.get_sink() == 0);
  BOOST_CHECK(test.get_filter() == 0);
}

BOOST_AUTO_TEST_CASE(can_open)
{
  FixedFormatFilter filter(spk1);
  FixedFormatSink sink(spk1_out);
  SinkFilter test;

  // No filter, No sink
  test.set(0, 0);
  BOOST_CHECK(!test.can_open(spk1));

  // Filter only
  test.set(0, &filter);
  BOOST_CHECK(!test.can_open(spk1));

  // Sink only
  test.set(&sink, 0);
  BOOST_CHECK(test.can_open(spk1_out));
  BOOST_CHECK(!test.can_open(spk1));

  // Filter and sink, compatible formats
  test.set(&sink, &filter);
  BOOST_CHECK(test.can_open(spk1));
  BOOST_CHECK(!test.can_open(spk2));

  // Filter and sink, incompatible formats
  // This test does not work because SinkFilter cannot
  // determine the output format of the filter at this stage.
/*
  sink.acceptable_spk = spk2;
  test.set(&sink, &filter);
  BOOST_CHECK(!test.can_open(spk1));
  BOOST_CHECK(!test.can_open(spk2));
*/
}

BOOST_AUTO_TEST_CASE(open)
{
  FixedFormatFilter filter(spk1);
  FixedFormatSink sink(spk1_out);
  SinkFilter test;

  // Sink only
  // Reopen the sink even when it's already open, full reinit may be nessesary.
  sink.open(spk1_out);
  sink.log.clear();
  test.set(&sink, 0);
  BOOST_CHECK(test.open(spk1_out));
  BOOST_CHECK(test.is_open());
  BOOST_CHECK_EQUAL(test.get_input(), spk1_out);
  BOOST_CHECK_EQUAL(sink.print(), "open(Linear PCM 5.1 96000)");

  // Sink only, bad format
  sink.log.clear();
  test.set(&sink, 0);
  BOOST_CHECK(!test.open(spk2));
  BOOST_CHECK_EQUAL(sink.print(), "open(Raw data Stereo 44100)");

  // Filter only
  // Do not open and do not touch the filter.
  filter.log.clear();
  test.set(0, &filter);
  BOOST_CHECK(!test.open(spk1));
  BOOST_CHECK_EQUAL(filter.print(), "");

  // Open filter and open sink
  // Reopen both, full reinit may be nessesary
  sink.open(spk1_out);
  sink.log.clear();
  filter.open(spk1);
  filter.log.clear();
  test.set(&sink, &filter);
  BOOST_CHECK(test.open(spk1));
  BOOST_CHECK(test.is_open());
  BOOST_CHECK_EQUAL(test.get_input(), spk1);
  BOOST_CHECK_EQUAL(filter.print(), "open(Linear PCM 5.1 48000)");
  BOOST_CHECK_EQUAL(sink.print(), "open(Linear PCM 5.1 96000)");

  // Bad format for filter
  sink.log.clear();
  filter.log.clear();
  BOOST_CHECK(!test.open(spk2));
  BOOST_CHECK_EQUAL(filter.print(), "open(Raw data Stereo 44100)");
  BOOST_CHECK_EQUAL(sink.print(), "");

  // Good format for filter, bad format for sink
  sink.acceptable_spk = spk2;
  sink.log.clear();
  filter.log.clear();
  BOOST_CHECK(!test.open(spk1));
  BOOST_CHECK_EQUAL(filter.print(), "open(Linear PCM 5.1 48000)");
  BOOST_CHECK_EQUAL(sink.print(), "open(Linear PCM 5.1 96000)");
}

BOOST_AUTO_TEST_CASE(reset_close)
{
  FormatChangeFilter filter;
  LoggerSink sink;
  SinkFilter test;

  // Sink only
  sink.log.clear();
  test.set(&sink, 0);
  test.open(spk1);
  test.reset();
  test.close();
  BOOST_CHECK(!test.is_open());
  BOOST_CHECK_EQUAL(sink.print(), "open(Linear PCM 5.1 48000)\nreset()\nclose()");

  // Filter and sink
  filter.log.clear();
  sink.log.clear();
  test.set(&sink, &filter);
  test.open(spk1);
  test.reset();
  test.close();
  BOOST_CHECK(!test.is_open());
  BOOST_CHECK_EQUAL(filter.print(), "open(Linear PCM 5.1 48000)\nreset()\nclose()");
  BOOST_CHECK_EQUAL(sink.print(), "open(Linear PCM 5.1 96000)\nreset()\nclose()");
}

BOOST_AUTO_TEST_CASE(process_sink_only)
{
  LoggerSink sink;
  SinkFilter test(&sink, 0);

  test.open(spk1);
  test.process(Chunk(rawdata, array_size(rawdata)));
  test.flush();
  test.process(Chunk(rawdata, array_size(rawdata)));
  test.reset();
  test.process(Chunk(rawdata, array_size(rawdata)));
  test.close();

  string sink_log = 
"open(Linear PCM 5.1 48000)\n"
"process(size = 8, rawdata)\n"
"flush()\n"
"process(size = 8, rawdata)\n"
"reset()\n"
"process(size = 8, rawdata)\n"
"close()";

  BOOST_CHECK_EQUAL(sink.print(), sink_log);
}

BOOST_AUTO_TEST_CASE(process_sink_filter)
{
  LoggerSink sink;
  FormatChangeFilter filter;
  SinkFilter test(&sink, &filter);

  test.open(spk1);
  test.process(Chunk(rawdata, array_size(rawdata)));
  test.flush();
  test.process(Chunk(rawdata, array_size(rawdata)));
  filter.start_new_stream();
  test.process(Chunk(rawdata, array_size(rawdata)));
  test.reset();
  test.close();

  string filter_log =
"open(Linear PCM 5.1 48000)\n"
"process(size = 8, rawdata)\n"
"process(size = 0)\n"
"flush()\n"
"process(size = 8, rawdata)\n"
"process(size = 0)\n"
"process(size = 8, rawdata)\n"
"process(size = 0)\n"
"reset()\n"
"close()";

  string sink_log =
"open(Linear PCM 5.1 96000)\n"
"process(size = 8, rawdata)\n"
"flush()\n"
"process(size = 8, rawdata)\n"
"flush()\n"
"open(Linear PCM 5.1 96000)\n"
"process(size = 8, rawdata)\n"
"reset()\n"
"close()";

  BOOST_CHECK_EQUAL(filter.print(), filter_log);
  BOOST_CHECK_EQUAL(sink.print(), sink_log);
}

BOOST_AUTO_TEST_SUITE_END();
