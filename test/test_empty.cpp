/*
  Empty filters test

  Test each filter to comply following requirements (filter and chunk are always empty):
  * Filter should be empty after reset() call
  * Filter should accept empty chunk at process() call without error
  * Filter should remain empty after receiving empty chunk
  * Filter should return empty chunk at get_chunk() call without error
  * Filter should go to flushing state after receiving end-of-stream and report that it is not empty
    (because it must send end-of-stream chunk on next get_chunk())
  * Filter should return empty end-of-stream chunk in flushing state and go to empty state
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

int test_empty(Log *log);
int test_empty_filter(Log *log, Filter *filter, const char *desc);


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
  test_empty_filter(log, &null,    "NullFilter");
  test_empty_filter(log, &agc,     "AGC");
  test_empty_filter(log, &conv,    "Converter");
  test_empty_filter(log, &dec,     "AudioDecoder");
  test_empty_filter(log, &delay,   "Delay");
  test_empty_filter(log, &dvd,     "DVDDecoder");
  test_empty_filter(log, &chain,   "FilterChain");
  test_empty_filter(log, &levels,  "Levels");
  test_empty_filter(log, &proc,    "AudioProcessor");
  test_empty_filter(log, &demux,   "Demux");
  test_empty_filter(log, &spdifer, "Spdifer");
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