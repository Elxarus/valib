#include <stdlib.h>
#include <stdio.h>
#include "spk.h"

extern int test_spdifer(const char *fn, const char *fn_spdif, Speakers spk);
int test_spdifer()
{
  int err = 0;
  printf("\n* Spdifer test\n");
  err += test_spdifer("f:\\ac3\\ac3test.ac3", "f:\\ac3\\ac3test.ac3.spdif", Speakers(FORMAT_AC3, 0, 0));
  return err;
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

extern bool test_pcm_passthrough_file(const char *filename, const char *desc, Speakers spk);
int test_pcm_passthrough()
{
  int err = 0;
  printf("\n* PCM passthrough test\n");
  if (!test_pcm_passthrough_file("f:\\ac3\\ac3test_pcm16.raw", "pcm16", Speakers(FORMAT_PCM16, MODE_5_1, 48000, 32767))) err++;

  if (!test_pcm_passthrough_file("f:\\_pes\\lpcm.pes", "crash test",  Speakers(FORMAT_PCM32, MODE_STEREO, 48000, 2147483647))) err++;
  if (!test_pcm_passthrough_file("f:\\ac3\\ac3test_pcm16_norm.raw", "pcm16", Speakers(FORMAT_PCM16, MODE_STEREO, 48000, 32767))) err++;
  if (!test_pcm_passthrough_file("f:\\ac3\\ac3test_pcm16.raw", "pcm16", Speakers(FORMAT_PCM16, MODE_STEREO, 48000, 32767))) err++;
  if (!test_pcm_passthrough_file("f:\\ac3\\ac3test_pcm24.raw", "pcm24", Speakers(FORMAT_PCM24, MODE_STEREO, 48000, 8388607))) err++;
  if (!test_pcm_passthrough_file("f:\\ac3\\ac3test_pcm32.raw", "pcm32", Speakers(FORMAT_PCM32, MODE_STEREO, 48000, 2147483647))) err++;
  if (!test_pcm_passthrough_file("f:\\ac3\\ac3test_pcmfloat.raw", "pcm float", Speakers(FORMAT_PCMFLOAT, MODE_STEREO, 48000, 1.0))) err++;
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


extern int test_ac3_enc(const char *_raw_filename, const char *_desc, Speakers _spk, int _bitrate, int _nframes);
int test_ac3_enc_all()
{
  int err = 0;
  printf("\n* AC3Enc test\n");

  err += test_ac3_enc("f:\\ac3\\ac3test_pcm16_6ch.raw", "5.1, 448kbps", Speakers(FORMAT_PCM16, MODE_5_1, 48000, 65535), 448000, 1874);
  err += test_ac3_enc("f:\\ac3\\ac3test_pcm16.raw", "stereo, 448kbps", Speakers(FORMAT_PCM16, MODE_STEREO, 48000, 65535), 448000, 1875);
  return err;
}





extern int test_filters();
extern int test_float();

int main(int argc, char **argv)
{
  int errors = 0;

  errors += test_filters();
  errors += test_spdifer();
 
  errors += test_float();
  errors += test_pcm_passthrough();

  errors += test_ac3_enc_all();
  errors += test_ac3_parser();
  errors += test_pes_demux();

  printf("-----------------------------------------------------------\n");
  if (errors)
    printf("There are %i errors!\n", errors);
  else
    printf("Ok!");
  return errors;
}