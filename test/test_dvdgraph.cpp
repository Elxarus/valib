/*
  DVDGraph filter test
*/

#include "log.h"
#include "filter_tester.h"
#include "filters\dvd_graph.h"
#include <source\noise.h>
#include <source\raw_source.h>
#include <win32\cpu.h>
#include "common.h"

///////////////////////////////////////////////////////////////////////////////
// Test constants

static const vtime_t time_per_test = 1.0;   // 1 sec for each speed test
static const size_t min_data_size = 2048*4; // minimum data size to generate after state change
                                            // size of pcm16 data with maximum number of samples per frame

// noise speed test
static const int noise_size = 10000000;

// file speed test
static const char *file1_src = "a.mad.mix.pes";
static const char *file1_out = "a.mad.mix.spdif";
static const char *file2_src = "a.madp.mix.pes";
static const char *file2_out = "a.madp.mix.spdif";
static const char *file3_src = "a.mad.mix.spdif";
static const char *file4_out = "a.mad.mix.spdif";

///////////////////////////////////////////////////////////////////////////////
// Test class

class DVDGraph_test
{
protected:
  Filter *f;
  FilterTester t;
  DVDGraph dvd;
  Log *log;

public:
  DVDGraph_test(Log *_log)
  {
    log = _log;
    t.link(&dvd, log);
    f = &dvd; // do not use FilterTester
  }

  int test()
  {
    log->open_group("DVDGraph test");
    transform();
    spdif_rebuild();
    return log->close_group();
  }

  void transform()
  {
    /////////////////////////////////////////////////////////
    // Transform test

    // PES to SPDIF transform with format changes
    dvd.use_spdif = true;
    dvd.spdif_pt = FORMAT_MASK_MPA | FORMAT_MASK_AC3 | FORMAT_MASK_DTS;

    dvd.spdif_stereo_pt = true;
    dvd.user_spk = Speakers(FORMAT_PCM16_BE, 0, 0, 32768);

    compare_file(log, Speakers(FORMAT_PES, 0, 0), file1_src, f, file1_out);
    compare_file(log, Speakers(FORMAT_PES, 0, 0), file2_src, f, file2_out);

    // SPDIF to SPDIF transfrm with format changes (todo)
    // dvd.use_spdif = true;
    // dvd.user_spk = Speakers(FORMAT_PCM16, 0, 0);
    // compare_file(log, Speakers(FORMAT_SPDIF, 0, 0), file3_src, &t, file3_out);

    // todo: transform test with decode
  }

  void spdif_rebuild()
  {
    spdif_rebuild("a.ac3.03f.ac3", Speakers(FORMAT_AC3, 0, 0));
    spdif_rebuild("a.dts.03f.dts", Speakers(FORMAT_DTS, 0, 0));
    spdif_rebuild("a.mpa.005.mpa", Speakers(FORMAT_MPA, 0, 0));
  }

  int spdif_rebuild(const char *file_name, Speakers spk)
  {
    // Check all possible transition between spdif modes
    // (decode, passthrough, encode, stereo passthrough)

    RAWSource src(spk, file_name, 2048);
    if (!src.is_open())
      return log->err("Cannot open file %s", file_name);

    if (!f->set_input(spk))
      return log->err("dvd.set_input(%s %s %nHz) failed", 
        spk.format_text(), spk.mode_text(), spk.sample_rate);

    test_decode(&src);
    test_passthrough(&src);
    test_encode(&src);
    test_stereo_passthrough(&src);

    test_decode(&src);
    test_encode(&src);
    test_decode(&src);

    test_stereo_passthrough(&src);
    test_passthrough(&src);
    test_stereo_passthrough(&src);

    test_encode(&src);
    test_passthrough(&src);
    test_decode(&src);

    Chunk chunk;
    while (!src.is_empty())
    {
      if (!src.get_chunk(&chunk))
        return log->err("src.get_chunk() failed");
      if (!f->process(&chunk))
        return log->err("dvd.process() failed");
    }

    // todo: check number of output streams (should be 7)
    return 0;
  }

  int test_decode(Source *src)
  {
    dvd.user_spk = Speakers(FORMAT_PCM16, 0, 0, 32768);
    dvd.use_spdif = false;
    dvd.spdif_stereo_pt = false;
    return test_cycle("test_decode()", src, SPDIF_DISABLED, FORMAT_PCM16);
  }

  int test_passthrough(Source *src)
  {
    dvd.user_spk = Speakers(FORMAT_PCM16, 0, 0, 32768);
    dvd.use_spdif = true;
    dvd.spdif_stereo_pt = false;
    dvd.spdif_pt = FORMAT_CLASS_SPDIFABLE;
    return test_cycle("test_passthrough()", src, SPDIF_PASSTHROUGH, FORMAT_SPDIF);
  }

  int test_encode(Source *src)
  {
    dvd.user_spk = Speakers(FORMAT_PCM16, 0, 0, 32768);
    dvd.use_spdif = true;
    dvd.spdif_stereo_pt = false;
    dvd.spdif_pt = 0;
    return test_cycle("test_encode()", src, SPDIF_ENCODE, FORMAT_SPDIF);
  }

  int test_stereo_passthrough(Source *src)
  {
    dvd.user_spk = Speakers(FORMAT_PCM16, MODE_STEREO, 0, 32768);
    dvd.use_spdif = true;
    dvd.spdif_stereo_pt = true;
    dvd.spdif_pt = 0;
    return test_cycle("test_stereo_passthrough()", src, SPDIF_STEREO_PASSTHROUGH, FORMAT_PCM16);
  }

  int test_cycle(const char *caller, Source *src, int status, int out_format)
  {
    Chunk chunk;

    // process until status change
    // (filter may be either full or empty)

    while (!src->is_empty() && dvd.get_spdif_status() != status)
      if (f->is_empty())
      {
        if (!src->get_chunk(&chunk))
          return log->err("%s: src->get_chunk() failed before status change", caller);

        if (!f->process(&chunk))
          return log->err("%s: dvd.process() failed before status change", caller);
      }
      else
      {
        if (!f->get_chunk(&chunk))
          return log->err("%s: dvd.get_chunk() failed before status change", caller);
      }

    if (dvd.get_spdif_status() != status)
      return log->err("%s: cannot switch to decode state", caller);

    // ensure correct data generation in new state
    // (filter may be either full or empty)

    size_t data_size = 0;
    while (!src->is_empty() && data_size < min_data_size)
      if (f->is_empty())
      {
        if (!src->get_chunk(&chunk))
          return log->err("%s: src->get_chunk() failed after status change", caller);

        if (!f->process(&chunk))
          return log->err("%s: dvd.process() failed after status change", caller);
      }
      else
      {
        if (!f->get_chunk(&chunk))
          return log->err("%s: dvd.get_chunk() failed after status change", caller);

        if (!chunk.is_dummy())
          if (chunk.spk.format != out_format)
            return log->err("%s: incorrect output format", caller);
        data_size += chunk.size;
      }

    // well done...
    // (filter may be either full or empty)
    return 0;
  }
};

///////////////////////////////////////////////////////////////////////////////
// Test function

int test_dvdgraph(Log *log)
{
  DVDGraph_test test(log);
  return test.test();
}
