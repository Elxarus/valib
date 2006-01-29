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
  WRONG. FILTER MAY REDUCE DATA SIZE.


Reset
=====
Post-conditions:
* filter is empty
* filter does not contain data

Format change
=============

* Forced format change.
  Filter state:
  * filter is empty and does not contain data
  * filter is full (only set_input())
  * filter is cycled through full state to empty
  * ? filter is in flushing state (only set_input())
  Methods:
  * set_input()
  * process() with empty chunk
  * process() with data chunk
  New format:
  * same format (it is no format change for process())
  * new format
  * unsupported format

  Total: 16 cases
  * set_input(): 12 cases
  * process(): 4 cases

* Flushing
  Filter state:
  * filter is empty and does not contain data
  * filter is cycled through full state to empty
  * filter is full (only flush())
  * filter is in flushing state (only flush())
  Methods:
  * flush() (not implemented yet)
  * process() with empty chunk
  * process() with data chunk
  New format:
  * same format (it is no format change for process())
  * new format
  * unsupported format

  Total: 4 cases

* Post-conditions check:
  * new format set (not set for unsupported format)
  * filter is empty
  * filter does not contain data

*/

#include "log.h"
#include "filter_tester.h"
#include "test_source.h"

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

static const int formats[] = 
{ 
  FORMAT_UNKNOWN, // unspecified format
  FORMAT_LINEAR,
  FORMAT_PCM16, FORMAT_PCM24, FORMAT_PCM32, FORMAT_PCMFLOAT,
  FORMAT_PCM16_LE, FORMAT_PCM24_LE, FORMAT_PCM32_LE, FORMAT_PCMFLOAT_LE,
  FORMAT_PES, FORMAT_SPDIF,
  FORMAT_AC3, FORMAT_MPA, FORMAT_DTS, FORMAT_AAC, FORMAT_OGG
};

static const int modes[] = 
{
  0, // unspecified mode
  MODE_1_0,     MODE_2_0,     MODE_3_0,     MODE_2_1,     MODE_3_1,     MODE_2_2,     MODE_3_2,
  MODE_1_0_LFE, MODE_2_0_LFE, MODE_3_0_LFE, MODE_2_1_LFE, MODE_3_1_LFE, MODE_2_2_LFE, MODE_3_2_LFE
};

static const int sample_rates[] = 
{
  0, // unspecified sample rate
  192000, 96000, 48000, 24000, 12000,
  176400, 88200, 44100, 22050, 11025, 
  128000, 64000, 32000, 16000, 8000,
};

static const int n_formats = array_size(formats);
static const int n_modes = array_size(modes);
static const int n_sample_rates = array_size(sample_rates);




int test_rules(Log *log);
int test_rules_filter(Log *log, Filter *filter, const char *filter_name, 
  Speakers spk_supported1, const char *file_name1, 
  Speakers spk_supported2, const char *file_name2, 
  Speakers spk_unsupported);
int test_rules_filter_int(Log *log, Filter *filter,
  Speakers spk_supported, const char *filename, 
  Speakers spk_supported2, const char *filename2, 
  Speakers spk_unsupported);

