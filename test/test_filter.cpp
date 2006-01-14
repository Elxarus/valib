/*
  General filters test

Creation
========
After creation filter should be in uninitialized state and get_output() should
report spk_unknown.

  Filter *filter = create_filter();
  // ... setup filter parameters ...
  if (filter.get_input() != spk_unknown)
    * filter created in initialized state.

Initialization
==============
Filter must be initialized by set_input() call. 

  // try to initialize filter by unsupported format

  if (filter->query_input(spk_unsupported))
    * query_input() lies
  else
  {
    if (filter->set_input(spk_unsupported))
      * set_input() set unsupported format

    if (filter->get_input() != spk_unknown)
      * filter was initialized by unsupported format
  }

  // initialize filter

  if (!filter->query_input(spk_init))
    * query_input() lies
  else
  {
    if (!filter->set_input(spk_init))
    {
      * set_input() cannot set supported format
      return; // cannot proceed further with an uninitialized filter
    }

    if (filter->get_input() != spk_init)
    {
      * filter was not initialized by set_input()
      return; // cannot proceed further with an uninitialized filter
    }

    if (!filter->is_empty())
    {
      * filter is not empty after initialization
      return; // cannot proceed further with a buggggy filter
    }

    // input format must not change until next format change
    spk_input = filter->get_input();
  }

Check output format
===================
After input format change output format must be either unitialized or equal to
spk_unknown if it depends on input data.

  if (filter.get_output() == spk_unknown)
  {
    // output format is data-dependent and may change during processing
    ofdd = true;
  }
  else
  {
    // output format cannot change during processing
    spk_output = filter.get_output();
  }

Check buffering
===============

Buffered processing may be of 2 kinds:
* Immediate. If output buffer must be larger than input buffer input data is
  processed from input buffer to private output buffer and returned 
  immediately. If filter cannot store all processed data into its output 
  buffer it generates several output chunks. This method does not produce 
  processing lag.
* Block buffering. To process data filter requires a block of data that is
  processed at once. So filter get input data until internal buffer fills up 
  and only after this generates output chunk. Also it may hold some amount of
  data in buffer to process next block. This method produces processing lag.
  Also this method requires flushing to release data locked in buffer and 
  correctly finish the stream.

Filters may use different processing models for different input formats. For
example if mixer does inplace processing if output number of channels is less
or equal to input number of channels and uses immediate buffering otherwise.

We can check buffering method used by following conditions:
* Filter that does in-place processing must return pointer to the input buffer.
* If filter does conversion between linear and rawdata it obviously cannot do
  it inplace and uses immediate buffering.
* If filter can process 1 byte/sample and generates output chunk immediately
  it uses immediate buffering. We can find buffer size by increasing input 
  chunk size by one sample/byte until filter to generate 2 output chunks for 1
  input chunk. Buffer size equals to input chunk size - 1.
* If filter remains empty after processing chunk with 1 byte/sample it uses 
  block buffering. We can find buffer size by sending 1 byte/sample chunks 
  until filter to generate output chunk. Buffer size equals to number of input
  chunks but size of output chunk may be less. Difference between buffer size
  and size of output chunk equals to size of data remains at buffer. This
  produces a lag between input and output.



Format change
=============
... kinds of format change ...

*/

#include "log.h"
#include "filters\agc.h"
#include "filters\convert.h"
#include "filters\decoder.h"
#include "filters\delay.h"
#include "filters\dvd_decoder.h"
#include "filters\filter_chain.h"
#include "filters\levels.h"
#include "filters\proc.h"
#include "filters\demux.h"
#include "filters\spdifer.h"

int test_filter(Log *log);
int test_empty_filter(Log *log, Filter *filter, const char *desc);

class FilterTester : public Filter
{
protected:
  Filter *filter;
  Log    *log;

  Speakers spk_input;  // input format
  bool     ofdd;       // output format is data-dependent
  Speakers spk_output; // output format

  void check_input(const char *caller)
  {
    if (filter->get_input() != spk_input)
      log->err("[k2] %s: input format illegaly changed", caller);

    spk_input = filter->get_input(); // suppress this error report afterwards
  }

  void check_output(const char *caller)
  {
    if (!ofdd && (filter->get_output() != spk_output))
      log->err("[s2] %s: output format illegaly changed", caller);

    spk_output = filter->get_output(); // suppress this error report afterwards
  }

public:
  FilterTest(Filter *filter, Log *log);

  void reset()
  {
    check_input("before reset()");
    check_output("before reset()");

    filter->reset();

    if (ofdd && (filter->get_output() != spk_unknown))
      log->err("[f1] reset(): output format did not change to spk_unknown");

    check_input("after reset()");
    check_output("after reset()");
  }

  /////////////////////////////////////////////////////////
  // Sink interface

  bool query_input(Speakers _spk) const 
  {
    return filter->query_input(_spk);
  }

