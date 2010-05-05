/*
  Noise crash test for filters that have raw input (Demux, Spdifer, etc)
  (using filter tester)

  Just send noise to the filter input and get output.
*/

#include "log.h"
#include "filters\demux.h"
#include "filters\spdifer.h"
#include "filters\decoder.h"
#include "filters\dvd_graph.h"
#include "source\generator.h"

static const int seed = 9374;
static const int noise_size = 1024 * 1024;

int test_crash(Log *log);
int test_crash_filter(Log *log, Speakers spk, Filter *filter, const char *filter_name);


int test_crash(Log *log)
{
  log->open_group("Raw noise crash test");

  Demux        demux;
  Spdifer      spdifer;
  AudioDecoder dec;
  DVDGraph     dvd;

  test_crash_filter(log, Speakers(FORMAT_PES,     0, 0), &demux, "Demux");

  test_crash_filter(log, Speakers(FORMAT_AC3,     0, 0), &spdifer, "Spdifer");
  test_crash_filter(log, Speakers(FORMAT_MPA,     0, 0), &spdifer, "Spdifer");
  test_crash_filter(log, Speakers(FORMAT_DTS,     0, 0), &spdifer, "Spdifer");
  test_crash_filter(log, Speakers(FORMAT_SPDIF,   0, 0), &spdifer, "Spdifer");
  test_crash_filter(log, Speakers(FORMAT_RAWDATA, 0, 0), &spdifer, "Spdifer");

  test_crash_filter(log, Speakers(FORMAT_AC3,     0, 0), &dec, "Decoder");
  test_crash_filter(log, Speakers(FORMAT_MPA,     0, 0), &dec, "Decoder");
  test_crash_filter(log, Speakers(FORMAT_DTS,     0, 0), &dec, "Decoder");

  test_crash_filter(log, Speakers(FORMAT_PCM16,   MODE_STEREO, 48000), &dvd, "DVDGraph");
  test_crash_filter(log, Speakers(FORMAT_PCM24,   MODE_STEREO, 48000), &dvd, "DVDGraph");
  test_crash_filter(log, Speakers(FORMAT_PCM32,   MODE_STEREO, 48000), &dvd, "DVDGraph");

  test_crash_filter(log, Speakers(FORMAT_AC3,     0, 0), &dvd, "DVDGraph");
  test_crash_filter(log, Speakers(FORMAT_MPA,     0, 0), &dvd, "DVDGraph");
  test_crash_filter(log, Speakers(FORMAT_DTS,     0, 0), &dvd, "DVDGraph");

  test_crash_filter(log, Speakers(FORMAT_PES,     0, 0), &dvd, "DVDGraph");

  return log->close_group();
}


int test_crash_filter(Log *log, Speakers spk, Filter *filter, const char *filter_name)
{
  log->msg("Testing filter %s with format %s %s", filter_name, spk.format_text(), spk.mode_text());

  Chunk in, out;
  size_t isize = 0;
  size_t osize = 0;

  NoiseGen noise;

  if (!filter->open(spk))
    return log->err("filter->open() failed!");

  if (!noise.init(spk, seed, noise_size))
    return log->err("noise.init() failed!");

  while (noise.get_chunk(in))
  {
    isize += in.size;
    if (isize < 10000)
      log->status("Pos: %u    ", isize);
    if (isize < 10000000)
      log->status("Pos: %uK    ", isize/1000);
    else
      log->status("Pos: %uM    ", isize/1000000);

    while (filter->process(in, out))
      osize += out.size;
  }

  return 0;
}