int test_rules(Log *log)
{
  // Base filter
  NullFilter     null;

  // Rawdata filters
  Converter      conv_ll(2048);
  Converter      conv_lr(2048);
  Converter      conv_rl(2048);
  AudioDecoder   dec;
  Demux          demux;
  Spdifer        spdifer;

  // Processing filters
  AGC            agc;
  Delay          delay;
  Levels         levels;

  // Aggregate filters
  DVDDecoder     dvd;
  FilterChain    chain;
  AudioProcessor proc(2048);

  // Setup filters

  conv_ll.set_format(FORMAT_LINEAR);
  conv_lr.set_format(FORMAT_PCM16);
  conv_rl.set_format(FORMAT_LINEAR);

  log->open_group("Test filters");


  // Base filter

  test_rules_filter(log, &null,    "NullFilter", 
    Speakers(FORMAT_LINEAR, MODE_STEREO, 48000), 0,
    Speakers(FORMAT_LINEAR, MODE_5_1, 96000), 0,
    Speakers(FORMAT_AC3, MODE_STEREO, 48000));

  // Rawdata filters

  test_rules_filter(log, &conv_ll, "Converter linear->linear",
    Speakers(FORMAT_LINEAR, MODE_STEREO, 48000), 0,
    Speakers(FORMAT_LINEAR, MODE_5_1, 96000), 0,
    Speakers(FORMAT_AC3, MODE_STEREO, 48000));

  test_rules_filter(log, &conv_lr, "Converter linear->raw",
    Speakers(FORMAT_LINEAR, MODE_STEREO, 48000), 0,
    Speakers(FORMAT_LINEAR, MODE_5_1, 96000), 0,
    Speakers(FORMAT_AC3, MODE_STEREO, 48000));

  test_rules_filter(log, &conv_rl, "Converter raw->linear",
    Speakers(FORMAT_PCM16, MODE_STEREO, 48000), 0,
    Speakers(FORMAT_PCM32, MODE_5_1, 96000), 0,
    Speakers(FORMAT_AC3, MODE_STEREO, 48000));

  test_rules_filter(log, &dec, "AudioDecoder",
    Speakers(FORMAT_AC3, MODE_STEREO, 48000), "test.ac3",
    Speakers(FORMAT_AC3, 0, 0), "test.ac3",
    Speakers(FORMAT_LINEAR, MODE_STEREO, 48000));

  test_rules_filter(log, &demux, "Demuxer",
    Speakers(FORMAT_PES, 0, 0), "test.ac3.pes",
    Speakers(FORMAT_PES, MODE_STEREO, 48000), "test.ac3.pes",
    Speakers(FORMAT_LINEAR, MODE_STEREO, 48000));

  test_rules_filter(log, &spdifer, "Spdifer",
    Speakers(FORMAT_AC3, MODE_STEREO, 48000), "test.ac3",
    Speakers(FORMAT_AC3, 0, 0), "test.ac3",
    Speakers(FORMAT_LINEAR, MODE_STEREO, 48000));

  // Processing filters

  test_rules_filter(log, &agc,     "AGC",
    Speakers(FORMAT_LINEAR, MODE_STEREO, 48000), 0,
    Speakers(FORMAT_LINEAR, MODE_5_1, 96000), 0,
    Speakers(FORMAT_AC3, MODE_STEREO, 48000));

    
/*
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
*/
  return log->close_group();
}



int test_rules_filter(Log *log, Filter *filter, const char *filter_name, 
  Speakers spk_supported, const char *filename, 
  Speakers spk_supported2, const char *filename2, 
  Speakers spk_unsupported)
{
  const char *err = 0;
  log->open_group("Testing %s", filter_name);
  test_rules_filter_int(log, filter, 
    spk_supported, filename, 
    spk_supported2, filename2, 
    spk_unsupported);
  return log->close_group();
}


