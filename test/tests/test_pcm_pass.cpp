/*
  PCM passthrough test
  AudioProcessor should not alter regular input stream with default settings
*/

#include <stdlib.h>
#include <stdio.h> 
#include <math.h>
#include "auto_file.h"
#include "filters\mixer.h"
#include "filters\agc.h"
#include "filters\convert.h"
#include "filters\filter_chain.h"
#include "parsers\file_parser.h"

#include "filters\proc.h"



// PCM test
bool test_pcm_passthrough_file(const char *filename, const char *desc, Speakers spk)
{
  int s;
  int nch = spk.nch();
  switch (spk.format)
  {
    case FORMAT_PCM16:    printf("Testing %ich PCM16 file %s (%s)\n", nch, filename, desc); break;
    case FORMAT_PCM24:    printf("Testing %ich PCM24 file %s (%s)\n", nch, filename, desc); break;
    case FORMAT_PCM32:    printf("Testing %ich PCM32 file %s (%s)\n", nch, filename, desc); break;
    case FORMAT_PCMFLOAT: printf("Testing %ich PCM Float file %s (%s)\n", nch, filename, desc); break;
    default:              printf("Error: unsupported format\n"); return false;
  }

  AutoFile f(filename);
  if (f.eof())
  {
    printf("!!!Error: cannot open file '%s' (%s)\n", filename, desc);
    return false;
  }

  // Buffers
  int buf_size = 8192 * nch * spk.sample_size();
  uint8_t *ibuf = new uint8_t[buf_size];
  uint8_t *obuf = new uint8_t[buf_size];
  if (!ibuf || !obuf)
  {
    printf("!!!Error: cannot allocate buffers\n");
    return false;
  }

  AudioProcessor chain;
  chain.set_input(spk);
  chain.set_output(spk);
  // chunks
  Chunk ichunk;
  Chunk ochunk;

  // process file data
  int read_size = 0;
  int rpos = 0;
  int wpos = 0;

  while (!f.eof())
  {
    printf("filepos: %i\r", f.pos());

    if (wpos >= rpos)
    {
      read_size = f.read(ibuf + wpos, buf_size - wpos);
      ichunk.set_spk(spk);
      ichunk.set_buf(ibuf + wpos, read_size);
      ichunk.set_time(false);
    }
    else
    {
      read_size = f.read(ibuf + wpos, rpos - wpos);
      ichunk.set_spk(spk);
      ichunk.set_buf(ibuf + wpos, read_size);
      ichunk.set_time(false);
    }
    memcpy(obuf + wpos, ibuf + wpos, read_size);
    wpos += read_size;

    if (wpos >= buf_size) wpos = 0;
    if (rpos >= buf_size) rpos = 0;

    // process
    if (!chain.process(&ichunk))
    {
      printf("!!!Error: data processing returns an error\n");
      return false;
    }

    while (!chain.is_empty())
    {
      // retrieve
      if (!chain.get_chunk(&ochunk))
      {
        printf("!!!Error: data retrieval returns an error\n");
        return false;
      }

      if (rpos >= buf_size) rpos = 0;

      // verification
      switch (spk.format)
      {
        case FORMAT_PCM16:
        {
          int16_t *refbuf = (int16_t *)(obuf + rpos);
          int16_t *testbuf = (int16_t *)ochunk.buf;
          s = ochunk.size / sizeof(int16_t);
          while (s--)
          {
            if (*refbuf++ != *testbuf++)
            {
              printf("!!!Error: sample difference\n");
              return false;
            }
          }
          rpos += ochunk.size;
          break;
        }

        case FORMAT_PCM24:
        {
          int24_t *refbuf = (int24_t *)(obuf + rpos);
          int24_t *testbuf = (int24_t *)ochunk.buf;
          s = ochunk.size / sizeof(int24_t);
          while (s--)
            if (*refbuf++ != *testbuf++)
            {
              printf("!!!Error: sample difference\n");
              return false;
            }
          rpos += ochunk.size;
          break;
        }

        case FORMAT_PCM32:
        {
          int32_t *refbuf = (int32_t *)(obuf + rpos);
          int32_t *testbuf = (int32_t *)ochunk.buf;
          s = ochunk.size / sizeof (int32_t);
          while (s--)
            if (*refbuf++ != *testbuf++)
            {
              printf("!!!Error: sample difference\n");
              return false;
            }
          rpos += ochunk.size;
          break;
        }

        case FORMAT_PCMFLOAT:
        {
          float *refbuf = (float *)(obuf + rpos);
          float *testbuf = (float *)ochunk.buf;
          s = ochunk.size / sizeof(float);
          while (s--)
            if (*refbuf++ != *testbuf++)
            {
              printf("!!!Error: sample difference\n");
              return false;
            }
          rpos += ochunk.size;
          break;
        }

      } // switch (spk.format)
    } // while (!chunk.is_empty())

  } // while (!f.eof())
  printf("                 \r");


  return true;
}


