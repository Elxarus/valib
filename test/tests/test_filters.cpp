/*
  Filters test
  Test each filter to comply following requirements:
  * Filter should accept empty chunks at process() call without error
  * Filter should return empty chunks at get_chunk() call if filter is empty without error
*/

#include <stdio.h>
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




int test_filter(Filter *filter, const char *desc);
int test_filters()
{
  AGC            agc;
  Converter      conv;
  AudioDecoder   dec;
  Delay          delay;
  DVDDecoder     dvd;
  FilterChain    chain;
  Levels         levels;
  AudioProcessor proc;
  Demux          demux;
  Spdifer        spdifer;

  int err = 0;
  printf("\n* Filter tests (empty chunk passthrough, empty filter returns empty chunks)\n");
  err += test_filter(&agc,     "AGC");
  err += test_filter(&conv,    "Converter");
  err += test_filter(&dec,     "AudioDecoder");
  err += test_filter(&delay,   "Delay");
  err += test_filter(&dvd,     "DVDDecoder");
  err += test_filter(&chain,   "FilterChain");
  err += test_filter(&levels,  "Levels");
  err += test_filter(&proc,    "AudioProcessor");
  err += test_filter(&demux,   "Demux");
  err += test_filter(&spdifer, "Spdifer");
  return err;
}

int test_filter(Filter *filter, const char *desc)
{
  // Test for:
  // Filter should accept empty chunks without error in process() call
  // Filter should return empty chunks without error in get_chunk() call if filter is empty 

  int i;
  Chunk empty;
  Chunk empty1;
  empty.set_empty();

  printf("Testing filter %s\n", desc);

  if (!filter->is_empty())
  {
    printf("Error: filter is not empty!!!\n");
    return 1;
  }

  for (i = 0; i < 100; i++)
  {
    if (!filter->process(&empty))
    {
      printf("Error: process() failed\n");
      return 1;
    }

    if (!filter->is_empty())
    { 
      printf("Error: filter is not empty after processing!\n");
      return 1;
    }

    if (!filter->get_chunk(&empty1))
    {
      printf("Error: get_chunk() failed\n");
      return 1;
    }

    if (!filter->is_empty())
    { 
      printf("Error: not empty chunk returned!\n");
      return 1;
    }
  }

  return 0;
}