/*
  AC3Parser test
  Compare VALib AC3 parser and LibA52 parser as reference
*/

#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <string.h> 

#include "parsers\ac3\ac3_parser.h"
#include "a52_parser.h"
#include "a52_internal.h"

bool test_compare(FILE *f);


int test_ac3_parser_compare(const char *filename, const char *desc)
{
  printf("Testing file %s (%s)...\n", filename, desc);

  FILE *f;
  if (!(f = fopen(filename, "rb")))
  {
    printf("!!!Error: Cannot open file %s for reading\n", filename);
    return 1;
  }

  bool ok = test_compare(f);
  fclose(f);

  if (!ok)
  {
    printf("!!!Error!\n", filename);
    return 1;
  }

  return 0;
}

bool 
test_compare(FILE *f)
{
  AC3Parser ac3;
  A52Parser a52;

  const buf_size = 65536;
  uint8_t buf[buf_size];

  uint8_t *buf_ptr, *buf_ptr2;
  int len, len2;
  bool ok, ok2;
  int filepos;
  bool err = false;

  while (!feof(f))
  {
    buf_ptr = buf;
    filepos = ftell(f);
    len = fread(buf, 1, buf_size, f);

    buf_ptr2 = buf_ptr;
    len2 = len;

    while (buf_ptr < buf + len)
    {
      ok = false;
      ok2 = false;

      if (ac3.get_frames() == 255)
        ok = ok;

      if (ac3.load_frame(&buf_ptr2, buf + len))
        if (ac3.decode_frame())
          ok2 = true;

      if (a52.load_frame(&buf_ptr, buf + len))
        if (a52.decode_frame())
          ok = true;


      if (ac3.get_errors())
        ok = ok;

      if (ok != ok2 || buf_ptr != buf_ptr2 || len != len2)
      {
        printf("\nSomething different at frame %i, filepos 0x%x\n", ac3.get_frames(), filepos + (buf_ptr - buf));
        err = true;
      }

      if (ok && ok2)
      {
        if (ac3.get_spk() != a52.get_spk())
        {
          printf("\nDifferent speaker configurations! at frame %i, filepos 0x%x\n", ac3.get_frames(), filepos + (buf_ptr - buf));
          err = true;
        }

        if (ac3.get_frame_size() != a52.get_frame_size() || memcmp(ac3.get_frame(), a52.get_frame(), ac3.get_frame_size()))
        {
          printf("\nDifferent raw frame! at frame %i, filepos 0x%x\n", ac3.get_frames(), filepos + (buf_ptr - buf));
          err = true;
        }

        Speakers spk = ac3.get_spk();
        int nfchans = spk.nch();
        if (spk.lfe()) nfchans--;

        for (int ch = 0; ch < nfchans; ch++)
          if (memcmp(ac3.exps[ch], a52.a52_state->fbw_expbap[ch].exp, ac3.endmant[ch]))
          {
            printf("\nDifferent fbw exponents (ch=%i)! at frame %i, filepos 0x%x\n", ch, ac3.get_frames(), filepos + (buf_ptr - buf));
            err = true;
          }
  
        if (memcmp(ac3.cplexps + ac3.cplstrtmant, a52.a52_state->cpl_expbap.exp + a52.a52_state->cplstrtmant, ac3.cplendmant - ac3.cplstrtmant))
        {
          printf("\nDifferent cpl exponents! at frame %i, filepos 0x%x\n", ac3.get_frames(), filepos + (buf_ptr - buf));
          err = true;
        }

        if (spk.lfe())
          if (memcmp(ac3.lfeexps, a52.a52_state->lfe_expbap.exp, 7))
          {
            printf("\nDifferent lfe exponents! at frame %i, filepos 0x%x\n", ac3.get_frames(), filepos + (buf_ptr - buf));
            err = true;
          }

        if (memcmp(ac3.cplexps + ac3.cplstrtmant, a52.a52_state->cpl_expbap.exp + a52.a52_state->cplstrtmant, ac3.cplendmant - ac3.cplstrtmant))
        {
          printf("\nDifferent coupling exponents! at frame %i, filepos 0x%x\n", ac3.get_frames(), filepos + (buf_ptr - buf));
          err = true;
        }

        int nsamples = ac3.get_nsamples();
        samples_t ac3_samples = ac3.get_samples();
        samples_t a52_samples = a52.get_samples();

        for (ch = 0; ch < spk.nch(); ch++)
          if (memcmp(ac3_samples[ch], a52_samples[ch], nsamples * sizeof(sample_t)))
          {
            double rms = 0;
            for (int i = 0; i < AC3_FRAME_SAMPLES; i++)
              rms += (ac3_samples[ch][i] - a52_samples[ch][i]) * (ac3_samples[ch][i] - a52_samples[ch][i]);
            rms = sqrt(rms / AC3_FRAME_SAMPLES);
            printf("\nDifferent samples (ch=%i, RMS = %e)! at frame %i, filepos 0x%x\n", ch, rms, ac3.get_frames(), filepos + (buf_ptr - buf));
            err = true;
          }
      }

      printf("Frames: %i, errors: %i\r", ac3.get_frames(), ac3.get_errors());
    } // while (len)
  } // while (!feof(f)

  printf("                               \r");

  return !err;
}

