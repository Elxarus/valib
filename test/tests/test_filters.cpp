/*
  Filters test
  Test each filter to comply following requirements:
  * Filter should accept empty chunks at process() call without error
  * Filter should return empty chunks at get_chunk() call if filter is empty without error
*/

#include "..\log.h"
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




int test_filter(Log *log, Filter *filter, const char *desc);
int test_filters(Log *log)
{
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

int test_filter(Log *log, Filter *filter, const char *filter_name)
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

    chunk.set(filter->get_input(), 0, 0);

    if (!filter->process(&chunk))
      return log->err("process() of empty chunk failed");

    if (!filter->is_empty())
      return log->err("filter is not empty after processing of empty chunk");

    if (!filter->get_chunk(&chunk))
      return log->err("get_chunk() of empty filter failed");

    if (!filter->is_empty())
      return log->err("filter is not empty after get_chunk()");

    if (!chunk.is_empty())
      return log->err("filter returned non-empty chunk");

    ///////////////////////////////////////////////////////
    // End-of-stream processing

    chunk.set(filter->get_input(), 0, 0, 0, 0, true);

    if (!filter->process(&chunk))
      return log->err("process() of end-of-stream failed");

    if (filter->is_empty())
      return log->err("filter is empty after receiving end-of-stream");

    if (!filter->get_chunk(&chunk))
      return log->err("get_chunk() of flushing filter failed");

    if (!filter->is_empty())
      return log->err("filter is not empty after get_chunk()");

    if (!chunk.is_empty())
      return log->err("filter returned non-empty chunk");

    if (!chunk.is_eos())
      return log->err("filter did not return end-of stream");
  }

  return 0;
}