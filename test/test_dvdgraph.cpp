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

static const vtime_t time_per_test = 1.0; // 1 sec for each speed test

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
  FilterTester t;
  DVDGraph dvd;
  Log *log;

public:
  DVDGraph_test(Log *_log)
  {
    log = _log;
    t.link(&dvd, log);
  }

  int test()
  {
    log->open_group("DVDGraph test");
    transform();
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

    compare_file(log, Speakers(FORMAT_PES, 0, 0), file1_src, &dvd, file1_out);
    compare_file(log, Speakers(FORMAT_PES, 0, 0), file2_src, &dvd, file2_out);

    // SPDIF to SPDIF transfrm with format changes (todo)
    // dvd.use_spdif = true;
    // dvd.user_spk = Speakers(FORMAT_PCM16, 0, 0);
    // compare_file(log, Speakers(FORMAT_SPDIF, 0, 0), file3_src, &t, file3_out);

    // todo: transform test with decode
  }

};

///////////////////////////////////////////////////////////////////////////////
// Test function

int test_dvdgraph(Log *log)
{
  DVDGraph_test test(log);
  return test.test();
}
