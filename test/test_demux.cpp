/*
  Demux filter test
  (using filter tester)

  1. Transform test
  1.1. pes to raw stream tests
  1.2. all possible pes format transitions test
  2. Speed test on noise
  3. Speed test on file
*/

#include "suite.h"
#include "filter_tester.h"
#include "filters\spdifer.h"
#include "filters\demux.h"
#include <source\generator.h>
#include <source\raw_source.h>
#include <win32\cpu.h>

///////////////////////////////////////////////////////////////////////////////
// Test constants

static const int seed = 98754;
static const int noise_size = 10000000;   // use 10MB noise buffer
static const vtime_t time_per_test = 1.0; // 1s per speed test
static const char *file_pes = "a.madp.mix.pes";

///////////////////////////////////////////////////////////////////////////////
// Test class

class Demux_test
{
protected:
  Demux demux;
  Log *log;

public:
  Demux_test(Log *_log)
  {
    log = _log;
  }

  int test()
  {
    log->open_group("Demux test");
    transform();
    speed_noise();
    speed_file(file_pes);
    return log->close_group();
  }

  void transform()
  {
    /////////////////////////////////////////////////////////
    // Transform test

    compare_file(log, Speakers(FORMAT_PES, 0, 0), "a.mp2.005.pes", &demux, "a.mp2.005.mp2");
    compare_file(log, Speakers(FORMAT_PES, 0, 0), "a.mp2.002.pes", &demux, "a.mp2.002.mp2");
    compare_file(log, Speakers(FORMAT_PES, 0, 0), "a.mp2.mix.pes", &demux, "a.mp2.mix.mp2");

    compare_file(log, Speakers(FORMAT_PES, 0, 0), "a.ac3.03f.pes", &demux, "a.ac3.03f.ac3");
    compare_file(log, Speakers(FORMAT_PES, 0, 0), "a.ac3.005.pes", &demux, "a.ac3.005.ac3");
    compare_file(log, Speakers(FORMAT_PES, 0, 0), "a.ac3.mix.pes", &demux, "a.ac3.mix.ac3");

    compare_file(log, Speakers(FORMAT_PES, 0, 0), "a.dts.03f.pes", &demux, "a.dts.03f.dts");

    compare_file(log, Speakers(FORMAT_PES, 0, 0), "a.pcm.005.pes", &demux, "a.pcm.005.lpcm");

    compare_file(log, Speakers(FORMAT_PES, 0, 0), "a.mad.mix.pes", &demux, "a.mad.mix.mad");
    compare_file(log, Speakers(FORMAT_PES, 0, 0), "a.madp.mix.pes", &demux, "a.madp.mix.madp");
  }

  void speed_noise()
  {
    /////////////////////////////////////////////////////////
    // Noise speed test

    Chunk2 in, out;
    NoiseGen noise(Speakers(FORMAT_PES, 0, 0), seed, noise_size, noise_size);
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
      demux.reset();
      while (demux.process(in, out))
        /*do nothing*/;
    }
    cpu.stop();

    log->msg("Demux speed on noise: %iMB/demux, Data: %i, Empty: %i", 
      int(double(noise_size) * runs / cpu.get_thread_time() / 1000000));
  }

  void speed_file(const char *file_name)
  {
    /////////////////////////////////////////////////////////
    // File speed test

    Chunk2 in, out;
    RAWSource f(Speakers(FORMAT_PES, 0, 0), file_name);

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
      demux.reset();
      while (f.get_chunk(in))
        while (demux.process(in, out))
          /*do nothing*/;
    }
    cpu.stop();

    log->msg("Demux speed on file %demux: %iMB/demux", file_name,
      int(double(f.size()) * runs / cpu.get_thread_time() / 1000000));
  }

};

///////////////////////////////////////////////////////////////////////////////
// Test function

int test_demux(Log *log)
{
  Demux_test test(log);
  return test.test();
}