int test_rules_filter_int(Log *log, Filter *filter,
  Speakers spk_supported, const char *filename, 
  Speakers spk_supported2, const char *filename2, 
  Speakers spk_unsupported)
{
  const data_size = 8192;
  TestSource src;
  Chunk chunk;

  FilterTester f(filter, log);

  /////////////////////////////////////////////////////////
  // Most used operations 

  #define SET_INPUT_OK(spk, err_text)   \
    if (!f.set_input(spk))              \
      return log->err(err_text, spk.format_text(), spk.mode_text(), spk.sample_rate);

  #define SET_INPUT_FAIL(spk, err_text) \
    if (f.set_input(spk))               \
      return log->err(err_text, spk.format_text(), spk.mode_text(), spk.sample_rate);

  #define PROCESS_OK(chunk, err_text)   \
    if (!f.process(&chunk))             \
      return log->err(err_text, chunk.spk.format_text(), chunk.spk.mode_text(), chunk.spk.sample_rate);

  #define PROCESS_FAIL(chunk, err_text) \
    if (f.process(&chunk))              \
      return log->err(err_text, chunk.spk.format_text(), chunk.spk.mode_text(), chunk.spk.sample_rate);

  #define GET_CHUNK_OK(chunk, err_text) \
    if (!f.get_chunk(&chunk))           \
      return log->err(err_text);

  #define FILL_FILTER                   \
    while (f.is_empty())                \
    {                                   \
      src.get_chunk(&chunk);            \
      PROCESS_OK(chunk, "process(%s %s %i) failed"); \
    }

  #define EMPTY_FILTER                  \
    while (!f.is_empty())               \
      GET_CHUNK_OK(chunk, "get_chunk() failed");


  /////////////////////////////////////////////////////////
  // Check sources

  src.open(spk_supported, filename, data_size);
  if (!src.is_open())
    return log->err("Cannot open file %s", filename);

  if (src.is_empty())
    return log->err("Source is empty");

  if (!src.get_chunk(&chunk))
    return log->err("Cannot get chunk from source");

  if (chunk.spk != spk_supported)
    return log->err("Source has wrong fromat");

  if (chunk.size != data_size)
    return log->err("Source has wrong chunk size");

  src.open(spk_supported2, filename2, data_size);
  if (!src.is_open())
    return log->err("Cannot open file %s", filename2);

  if (src.is_empty())
    return log->err("Source2 is empty");

  if (!src.get_chunk(&chunk))
    return log->err("Cannot get chunk from source2");

  if (chunk.spk != spk_supported2)
    return log->err("Source2 has wrong format");

  if (chunk.size != data_size)
    return log->err("Source2 has wrong chunk size");

  /////////////////////////////////////////////////////////
  // Filter should be created in uninitialized state

  if (f.get_input() != spk_unknown)
    log->msg("Filter was created in initialized state");

  /////////////////////////////////////////////////////////
  // Check output format dependency

  SET_INPUT_OK(spk_supported, "Set format: %s %s %i failed");
  if (f.get_output().is_unknown())
    log->msg("Output format is data-dependent");

  /////////////////////////////////////////////////////////
  // Determine buffering

  {
    size_t size = 0;
    bool empty_chunks = false;
    bool buffering = false;

    src.open(spk_supported, filename, 1);
    src.get_chunk(&chunk);
    SET_INPUT_OK(spk_supported, "Set format: %s %s %i failed");
    PROCESS_OK(chunk, "process(%s %s %i) failed");

    // block-buffering filter may generate empty chunks
    // but it is bad strategy
    while (!f.is_empty())
    {
      GET_CHUNK_OK(chunk, "get_chunk() failed");
      size += chunk.size;
      if (chunk.is_empty())
        empty_chunks = true;
    }

    if (size == 0)
      log->msg("Filter possibly uses block buffering");

    if (empty_chunks)
      log->msg("Filter generates unnesesary empty chunks");
  }

  /////////////////////////////////////////////////////////
  // Format change crash test.
  // Try to set numerous formats. 

  log->msg("Format change crash test");

  for (int i_format = 0; i_format < n_formats; i_format++)
    for (int i_mode = 0; i_mode < n_modes; i_mode++)
      for (int i_sample_rate = 0; i_sample_rate < n_sample_rates; i_sample_rate++)
        f.set_input(Speakers(formats[i_format], modes[i_mode], sample_rates[i_sample_rate]));

  /////////////////////////////////////////////////////////
  // Format change
  //
  // Test format change scenarios. Most of work is done by 
  // FilterTester so we do not explicitly check the filter
  // to actually change the stream. We just run different
  // scenarios to force traps to work...
  /////////////////////////////////////////////////////////

  /////////////////////////////////////////////////////////
  // Forced format change 1. 
  // Empty filter, set_input()

  log->msg("Forced format change 1. Empty filter, set_input()");

  SET_INPUT_OK(spk_supported,     "Set format: %s %s %i failed");
  SET_INPUT_OK(spk_supported,     "set_input(same format: %s %s %i) failed");
  SET_INPUT_OK(spk_supported2,    "set_input(new format: %s %s %i) failed");
  SET_INPUT_FAIL(spk_unsupported, "set_input(wrong format: %s %s %i) succeeded");

  /////////////////////////////////////////////////////////
  // Forced format change 2. 
  // Empty filter, process(empty chunk)

  log->msg("Forced format change 2. Empty filter, process(empty chunk)");

  // 2.1 - format change to the same format
  chunk.set_empty(spk_supported);
  SET_INPUT_OK(spk_supported,     "Set format: %s %s %i failed");
  PROCESS_OK(chunk,               "process(same format: %s %s %i) failed");

  // 2.2 - format change to the new format
  chunk.set_empty(spk_supported2);
  SET_INPUT_OK(spk_supported,     "Set format: %s %s %i failed");
  PROCESS_OK(chunk,               "process(new format: %s %s %i) failed");

  // 2.3 - format change to the unsupported format
  chunk.set_empty(spk_unsupported);
  SET_INPUT_OK(spk_supported,     "Set format: %s %s %i failed");
  PROCESS_FAIL(chunk,             "process(wrong format: %s %s %i) succeeded");

  /////////////////////////////////////////////////////////
  // Forced format change 3. 
  // Empty filter, process(data chunk)

  log->msg("Forced format change 3. Empty filter, process(data chunk)");

  // 3.1 - format change to the same format
  src.open(spk_supported, filename, data_size);
  src.get_chunk(&chunk);
  SET_INPUT_OK(spk_supported,     "Set format: %s %s %i failed");
  PROCESS_OK(chunk,               "process(same format: %s %s %i) failed");

  // 3.2 - format change to the new format
  src.open(spk_supported2, filename2, data_size);
  src.get_chunk(&chunk);
  SET_INPUT_OK(spk_supported,     "Set format: %s %s %i failed");
  PROCESS_OK(chunk,               "process(new format: %s %s %i) failed");

  // 3.3 - format change to the unsupported format
  // (use noise source for unsupported format)
  src.open(spk_unsupported, 0, data_size);
  src.get_chunk(&chunk);
  SET_INPUT_OK(spk_supported,     "Set format: %s %s %i failed");
  PROCESS_FAIL(chunk,             "process(wrong format: %s %s %i) succeeded");

  /////////////////////////////////////////////////////////
  // Forced format change 4. 
  // Full filter, set_input()

  log->msg("Forced format change 4. Full filter, set_input()");

  // 4.1 - format change to the same format
  src.open(spk_supported, filename, data_size);
  SET_INPUT_OK(spk_supported,     "Set format: %s %s %i failed");
  FILL_FILTER;
  SET_INPUT_OK(spk_supported,     "set_input(same format: %s %s %i) failed");

  // 4.2 - format change to the new format
  src.open(spk_supported, filename, data_size);
  SET_INPUT_OK(spk_supported,     "Set format: %s %s %i failed");
  FILL_FILTER;
  SET_INPUT_OK(spk_supported2,    "set_input(new format: %s %s %i) failed");

  // 4.3 - format change to the usupported format
  src.open(spk_supported, filename, data_size);
  SET_INPUT_OK(spk_supported,     "Set format: %s %s %i failed");
  FILL_FILTER;
  SET_INPUT_FAIL(spk_unsupported, "set_input(wrong format: %s %s %i) succeeded");

  /////////////////////////////////////////////////////////
  // Forced format change 5. 
  // Cycled filter, set_input()

  log->msg("Forced format change 5. Cycled filter, set_input()");

  // 5.1 - format change to the same format

  src.open(spk_supported, filename, data_size);
  SET_INPUT_OK(spk_supported,     "Set format: %s %s %i failed");
  FILL_FILTER;
  EMPTY_FILTER;
  SET_INPUT_OK(spk_supported,     "set_input(same format: %s %s %i) failed");

  // 5.2 - format change to the new format

  src.open(spk_supported, filename, data_size);
  SET_INPUT_OK(spk_supported,     "Set format: %s %s %i failed");
  FILL_FILTER;
  EMPTY_FILTER;
  SET_INPUT_OK(spk_supported2,    "set_input(new format: %s %s %i) failed");

  // 5.3 - format change to the unsupported format

  src.open(spk_supported, filename, data_size);
  SET_INPUT_OK(spk_supported,     "Set format: %s %s %i failed");
  FILL_FILTER;
  EMPTY_FILTER;
  SET_INPUT_FAIL(spk_unsupported, "set_input(wrong format: %s %s %i) succeeded");

  /////////////////////////////////////////////////////////
  // Forced format change 6.
  // Cycled filter, process(empty chunk)

  log->msg("Forced format change 6. Cycled filter, process(empty chunk)");

  // 6.1 - format change to the new format

  src.open(spk_supported, filename, data_size);
  SET_INPUT_OK(spk_supported,     "Set format: %s %s %i failed");
  FILL_FILTER;
  EMPTY_FILTER;

  chunk.set_empty(spk_supported2);
  PROCESS_OK(chunk,               "process(new format: %s %s %i) failed");

  // 6.2 - format change to the unsupported format

  src.open(spk_supported, filename, data_size);
  SET_INPUT_OK(spk_supported,     "Set format: %s %s %i failed");
  FILL_FILTER;
  EMPTY_FILTER;

  chunk.set_empty(spk_unsupported);
  PROCESS_FAIL(chunk,             "process(wrong format: %s %s %i) succeeded");

  /////////////////////////////////////////////////////////
  // Forced format change 7.
  // Cycled filter, process(data chunk)

  log->msg("Forced format change 7. Cycled filter, process(data chunk)");

  // 7.1 - format change to the new format

  src.open(spk_supported, filename, data_size);
  SET_INPUT_OK(spk_supported,     "Set format: %s %s %i failed");
  FILL_FILTER;
  EMPTY_FILTER;

  src.open(spk_supported2, filename2, data_size);
  src.get_chunk(&chunk);
  PROCESS_OK(chunk,               "process(new format: %s %s %i) failed");

  // 7.2 - format change to the unsupported format
  // (use noise source for unsupported format)

  src.open(spk_supported, filename, data_size);
  SET_INPUT_OK(spk_supported,     "Set format: %s %s %i failed");
  FILL_FILTER;
  EMPTY_FILTER;

  src.open(spk_unsupported, 0, data_size);
  src.get_chunk(&chunk);
  PROCESS_FAIL(chunk,             "process(wrong format: %s %s %i) succeeded");

  /////////////////////////////////////////////////////////
  // Flushing
  //
  // Test flushing scenarios. Most of work is done by 
  // FilterTester so we do not explicitly check the filter
  // to end the stream.  We just run different scenarios to
  // force traps to work...
  /////////////////////////////////////////////////////////

  /////////////////////////////////////////////////////////
  // Flushing 1. 
  // Empty filter, process(empty chunk)

  log->msg("Flushing 1. Empty filter, process(empty chunk)");

  // 1.1 same format (flush current stream)

  chunk.set_empty(spk_supported, false, 0, true);
  SET_INPUT_OK(spk_supported,     "Set format: %s %s %i failed");
  PROCESS_OK(chunk,               "process(%s %s %i) failed");
  if (!f.is_empty())
  {
    size_t size = 0;
    while (!f.is_empty())
    {
      GET_CHUNK_OK(chunk,         "get_chunk() failed");
      size += chunk.size;
    }
    if (size)
      log->err("Empty filter generates output on flushing");
  }
  else
    log->msg("Empty filter does not pass the eos-chunk");

  // 1.2 new format (forced format change and then flush new stream)

  chunk.set_empty(spk_supported2, false, 0, true);
  SET_INPUT_OK(spk_supported,     "Set format: %s %s %i failed");
  PROCESS_OK(chunk,               "process(%s %s %i) failed");
  if (!f.is_empty())
  {
    size_t size = 0;
    while (!f.is_empty())
    {
      GET_CHUNK_OK(chunk,         "get_chunk() failed");
      size += chunk.size;
    }
    if (size)
      log->err("Empty filter generates output on flushing");
  }
  else
    log->msg("Empty filter does not pass the eos-chunk");

  // 1.3 unsupported format (terminate current stream)

  chunk.set_empty(spk_unsupported, false, 0, true);
  SET_INPUT_OK(spk_supported,     "Set format: %s %s %i failed");
  PROCESS_FAIL(chunk,             "process(%s %s %i) succeeded");

  /////////////////////////////////////////////////////////
  // Flushing 2. 
  // Empty filter, process(data chunk)

  log->msg("Flushing 2. Empty filter, process(data chunk)");

  // 2.1 same format

  src.open(spk_supported, filename, data_size);
  src.get_chunk(&chunk);
  chunk.eos = true;
  SET_INPUT_OK(spk_supported,     "Set format: %s %s %i failed");
  PROCESS_OK(chunk,               "process(%s %s %i) failed");
  if (f.is_empty())
    log->msg("Filter does not pass the eos-chunk");
  else
    EMPTY_FILTER;
 
  // 2.2 new format (forced format change and then flush new stream)

  src.open(spk_supported2, filename, data_size);
  src.get_chunk(&chunk);
  chunk.eos = true;
  SET_INPUT_OK(spk_supported,     "Set format: %s %s %i failed");
  PROCESS_OK(chunk,               "process(%s %s %i) failed");
  if (!f.is_empty())
  {
    EMPTY_FILTER;
    if (!chunk.eos)
      log->err("Empty filter did not end the stream");
  }
  else
    log->msg("Empty filter does not pass the eos-chunk");

  // 2.3 unsupported format

  src.open(spk_unsupported, 0, data_size);
  src.get_chunk(&chunk);
  chunk.eos = true;
  SET_INPUT_OK(spk_supported,     "Set format: %s %s %i failed");
  PROCESS_FAIL(chunk,             "process(%s %s %i) succeeded");
/*
  /////////////////////////////////////////////////////////
  // Flushing 3. 
  // Cycled filter, process(empty chunk)

  log->msg("Flushing 3. Cycled filter, process(empty chunk)");

  // 2.1 same format

  src.open(spk_supported, filename, data_size);
  SET_INPUT_OK(spk_supported,     "Set format: %s %s %i failed");

  src.get_chunk(&chunk);
  PROCESS_OK(chunk,               "process(%s %s %i) failed");
  if (!f.is_empty())
  {
    EMPTY_FILTER;
    if (!chunk.eos)
      log->err("Filter did not end the stream");
  }
  else
    log->msg("Filter does not pass the eos-chunk");

  // 2.2 new format (forced format change and then flush new stream)

  src.open(spk_supported2, filename, data_size);
  src.get_chunk(&chunk);
  SET_INPUT_OK(spk_supported,     "Set format: %s %s %i failed");
  PROCESS_OK(chunk,               "process(%s %s %i) failed");
  if (!f.is_empty())
  {
    EMPTY_FILTER;
    if (!chunk.eos)
      log->err("Empty filter did not end the stream");
  }
  else
    log->msg("Empty filter does not pass the eos-chunk");

  // 2.3 unsupported format

  src.open(spk_unsupported, 0, data_size);
  src.get_chunk(&chunk);
  chunk.eos = true;
  SET_INPUT_OK(spk_supported,     "Set format: %s %s %i failed");
  PROCESS_FAIL(chunk,             "process(%s %s %i) succeeded");
*/
  return 0;
}