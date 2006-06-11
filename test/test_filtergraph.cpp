/*
  FilterGraph test
  * FilterGraph must behave like NullFilter
*/

#include "log.h"
#include "filter_tester.h"
#include "filter_graph.h"
#include <source\noise.h>
#include <source\raw_source.h>
#include <win32\cpu.h>
#include "common.h"


///////////////////////////////////////////////////////////////////////////////
// Test class

class FilterGraph_test
{
protected:
  FilterTester t;
  FilterGraph filter_graph;
  Log *log;

public:
  FilterGraph_test(Log *_log)
  {
    log = _log;
    t.link(&filter_graph, log);
  }

  int test()
  {
    log->open_group("FilterGraph test");
    null_transform();
    return log->close_group();
  }

  void null_transform()
  {
    /////////////////////////////////////////////////////////
    // Transform test
    log->open_group("Null transform test");

    Speakers spk;
    int seed = 9754397;
    Noise src;
    Noise ref;

    // Rawdata test
    log->msg("Rawdata null transform test");
    spk.set(FORMAT_PCM16, MODE_STEREO, 48000, 65536);

    src.set_seed(seed);
    ref.set_seed(seed);

    if (src.set_output(spk) &&
        ref.set_output(spk) &&
        t.set_input(spk))
      compare(log, &src, &t, &ref);
    else
      log->err("Init failed");

    // Linear test
    log->msg("Linear null transform test");
    spk.set(FORMAT_LINEAR, MODE_STEREO, 48000, 1.0);

    src.set_seed(seed);
    ref.set_seed(seed);

    if (src.set_output(spk) &&
        ref.set_output(spk) &&
        t.set_input(spk))
      compare(log, &src, &t, &ref);
    else
      log->err("Init failed");

    log->close_group();
  }
};

///////////////////////////////////////////////////////////////////////////////
// Test function

int test_filtergraph(Log *log)
{
  FilterGraph_test test(log);
  return test.test();
}
