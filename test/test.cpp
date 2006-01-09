#include <stdlib.h>
#include <stdio.h>
#include "common.h"


int test_float(Log *log);
int test_empty(Log *log);
int test_null(Log *log);
int test_proc(Log *log);
int test_crash(Log *log);
int test_spdifer(Log *log);


int main(int argc, char **argv)
{
  Log log(LOG_SCREEN | LOG_HEADER | LOG_STATUS, "test.log");
  log.open_group("Test session");

  test_float(&log);
//  test_empty(&log);
  test_null(&log);
  test_proc(&log);
  test_crash(&log);
  test_spdifer(&log);

  log.close_group();
  log.msg("-----------------------------------------------------------");
  if (log.get_total_errors())
    log.msg("There are %i errors!\n", log.get_total_errors());
  else
    log.msg("Ok!");

  return log.get_total_errors();
}

/*
int test_spdifer(Log *log, Speakers spk, const char *data_file, const char *spdif_file);
int test_spdifer(Log *log)
{
  log->open_group("Spdifer test");
  test_spdifer(log, Speakers(FORMAT_AC3, 0, 0), "f:\\ac3\\ac3test.ac3", "f:\\ac3\\ac3test.spdif");
  return log->close_group();
}


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

extern int test_pes_demux(const char *filename);
int test_pes_demux()
{
  int err = 0;
  printf("\n* MPEG1/2 PES demultiplexor test\n");

  err += test_pes_demux("f:\\ac3\\ac3test_pcm16.raw");   // crash test

  err += test_pes_demux("f:\\ac3\\ac3test.ac3.pes");     // general ac3 pes test

  err += test_pes_demux("f:\\_pes\\ac3.pes");            // ac3-pes
  err += test_pes_demux("f:\\_pes\\mpa.pes");            // mpa-pes
  err += test_pes_demux("f:\\_pes\\lpcm.pes");           // lpcm-pes
  // todo: add dts-pes

  err += test_pes_demux("f:\\_dvd\\mpa.vob");            // mpeg2
  err += test_pes_demux("f:\\_dvd\\dts.vob");            // mpeg2
  // todo: add mpeg1 stream test
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







extern int test_converter(Log *log);

#include "source/noise.h"

class PassthroughFilter : public NullFilter
{
public:
  PassthroughFilter() {};
  bool query_input(Speakers _spk) const { return true; }
};

int null_test(Log *log)
{
  log->open_group("NullFilter noise passthrough test");

  Speakers spk;
  int seed = 2435346;
  Noise src;
  Noise ref;
  PassthroughFilter filter;

  // Rawdata test
  log->msg("Rawdata test");
  spk.set(FORMAT_PCM16, MODE_STEREO, 48000, 65536);

  src.set_seed(seed);
  ref.set_seed(seed);

  if (src.set_output(spk) &&
      ref.set_output(spk) &&
      filter.set_input(spk))
    compare(log, &src, &filter, &ref);
  else
    log->err("Init failed");

  // Linear test
  log->msg("Linear test");
  spk.set(FORMAT_LINEAR, MODE_STEREO, 48000, 1.0);

  src.set_seed(seed);
  ref.set_seed(seed);

  if (src.set_output(spk) &&
      ref.set_output(spk) &&
      filter.set_input(spk))
    compare(log, &src, &filter, &ref);
  else
    log->err("Init failed");

  return log->close_group();
}


extern int test_filters(Log *log);
extern int test_float(Log *log);
extern int passthrough_noise(Log *log);
extern int test_spdifer(Log *log);

int main(int argc, char **argv)
{
  Log log;
  log.open_group("Test session");


  test_spdifer(&log);

  test_float(&log);
  test_filters(&log);
  null_test(&log);
  passthrough_noise(&log);

 

  test_ac3_parser();
  test_ac3_enc_all(&log);
  test_pes_demux();

  log.close_group();
  log.msg("-----------------------------------------------------------");
  if (log.get_total_errors())
    log.msg("There are %i errors!\n", log.get_total_errors());
  else
    log.msg("Ok!");

  return log.get_total_errors();
}
*/