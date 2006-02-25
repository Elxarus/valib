/*
  NullFilter noise passthrough test
*/

#include "log.h"
#include "filter_tester.h"
#include "filters\spdifer.h"
#include "filters\demux.h"
#include <source\noise.h>
#include <source\raw_source.h>
#include <win32\cpu.h>
#include "common.h"

///////////////////////////////////////////////////////////////////////////////
// Test constants

// noise speed test
static const int noise_size = 10000000;
static const int noise_runs = 1;

// file speed test
static const char *file_raw   = "test.all";
static const char *file_spdif = "test.all.spdif";
static const int   file_runs = 50;

///////////////////////////////////////////////////////////////////////////////
// Test class

class Spdifer_test
{
protected:
  FilterTester t;
  Spdifer s;
  Log *log;

public:
  Spdifer_test(Log *_log)
  {
    log = _log;
    t.link(&s, log);
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

    // passthrough test
    compare_file(log, Speakers(FORMAT_AC3, 0, 0), "F:\\samples\\dts\\DTS-AudioCD\\Music\\Mendelssohn_2.dts", &t, "F:\\samples\\dts\\DTS-AudioCD\\Music\\Mendelssohn_2.dts");
    // raw stream -> spdif stream
    compare_file(log, Speakers(FORMAT_AC3, 0, 0), "test.ac3", &t, "test.ac3.spdif");
    compare_file(log, Speakers(FORMAT_DTS, 0, 0), "test.dts", &t, "test.dts.spdif");
    compare_file(log, Speakers(FORMAT_MPA, 0, 0), "test.mp2", &t, "test.mp2.spdif");
    compare_file(log, Speakers(FORMAT_UNKNOWN, 0, 0), "test.all", &t, "test.all.spdif");
    // spdif stream -> spdif stream
    compare_file(log, Speakers(FORMAT_AC3, 0, 0), "test.ac3.spdif", &t, "test.ac3.spdif");
    compare_file(log, Speakers(FORMAT_DTS, 0, 0), "test.dts.spdif", &t, "test.dts.spdif");
    compare_file(log, Speakers(FORMAT_MPA, 0, 0), "test.mp2.spdif", &t, "test.mp2.spdif");
    compare_file(log, Speakers(FORMAT_UNKNOWN, 0, 0), "test.all.spdif", &t, "test.all.spdif");
  }

  void speed_noise()
  {
    /////////////////////////////////////////////////////////
    // Noise speed test

    Chunk ichunk;
    Chunk ochunk;
    Noise noise(Speakers(FORMAT_AC3, 0, 0), noise_size, noise_size);
    noise.get_chunk(&ichunk);

    CPUMeter cpu;
    cpu.reset();
    cpu.start();

    int data_chunks = 0;
    int empty_chunks = 0;
    for (int i = 0; i < noise_runs; i++)
    {
      t.reset();
      t.process(&ichunk);
      while (!t.is_empty())
      {
        t.get_chunk(&ochunk);
        if (ochunk.size)
          data_chunks++;
        else
          empty_chunks++;
      }
    }
    cpu.stop();

    log->msg("Spdifer speed on noise: %iMB/s, Data: %i, Empty: %i", 
      int(double(noise_size) * noise_runs / cpu.get_thread_time() / 1000000), 
      data_chunks / noise_runs, empty_chunks / noise_runs);
  }

  void speed_file(const char *file_name)
  {
    /////////////////////////////////////////////////////////
    // File speed test

    Chunk ichunk;
    Chunk ochunk;
    RAWSource f(spk_unknown, file_name);
    if (!f.is_open())
    {
      log->err("Cannot open file %s", file_name);
      return;
    }

    t.reset();

    CPUMeter cpu;
    cpu.reset();
    cpu.start();

    int data_chunks = 0;
    int empty_chunks = 0;
    for (int i = 0; i < file_runs; i++)
    {
      f.seek(0);
      t.reset();
      while (!f.eof())
      {
        f.get_chunk(&ichunk);
        t.process(&ichunk);
        while (!t.is_empty())
        {
          t.get_chunk(&ochunk);
          if (ochunk.size)
            data_chunks++;
          else
            empty_chunks++;
        }
      }
    }
    cpu.stop();

    log->msg("Spdifer speed on file %s: %iMB/s, Data: %i, Empty: %i", file_name, 
      int(double(f.size()) * file_runs / cpu.get_thread_time() / 1000000), 
      data_chunks / file_runs, empty_chunks / file_runs);
  }

};

///////////////////////////////////////////////////////////////////////////////
// Test function

int test_spdifer(Log *log)
{
  Spdifer_test test(log);
  return test.test();
}
