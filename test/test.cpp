#include <stdlib.h>
#include <stdio.h>
#include "log.h"
#include "spk.h"

int test_spdifer(Log *log, const char *data_file, const char *spdif_file, Speakers spk);
int test_spdifer(Log *log)
{
  log->open_group("Spdifer test");
  test_spdifer(log, "f:\\ac3\\ac3test.ac3", "f:\\ac3\\ac3test.spdif", Speakers(FORMAT_AC3, 0, 0));
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

extern int test_pcm_passthrough_file(Log *log, const char *filename, const char *desc, Speakers spk);
int test_pcm_passthrough(Log *log)
{
  log->open_group("PCM passthrough test");
  test_pcm_passthrough_file(log, "f:\\ac3\\ac3test_pcm16.raw", "pcm16", Speakers(FORMAT_PCM16, MODE_5_1, 48000, 32767));
  test_pcm_passthrough_file(log, "f:\\ac3\\ac3test_pcm16_norm.raw", "pcm16", Speakers(FORMAT_PCM16, MODE_STEREO, 48000, 32767));
  test_pcm_passthrough_file(log, "f:\\ac3\\ac3test_pcm16.raw", "pcm16", Speakers(FORMAT_PCM16, MODE_STEREO, 48000, 32767));
  test_pcm_passthrough_file(log, "f:\\ac3\\ac3test_pcm24.raw", "pcm24", Speakers(FORMAT_PCM24, MODE_STEREO, 48000, 8388607));
  test_pcm_passthrough_file(log, "f:\\ac3\\ac3test_pcm32.raw", "pcm32", Speakers(FORMAT_PCM32, MODE_STEREO, 48000, 2147483647));
  test_pcm_passthrough_file(log, "f:\\ac3\\ac3test_pcmfloat.raw", "pcm float", Speakers(FORMAT_PCMFLOAT, MODE_STEREO, 48000, 1.0));
  test_pcm_passthrough_file(log, "f:\\_pes\\lpcm.pes", "crash test",  Speakers(FORMAT_PCM32, MODE_STEREO, 48000, 2147483647));
  return log->close_group();
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





extern int test_filters(Log *log);
extern int test_float(Log *log);



extern int test_converter(Log *log);


int main(int argc, char **argv)
{
  ScreenLog log;
  log.open_group("Test session");

  test_ac3_enc_all(&log);

  test_filters(&log);
  test_spdifer(&log);
 
  test_float(&log);
  test_pcm_passthrough(&log);

  test_ac3_parser();
  test_pes_demux();

  log.close_group();
  log.msg("-----------------------------------------------------------");
  if (log.get_total_errors())
    log.msg("There are %i errors!\n", log.get_total_errors());
  else
    log.msg("Ok!");

  return log.get_total_errors();
}
