/*
  SPDIF passthrough test
  Compare output of AudioDecoder with reference file
*/

#include "..\utils.h"
#include "filters/spdifer.h"

int test_spdifer(Log *log, Speakers spk, const char *data_file, const char *spdif_file)
{
  log->msg("Testing file %s, compare with %s", data_file, spdif_file);

  Spdifer spdifer;
  return compare_file(log, spk, data_file, &spdifer, spdif_file);
}
