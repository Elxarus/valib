/*
  SPDIF passthrough test
  Compare output of AudioDecoder with reference file
*/

#include "..\log.h"
#include "filters\spdifer.h"
#include "auto_file.h"

extern int test_compare_file(Log *log, Filter *filter, const char *data_file, const char *ref_file);

int test_spdifer(Log *log, const char *data_file, const char *spdif_file, Speakers spk)
{
  log->msg("Testing file %s, compare with %s", data_file, spdif_file);

  Spdifer spdifer;
  spdifer.set_input(spk);

  return test_compare_file(log, &spdifer, data_file, spdif_file);
}
