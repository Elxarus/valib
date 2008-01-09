#include <stdlib.h>
#include <stdio.h>
#include "suite.h"
#include "common.h"



int test_general(Log *log);
int test_bs_convert(Log *log);
int test_crc(Log *log);
int test_syncer(Log *log);
int test_streambuffer(Log *log);
int test_parser_filter(Log *log);

int test_ac3(Log *log);

int test_null(Log *log);
int test_rules(Log *log);

int test_crash(Log *log);
int test_demux(Log *log);
int test_spdifer(Log *log);
int test_despdifer(Log *log);
int test_detector(Log *log);

int test_filtergraph(Log *log);
int test_decodergraph(Log *log);
int test_dvdgraph(Log *log);

int test_proc(Log *log);

EXTERN_SUITE(fir);
FLAT_SUITE(all, "Test session")
  SUITE_FACTORY(fir),
SUITE_END;

int main(int argc, char **argv)
{
  Log log(LOG_SCREEN | LOG_HEADER | LOG_STATUS, "test.log");
  log.msg("Valib build info:\n%s", valib_build_info());

  Test *all = CREATE_SUITE(all);

  if (argc == 1)
    all->run(&log);

  if (argc > 1) for (int i = 1; i < argc; i++)
  {
    Test *t = all->find(argv[i]);
    if (t == 0)
      log.err("Unknown test: %s", argv[i]);
    else
      t->run(&log);
  }

  delete all;

  test_general(&log);

  test_null(&log);
  test_rules(&log);

  test_crc(&log);
  test_syncer(&log);
  test_streambuffer(&log);
  test_bs_convert(&log);
  test_parser_filter(&log);
  test_ac3(&log);

  test_crash(&log);

  test_demux(&log);
  test_spdifer(&log);
  test_despdifer(&log);
  test_detector(&log);

  test_proc(&log);

  test_filtergraph(&log);
  test_decodergraph(&log);
  test_dvdgraph(&log);

  log.msg("-----------------------------------------------------------");

  int total_time = (int)log.get_total_time();
  log.msg("Total time: %i:%02i", total_time / 60, total_time % 60);
  if (log.get_total_errors())
    log.msg("There are %i errors!\n", log.get_total_errors());
  else
    log.msg("Ok!");

  return log.get_total_errors();
}
 

/*
extern int test_ac3_parser_compare(const char *filename, const char *desc);
int test_ac3_parser()
{
  int err = 0;
  printf("\n* AC3Parser test (compare with LibA52)\n");
  err += test_ac3_parser_compare("f:\\ac3\\ac3test.ac3", "general 5.1");
  err += test_ac3_parser_compare("f:\\ac3\\maria.ac3",   "stereo");
  err += test_ac3_parser_compare("f:\\ac3\\surraund.ac3","Dolby surround");
  return err;
}

extern int test_ac3_enc(Log *log, const char *_raw_filename, const char *_desc, Speakers _spk, int _bitrate, int _nframes);
int test_ac3_enc_all(Log *log)
{
  log->open_group("AC3Enc test");
  test_ac3_enc(log, "f:\\ac3\\ac3test_pcm16.raw", "stereo, 448kbps", Speakers(FORMAT_PCM16, MODE_STEREO, 48000, 65535), 448000, 1875);
  test_ac3_enc(log, "f:\\ac3\\ac3test_pcm16_6ch.raw", "5.1, 448kbps", Speakers(FORMAT_PCM16, MODE_5_1, 48000, 65535), 448000, 1874);
  return log->close_group();
}
*/
