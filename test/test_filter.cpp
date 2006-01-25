/*
  General filters test

Creation
========
After creation the filter should be in uninitialized state and get_output() should
report spk_unknown.

  Filter *filter = create_filter();
  // ... setup filter parameters ...
  if (filter.get_input() != spk_unknown)
    * filter was created in initialized state.

Initialization
==============
Filter must be initialized by set_input() call. 

  // try to initialize the filter with an unsupported format

  if (filter->query_input(spk_unsupported))
    * query_input() lies
  else
  {
    if (filter->set_input(spk_unsupported))
      * set_input() set unsupported format

    if (filter->get_input() != spk_unknown)
      * filter was initialized with an unsupported format
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

Following buffer processing methods possible:
* In-place. Filter processes data in-place at buffers received with process()
  call. Therefore, upstream must provide buffer that we can safely change. 
  This allows to avoid data copy from buffer to buffer so speed up processing.
  But we must always remember about data references. After process() call
  filter may hold references to upstream data buffers so we must not alter it
  until the end of processing. 
* Immediate. If output data must be larger than input buffer input data is
  processed from input buffer to private output buffer and returned 
  immediately. If filter cannot store all processed data into its output 
  buffer it produces several output chunks. This method does not produce
  processing lag.
* Block buffering. To process data filter requires a block of data that is
  processed at once. So filter get input data until internal buffer fills up 
  and only after this generates output chunk. Also it may hold some amount of
  data in buffer to process next block. This method produces processing lag.
  Also this method requires flushing to release data locked in buffer and 
  correctly finish the stream.

Filters may use different processing models for different input formats. For
example if mixer does inplace processing if output number of channels is less
or equal to input number of channels (faster) and uses immediate buffering 
otherwise.

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


Reset
=====
Post-conditions:
* filter is empty
* filter does not contain data

Format change
=============

* Forced format change.
  Methods:
  * set_input()
  * process()
  New format:
  * same format (only set_input())
  * unsupported format
  * new format
  Filter state:
  * filter is empty and does not contain data
  * filter is empty but contains data (for block buffering filters)
  * filter is full (only set_input())
  * filter is in flushing state (only set_input())

  Total: 16 cases
  * set_input(): 12 cases
  * process(): 4 cases

* Flushing
  Methods:
  * flush() (not implemented yet)
  * process()
  New format: does not matter...
  Filter state:
  * filter is empty and does not contain data
  * filter is empty but contains data (for block buffering filters)
  * filter is full (only flush())
  * filter is in flushing state (only flush())

  Total: 2 cases

* Post-conditions check:
  * new format set (not set for unsupported format)
  * filter is empty
  * filter does not contain data

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

int test_rules(Log *log);
int test_rules_filter(Log *log, Filter *filter, const char *desc);

class FilterTester : public Filter
{
protected:
  Filter *filter;
  Log    *log;

  Speakers spk_input;  // input format
  bool     ofdd;       // output format is data-dependent
  Speakers spk_output; // output format

  void update_formats()
  {
    spk_input = filter->get_input();
    spk_output = filter->get_output();

    // output format may depend on data for some input formats
    // and may not depend for others so status may change...
    // see [f3] rule
    ofdd = (spk_output == spk_unknown);
  }

  void check_formats(const char *caller)
  {
    // check input format
    if (filter->get_input() != spk_input)
      log->err("[k2] %s: input format was illegaly changed", caller);
    spk_input = filter->get_input(); // suppress this error report afterwards

    // check output format
    if (!ofdd && (filter->get_output() != spk_output))
      log->err("[f2] %s: output format was illegaly changed", caller);
    spk_output = filter->get_output(); // suppress this error report afterwards

    // check unininitialized state
    if ((filter->get_input() == spk_unknown) && !filter->is_empty())
      log->err("[f5] %s: filter is not empty in uninitialized state");
  }

  void check_reset(const char *caller)
  {
    if (ofdd && (filter->get_output() != spk_unknown))
      log->err("[f1] %s: output format did not change to spk_unknown", caller);

    if (!filter->is_empty())
      log->err("[f5] %s: filter is not empty", caller);

    // todo: check buffered data
  }

public:
  FilterTester(Filter *_filter, Log *_log)
  {
    filter = _filter;
    log = _log;
    update_formats();
  }

  void reset()
  {
    check_formats("before reset()");

    filter->reset();

    check_reset("after reset()");
    check_formats("after reset()");
  }

  /////////////////////////////////////////////////////////
  // Sink interface

  bool query_input(Speakers _spk) const 
  {
    return filter->query_input(_spk);
  }

  bool set_input(Speakers _spk)
  {
    check_formats("before set_input()");

    bool query = filter->query_input(_spk);
    bool result = filter->set_input(_spk);

    if (query != result)
      log->err("[k3] set_input(): query_input() lies");

    if (result)
    {
      // after successful format change filter must 
      // update input and output formats
      if (filter->get_input() != _spk)
        log->err("[k4] set_input(): new format was not set");
      update_formats();
    }
    else
    {
      // if format change failed input and output must 
      // reamin unchanged or require initialization
      if (filter->get_input() == spk_unknown)
        // filter requires reinit so formats was changed
        update_formats();
      else
        // formats stay unchanged
        check_formats("set_input()");
    }

    // filter must reset in either case
    check_reset("set_input()"); 
    return result;
  }

  Speakers get_input() const
  {
    return filter->get_input();
  }

  bool process(const Chunk *_chunk)
  {
    // check input parameters
    if (!_chunk)
    {
      log->err("process(): null chunk pointer!!!");
      return false;
    }

    bool input_format_change = (_chunk->spk != filter->get_input());
    bool query = true;

    check_formats("before process()");

    if (input_format_change)
      query = filter->query_input(_chunk->spk);

    bool result = filter->process(_chunk);

    if (input_format_change)
    {
      if (query != result)
        log->err("[k3] process(): query_input() lies");

      if (result)
      {
        // successful format change
        // filter must update input and output formats
        if (filter->get_input() != _chunk->spk)
          log->err("[k4] process(): new format was not set");
        update_formats();
      }
      else
      {
        // if format change failed input and output must 
        // reamin unchanged or require initialization
        if (filter->get_input() == spk_unknown)
          update_formats();
        else
          check_formats("process()");
      }
    }
    else
      check_formats("after process()");

    return result;
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
    // check input parameters
    if (!_chunk)
    {
      log->err("get_chunk(): null chunk pointer!!!");
      return false;
    }

    // check filter state correctness
    // get_chunk() must be called only in full state
    if (filter->is_empty())
      log->err("get_chunk() is called in empty state");

    check_formats("before get_chunk()");

    Speakers spk = filter->get_output();

    if (!filter->get_chunk(_chunk))
      return false;

    if (_chunk->spk != spk)
      log->err("[s1] get_chunk(): get_output() lies");

    check_formats("after get_chunk()");

    return true;
  }
};


int test_rules(Log *log)
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

  log->open_group("Test filters");
  test_rules_filter(log, &null,    "NullFilter");
  test_rules_filter(log, &agc,     "AGC");
  test_rules_filter(log, &conv,    "Converter");
  test_rules_filter(log, &dec,     "AudioDecoder");
  test_rules_filter(log, &delay,   "Delay");
  test_rules_filter(log, &dvd,     "DVDDecoder");
  test_rules_filter(log, &chain,   "FilterChain");
  test_rules_filter(log, &levels,  "Levels");
  test_rules_filter(log, &proc,    "AudioProcessor");
  test_rules_filter(log, &demux,   "Demux");
  test_rules_filter(log, &spdifer, "Spdifer");
  return log->close_group();
}


static const int formats[] = 
{ 
  FORMAT_LINEAR,
  FORMAT_PCM16,
  FORMAT_PCM24,
  FORMAT_PCM32,
  FORMAT_PCMFLOAT,
  FORMAT_PCM16_LE,
  FORMAT_PCM24_LE,
  FORMAT_PCM32_LE,
  FORMAT_PCMFLOAT_LE,
  FORMAT_PES,
  FORMAT_SPDIF,
  FORMAT_AC3,
  FORMAT_MPA,
  FORMAT_DTS,
  FORMAT_AAC,
  FORMAT_OGG,
  FORMAT_UNKNOWN
};

static const int modes[] = 
{
  MODE_1_0,
  MODE_2_0,
  MODE_3_0,
  MODE_2_1,
  MODE_3_1,
  MODE_2_2,
  MODE_3_2,
  MODE_1_0_LFE,
  MODE_2_0_LFE,
  MODE_3_0_LFE,
  MODE_2_1_LFE,
  MODE_3_1_LFE,
  MODE_2_2_LFE,
  MODE_3_2_LFE,
  0 // unspecified mode
};

static const int sample_rates[] = 
{
  192000, 96000, 48000, 24000, 12000,
  176400, 88200, 44100, 22050, 11025, 
  128000, 64000, 32000, 16000, 8000,
  0 // unspecified sample rate
};

static const int n_formats = array_size(formats);
static const int n_modes = array_size(modes);
static const int n_sample_rates = array_size(sample_rates);

int test_rules_filter(Log *log, Filter *filter, const char *filter_name)
{
  FilterTester f(filter, log);
  log->open_group("Testing %s filter", filter_name);

  // Filter should be created in uninitialized state
  if (f.get_input() != spk_unknown)
    log->msg("Filter was created in initialized state");

  // Try to set different formats
  for (int i_format = 0; i_format < n_formats; i_format++)
    for (int i_mode = 0; i_mode < n_modes; i_mode++)
      for (int i_sample_rate = 0; i_sample_rate < n_sample_rates; i_sample_rate++)
        f.set_input(Speakers(formats[i_format], modes[i_mode], sample_rates[i_sample_rate]));

  return log->close_group();
}