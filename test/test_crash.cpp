/*
  Noise crash test for filters that have raw input
  (Demux, Spdifer, etc)
*/

#include "log.h"
#include "filter_tester.h"
#include "filters\demux.h"
#include "filters\spdifer.h"
#include "filters\decoder.h"
#include "filters\dvd_decoder.h"

#include "source\noise.h"


int test_crash(Log *log);
int test_crash_filter(Log *log, Speakers spk, Filter *filter, const char *filter_name);


int test_crash(Log *log)
{
  log->open_group("Raw noise crash test");

  Demux        demux;
  Spdifer      spdifer;
  AudioDecoder dec;
  DVDDecoder   dvd;

  test_crash_filter(log, Speakers(FORMAT_PES,     0, 0), &demux, "Demux");

  test_crash_filter(log, Speakers(FORMAT_AC3,     0, 0), &spdifer, "Spdifer");
  test_crash_filter(log, Speakers(FORMAT_MPA,     0, 0), &spdifer, "Spdifer");
  test_crash_filter(log, Speakers(FORMAT_DTS,     0, 0), &spdifer, "Spdifer");
  test_crash_filter(log, Speakers(FORMAT_SPDIF,   0, 0), &spdifer, "Spdifer");
  test_crash_filter(log, Speakers(FORMAT_UNKNOWN, 0, 0), &spdifer, "Spdifer");

  test_crash_filter(log, Speakers(FORMAT_AC3,     0, 0), &dec, "Decoder");
  test_crash_filter(log, Speakers(FORMAT_MPA,     0, 0), &dec, "Decoder");
  test_crash_filter(log, Speakers(FORMAT_DTS,     0, 0), &dec, "Decoder");

  test_crash_filter(log, Speakers(FORMAT_PCM16,   MODE_STEREO, 48000), &dvd, "DVDDecoder");
  test_crash_filter(log, Speakers(FORMAT_PCM24,   MODE_STEREO, 48000), &dvd, "DVDDecoder");
  test_crash_filter(log, Speakers(FORMAT_PCM32,   MODE_STEREO, 48000), &dvd, "DVDDecoder");

  test_crash_filter(log, Speakers(FORMAT_AC3,     0, 0), &dvd, "DVDDecoder");
  test_crash_filter(log, Speakers(FORMAT_MPA,     0, 0), &dvd, "DVDDecoder");
  test_crash_filter(log, Speakers(FORMAT_DTS,     0, 0), &dvd, "DVDDecoder");

  test_crash_filter(log, Speakers(FORMAT_PES,     0, 0), &dvd, "DVDDecoder");

  return log->close_group();
}


int test_crash_filter(Log *log, Speakers spk, Filter *filter, const char *filter_name)
{
  log->msg("Testing filter %s with format %s %s", filter_name, spk.format_text(), spk.mode_text());

  Noise noise;
  Chunk ichunk;
  Chunk ochunk;
  size_t isize = 0;
  size_t osize = 0;
  FilterTester f(filter, log);

  if (!f.set_input(spk))
    return log->err("filter->set_input() failed!");

  if (!noise.set_output(spk))
    return log->err("noise.set_input() failed!");

  if (!noise.set_output(spk, 1024*1024))
    return log->err("noise.set_input() failed!");

  while (!noise.is_empty())
  {
    log->status("Pos: %i", osize);

    if (!noise.get_chunk(&ichunk))
      return log->err("noise.get_chunk() failed!");
    isize += ichunk.size;

    if (!f.process(&ichunk))
      return log->err("filter->process() failed!");

    while (!f.is_empty())
    {
      if (!f.get_chunk(&ochunk))
        return log->err("filter->get_chunk() failed!");
      osize += ochunk.size;
    }
  }

  return 0;
}
