/*
  SPDIF passthrough test
  Compare output of AudioDecoder with reference file
*/

#include "..\common.h"
#include "source\raw_source.h"
#include "filters/spdifer.h"
#include "filters/decoder.h"
#include "filters/counter.h"
#include "filters/filter_chain.h"

int test_spdifer_big(Log *log);
int test_spdifer_file(Log *log, Speakers spk, const char *file);

int test_spdifer(Log *log)
{
  log->open_group("Spdifer test");
  test_spdifer_file(log, Speakers(FORMAT_AC3, 0, 0), "test.ac3");
  return log->close_group();
}


int test_spdifer_file(Log *log, Speakers spk, const char *file)
{
  log->msg("Testing file %s", file);
  RAWSource src_tst(spk, file);
  RAWSource src_ref(spk, file);

  if (!src_tst.is_open() || !src_ref.is_open())
    return log->err("Cannot open file %s", file);

  Spdifer spdifer;
  AudioDecoder dec_tst;
  AudioDecoder dec_ref;
  Counter cnt_spdif;
  Counter cnt_samples;

  FilterChain tst;

  tst.add_back(&spdifer, "Spdifer");
  tst.add_back(&cnt_spdif, "Bytes counter");
  tst.add_back(&dec_tst, "Decoder");
  tst.add_back(&cnt_samples, "Samples counter");

  return compare(log, &src_tst, &tst, &src_ref, &dec_ref);
}


/*
int test_spdifer(Log *log, Speakers spk, const char *data_file, const char *spdif_file)
{
  log->msg("Testing file %s, compare with %s", data_file, spdif_file);

  Spdifer spdifer;
  return compare_file(log, spk, data_file, &spdifer, spdif_file);
}
*/