/*
  NullFilter noise passthrough test
*/

#include "log.h"
#include "common.h"
#include "source\raw_source.h"
#include "filters\spdifer.h"

int test_spdifer(Log *log);
int test_spdifer_file(Log *log, const char *raw_file, const char *spdif_file);


int test_spdifer(Log *log)
{
  log->open_group("Spdifer test");
  test_spdifer_file(log, "f:\\ac3\\ac3test.ac3", "f:\\ac3\\ac3test.ac3.spdif");
  return log->close_group();
}

int test_spdifer_file(Log *log, const char *raw_file, const char *spdif_file)
{
  log->msg("Testing transform %s => %s", raw_file, spdif_file);

  RAWSource raw_src(unk_spk, raw_file);
  RAWSource spdif_src(unk_spk, spdif_file);
  Spdifer spdifer;

  if (!raw_src.is_open()) 
    return log->err("cannot open file %s", raw_file);

  if (!spdif_src.is_open()) 
    return log->err("cannot open file %s", spdif_file);

  return compare(log, &raw_src, &spdifer, &spdif_src);
}

