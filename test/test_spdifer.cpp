/*
  NullFilter noise passthrough test
*/

#include "log.h"
#include "filter_tester.h"
#include "filters\spdifer.h"
#include "filters\demux.h"
#include "common.h"

int test_spdifer(Log *log);
int test_spdifer_file(Log *log, const char *raw_file, const char *spdif_file);


int test_spdifer(Log *log)
{
  Spdifer spdifer;
  FilterTester spdifer_tester(&spdifer, log);

  log->open_group("Spdifer test");
  compare_file(log, Speakers(FORMAT_AC3, 0, 0), "test.ac3", &spdifer_tester, "test.ac3.spdif");
  compare_file(log, Speakers(FORMAT_DTS, 0, 0), "test.dts", &spdifer_tester, "test.dts.spdif");
  compare_file(log, Speakers(FORMAT_MPA, 0, 0), "test.mp2", &spdifer_tester, "test.mp2.spdif");
  compare_file(log, Speakers(FORMAT_UNKNOWN, 0, 0), "test.all", &spdifer_tester, "test.all.spdif");
  log->close_group();

  Demux demux;
  FilterTester demux_tester(&demux, log);

  log->open_group("Demuxer test");
  compare_file(log, Speakers(FORMAT_PES, 0, 0), "test.ac3.pes", &demux_tester, "test.ac3");
  compare_file(log, Speakers(FORMAT_PES, 0, 0), "test.dts.pes", &demux_tester, "test.dts");
  compare_file(log, Speakers(FORMAT_PES, 0, 0), "test.mp2.pes", &demux_tester, "test.mp2");
  compare_file(log, Speakers(FORMAT_PES, 0, 0), "test.lpcm.pes", &demux_tester, "test.lpcm");
  compare_file(log, Speakers(FORMAT_PES, 0, 0), "test.all2.pes", &demux_tester, "test.all2");
  return log->close_group();

}
