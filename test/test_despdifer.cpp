/*
  Despdifer filter test
  (using filter tester)

  1. Conversion test:
  1.1 spdif->raw conversion

  2. Speed test on noise.

  3. Speed test on file:
  3.1 spdif file
*/

#include "suite.h"
#include "filters\spdifer.h"
#include <source\generator.h>
#include <source\raw_source.h>
#include <win32\cpu.h>

///////////////////////////////////////////////////////////////////////////////
// Test constants

static const vtime_t time_per_test = 1.0; // 1 sec for each speed test

// noise speed test
static const int seed = 5465;
static const int noise_size = 10000000;

// file speed test
static const char *file_spdif = "a.mad.mix.spdif";

///////////////////////////////////////////////////////////////////////////////
// Test class

class Despdifer_test
{
protected:
  Despdifer despdifer;
  Log *log;

public:
  Despdifer_test(Log *_log)
  {
    log = _log;
  }

  int test()
  {
    log->open_group("Despdifer test");
    transform();
    speed_noise();
    speed_file(file_spdif);
    return log->close_group();
  }

  void transform()
  {
    /////////////////////////////////////////////////////////
    // Transform test

    // spdif stream -> raw stream
    compare_file(log, Speakers(FORMAT_SPDIF, 0, 0),   "a.mp2.005.spdif", &despdifer, "a.mp2.005.mp2");
    compare_file(log, Speakers(FORMAT_SPDIF, 0, 0),   "a.mp2.002.spdif", &despdifer, "a.mp2.002.mp2");
    compare_file(log, Speakers(FORMAT_SPDIF, 0, 0),   "a.ac3.03f.spdif", &despdifer, "a.ac3.03f.ac3");
    compare_file(log, Speakers(FORMAT_SPDIF, 0, 0),   "a.ac3.005.spdif", &despdifer, "a.ac3.005.ac3");
    compare_file(log, Speakers(FORMAT_SPDIF, 0, 0),   "a.dts.03f.spdif", &despdifer, "a.dts.03f.dts");
    compare_file(log, Speakers(FORMAT_RAWDATA, 0, 0), "a.mad.mix.spdif", &despdifer, "a.mad.mix.mad");
  }

  void speed_noise()
  {
    /////////////////////////////////////////////////////////
    // Noise speed test

    Chunk2 in, out;
    NoiseGen noise(Speakers(FORMAT_SPDIF, 0, 0), seed, noise_size, noise_size);
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
      despdifer.reset();
      while (despdifer.process(in, out))
        /*do nothing*/;
    }
    cpu.stop();

    log->msg("Despdifer speed on noise: %iMB/s", 
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
      despdifer.reset();
      while (f.get_chunk(in))
        while (despdifer.process(in, out))
          /*do nothing*/;
    }
    cpu.stop();

    log->msg("Despdifer speed on file %s: %iMB/s", file_name, 
      int(double(f.size()) * runs / cpu.get_thread_time() / 1000000));
  }

};

///////////////////////////////////////////////////////////////////////////////
// Test function

int test_despdifer(Log *log)
{
  Despdifer_test test(log);
  return test.test();
}
