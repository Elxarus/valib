/*
  Spdifer filter test
  (using filter tester)

  1. Conversion test:
  1.1 high-bitrate DTS passthrough mode
  1.2 raw->spdif conversion
  1.3 spdif->spdif conversion

  2. Speed test on noise.

  3. Speed test on file:
  3.1 raw file
  3.2 spdif file

  Required files:
  test.mpa
  test.ac3
  test.dts
  test.mpa.spdif
  test.ac3.spdif
  test.dts.spdif
  test.all
  test.all.spdif

*/

#include "suite.h"
#include "filter_tester.h"
#include "filters\spdifer.h"
#include <source\generator.h>
#include <source\raw_source.h>
#include <win32\cpu.h>

///////////////////////////////////////////////////////////////////////////////
// Test constants

static const vtime_t time_per_test = 1.0; // 1 sec for each speed test

// noise speed test
static const int seed = 84735;
static const int noise_size = 10000000;

// file speed test
static const char *file_raw   = "a.mad.mix.mad";
static const char *file_spdif = "a.mad.mix.spdif";

///////////////////////////////////////////////////////////////////////////////
// Test class

class Spdifer_test
{
protected:
  Spdifer spdifer;
  Log *log;

public:
  Spdifer_test(Log *_log)
  {
    log = _log;
  }

  int test()
  {
    log->open_group("Spdifer test");
    transform();
    speed_noise();
    speed_file(file_raw);
    speed_file(file_spdif);
    return log->close_group();
  }

  void transform()
  {
    /////////////////////////////////////////////////////////
    // Transform test

    // high-bitrate dts passthrough test
    compare_file(log, Speakers(FORMAT_DTS, 0, 0), "e:\\samples\\dts\\DTS-AudioCD\\Music\\Mendelssohn_2.dts", &spdifer, "e:\\samples\\dts\\DTS-AudioCD\\Music\\Mendelssohn_2.dts");
    // raw stream -> spdif stream
    compare_file(log, Speakers(FORMAT_MPA, 0, 0), "a.mp2.005.mp2", &spdifer, "a.mp2.005.spdif");
    compare_file(log, Speakers(FORMAT_MPA, 0, 0), "a.mp2.002.mp2", &spdifer, "a.mp2.002.spdif");
    compare_file(log, Speakers(FORMAT_AC3, 0, 0), "a.ac3.03f.ac3", &spdifer, "a.ac3.03f.spdif");
    compare_file(log, Speakers(FORMAT_AC3, 0, 0), "a.ac3.005.ac3", &spdifer, "a.ac3.005.spdif");
    compare_file(log, Speakers(FORMAT_DTS, 0, 0), "a.dts.03f.dts", &spdifer, "a.dts.03f.spdif");
    compare_file(log, Speakers(FORMAT_RAWDATA, 0, 0), "a.mad.mix.mad", &spdifer, "a.mad.mix.spdif");
    // spdif stream -> spdif stream
    compare_file(log, Speakers(FORMAT_SPDIF, 0, 0), "a.mp2.005.spdif", &spdifer, "a.mp2.005.spdif");
    compare_file(log, Speakers(FORMAT_SPDIF, 0, 0), "a.mp2.002.spdif", &spdifer, "a.mp2.002.spdif");
    compare_file(log, Speakers(FORMAT_SPDIF, 0, 0), "a.ac3.03f.spdif", &spdifer, "a.ac3.03f.spdif");
    compare_file(log, Speakers(FORMAT_SPDIF, 0, 0), "a.ac3.005.spdif", &spdifer, "a.ac3.005.spdif");
    compare_file(log, Speakers(FORMAT_SPDIF, 0, 0), "a.dts.03f.spdif", &spdifer, "a.dts.03f.spdif");
    compare_file(log, Speakers(FORMAT_SPDIF, 0, 0), "a.mad.mix.spdif", &spdifer, "a.mad.mix.spdif");
  }

  void speed_noise()
  {
    /////////////////////////////////////////////////////////
    // Noise speed test

    Chunk2 in, out;
    NoiseGen noise(Speakers(FORMAT_AC3, 0, 0), seed, noise_size, noise_size);
    noise.get_chunk(in);

    CPUMeter cpu;
    cpu.reset();
    cpu.start();

    int runs = 0;
    int data_chunks = 0;
    int empty_chunks = 0;
    while (cpu.get_thread_time() < time_per_test)
    {
      runs++;
      spdifer.reset();
      while (spdifer.process(in, out))
        /*do nothing*/;
    }
    cpu.stop();

    log->msg("Spdifer speed on noise: %iMB/s", 
      int(double(noise_size) * runs / cpu.get_thread_time() / 1000000));
  }

  void speed_file(const char *file_name)
  {
    /////////////////////////////////////////////////////////
    // File speed test

    Chunk2 in, out;
    RAWSource f(Speakers(FORMAT_RAWDATA, 0, 0), file_name);
    if (!f.is_open())
    {
      log->err("Cannot open file %s", file_name);
      return;
    }

    CPUMeter cpu;
    cpu.reset();
    cpu.start();

    int runs = 0;
    int data_chunks = 0;
    int empty_chunks = 0;
    while (cpu.get_thread_time() < time_per_test)
    {
      runs++;
      f.reset();
      spdifer.reset();
      while (f.get_chunk(in))
        while (spdifer.process(in, out))
          /*do nothing*/;
    }
    cpu.stop();

    log->msg("Spdifer speed on file %s: %iMB/s", file_name, 
      int(double(f.size()) * runs / cpu.get_thread_time() / 1000000));
  }

};

///////////////////////////////////////////////////////////////////////////////
// Test function

int test_spdifer(Log *log)
{
  Spdifer_test test(log);
  return test.test();
}