  bool set_input(Speakers _spk)
  {
    check_input("before set_input()");
    check_output("before set_input()");

    bool query = filter->query_input(_spk);
    bool result = filter->set_input(_spk);

    if (query != result)
      log->err("[k3] set_input() and query_input() conflict");

    if (!result)
      return false;

    if (filter->get_input() != _spk)
      log->err("[k4] set_input() and get_input() conflict");

    spk_input = filter->get_input();
    spk_output = filter->get_output();

    // output format may depend on data for some input formats
    // and may not depend for others so status may change...
    ofdd = (spk_output == spk_unknown);

    return true;
  }

  Speakers get_input() const
  {
    return filter->get_input();
  }

  bool process(const Chunk *_chunk)
  {
    check_input("before process()");
    check_output("before process()");

    bool input_format_change = (_chunk->spk != filter->get_input());

    bool query = filter->query_input(_chunk->spk);

    bool result = filter->process(_chunk);

    if (query != result)
      log->err("[k3] process() and query_input() conflict");

    if (!result)
      return false;

    if (input_format_change)
    {
      if (_chunk->spk != filter->get_input())
        log->err("[k4] process() and get_input() conflict");

      spk_input = filter->get_input();
      spk_output = filter->get_output();

      // output format may depend on data for some input formats
      // and may not depend for others so status may change...
      // (see [f3] and [f1] rules)
      ofdd = (spk_output == spk_unknown);
    }
    else
    {
      check_input("after process()");
      check_output("after process()");
    }
    return true;
  }

  /////////////////////////////////////////////////////////
  // Source interface

  Speakers get_output() const
  {
    return filter->get_output();
  }

  bool is_empty() const
  {
    return filter->is_empty();
  }

  bool get_chunk(Chunk *_chunk)
  {
    check_input("before get_chunk()");
    check_output("before get_chunk()");

    Speakers spk = filter->get_output();

    if (!filter->get_chunk(_chunk))
      return false;

    if (_chunk->spk != spk)
      log->err("[s1] get_chunk() and get_output() conflict");

    check_input("after get_chunk()");
    check_output("after get_chunk()");

    return true;
  }
};


int test_empty(Log *log)
{
  NullFilter     null;
  AGC            agc;
  Converter      conv(2048);
  AudioDecoder   dec;
  Delay          delay;
  DVDDecoder     dvd;
  FilterChain    chain;
  Levels         levels;
  AudioProcessor proc(2048);
  Demux          demux;
  Spdifer        spdifer;

  log->open_group("Empty filters test");
  test_filter(log, &null,    "NullFilter");
  test_filter(log, &agc,     "AGC");
  test_filter(log, &conv,    "Converter");
  test_filter(log, &dec,     "AudioDecoder");
  test_filter(log, &delay,   "Delay");
  test_filter(log, &dvd,     "DVDDecoder");
  test_filter(log, &chain,   "FilterChain");
  test_filter(log, &levels,  "Levels");
  test_filter(log, &proc,    "AudioProcessor");
  test_filter(log, &demux,   "Demux");
  test_filter(log, &spdifer, "Spdifer");
  return log->close_group();
}

int test_empty_filter(Log *log, Filter *filter, const char *filter_name)
{
  // Test for:
  // * Filter should go to empty state after reset().
  // * Empty filter should accept empty chunks without error in process() call
  //   and remain in empty state.
  // * Empty filter should return empty chunks without error in get_chunk() 
  //   call and remain in empty state.
  // * Empty filter should go to flushing state after receiving empty 
  //   eos-chunk and report that it not empty.
  // * Empty flushing filter should return empty eos-chunk and go to empty 
  //   state.

  int i;
  Chunk chunk;

  log->msg("%s", filter_name);

  filter->reset();
  if (!filter->is_empty())
    return log->err("filter is not empty after reset()");

  for (i = 0; i < 10; i++)
  {
    ///////////////////////////////////////////////////////
    // Empty chunk processing

    chunk.set_empty(filter->get_input());

    if (!filter->process(&chunk))
      return log->err("process() of empty chunk failed");

    if (!filter->is_empty())
      return log->err("filter is not empty after receiving empty chunk");

    if (!filter->get_chunk(&chunk))
      return log->err("get_chunk() in empty state failed");

    if (!filter->is_empty())
      return log->err("filter is not empty after get_chunk()");

    if (!chunk.is_empty())
      return log->err("non-empty chunk returned");

    ///////////////////////////////////////////////////////
    // End-of-stream processing

    chunk.set_empty(filter->get_input(), 0, 0, true);

    if (!filter->process(&chunk))
      return log->err("process() of end-of-stream failed");

    if (filter->is_empty())
      return log->err("filter is empty after receiving end-of-stream");

    if (!filter->get_chunk(&chunk))
      return log->err("get_chunk() in flushing state failed");

    if (!filter->is_empty())
      return log->err("filter is not empty after get_chunk()");

    if (!chunk.is_empty())
      return log->err("non-empty chunk returned");

    if (!chunk.eos)
      return log->err("filter did not return end-of stream");
  }

  return 0;
}