/*
  PCM passthrough test
  AudioProcessor should not alter regular input stream with default settings
*/

#include <stdlib.h>
#include <stdio.h> 
#include <math.h>
#include "..\log.h"
#include "auto_file.h"
#include "filters\mixer.h"
#include "filters\agc.h"
#include "filters\convert.h"
#include "filters\filter_chain.h"
#include "parsers\file_parser.h"

#include "filters\proc.h"



int test_compare_file(Log *log, Filter *filter, const char *data_file, const char *ref_file)
{
  const size_t buf_size = 65536;

  AutoFile fdata(data_file);
  AutoFile fref(ref_file);

  // open files
  if (!fdata.is_open())
    return log->err("Cannot open data file %s", data_file);

  if (!fref.is_open())
    return log->err("Cannot open reference file %s", ref_file);

  // buffers
  DataBuf data_buf;
  DataBuf ref_buf;
  size_t data_size = 0;
  size_t ref_size = 0;
  uint8_t *ref_pos;
  data_buf.allocate(buf_size);
  ref_buf.allocate(buf_size);

  Chunk chunk;
  bool flushing = false;

  while (1)
  {
    // read input data
    data_size = fdata.read(data_buf, buf_size);

    if (data_size)
      chunk.set(filter->get_input(), data_buf, data_size);
    else
    {
      chunk.set(filter->get_input(), 0, 0, 0, 0, true);
      flushing = true;
    }

    // process data
    if (!filter->process(&chunk))
      return log->err("process() failed");

    if (flushing && filter->is_empty())
      return log->err("Filter is empty after receiving end-of-stream");

    while (!filter->is_empty())
    {
      if (!filter->get_chunk(&chunk))
        return log->err("get_chunk() failed");

      if (chunk.get_spk().format == FORMAT_LINEAR)
        return log->err("cannot work with linear output");

      while (!chunk.is_empty())
      {
        // read reference data buffer
        if (!ref_size)
        {
          ref_size = fref.read(ref_buf, buf_size);
          if (!ref_size)
            return log->err("processed output is longer then reference data");
          ref_pos = ref_buf;
        }

        // compare
        size_t len = MIN(chunk.get_size(), ref_size);
        if (memcmp(ref_pos, chunk.get_rawdata(), len))
          return log->err("data differs");
        chunk.drop(len);
        ref_pos += len;
        ref_size -= len;
      } // while (!chunk.is_empty())
    } // while (!filter->is_empty())

    if (flushing)
    {
      if (!chunk.is_eos())
        return log->err("Last chunk is not end-of-stream");
      return 0;
    }
  } // while (1)
};

// PCM passthrough test
int test_pcm_passthrough_file(Log *log, const char *filename, const char *desc, Speakers spk)
{
  log->msg("Testing %s %s %iHz file %s (%s)", spk.format_text(), spk.mode_text(), spk.sample_rate, filename, desc);

  AudioProcessor proc(2048);
  proc.set_input(spk);
  proc.set_output(spk);

  return test_compare_file(log, &proc, filename, filename);
}
/*  



  int s;
  int nch = spk.nch();

  printf("Testing %s %s %iHz file %s (%s)\n", spk.format_text(), spk.mode_text(), spk.sample_rate, filename, desc);

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

  AudioProcessor chain(2048);
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
      ichunk.set(spk, ibuf + wpos, read_size); 
    }
    else
    {
      read_size = f.read(ibuf + wpos, rpos - wpos);
      ichunk.set(spk, ibuf + wpos, read_size);
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
          int16_t *testbuf = (int16_t *)ochunk.get_rawdata();
          s = ochunk.get_size() / sizeof(int16_t);
          while (s--)
          {
            if (*refbuf++ != *testbuf++)
            {
              printf("!!!Error: sample difference\n");
              return false;
            }
          }
          rpos += ochunk.get_size();
          break;
        }

        case FORMAT_PCM24:
        {
          int24_t *refbuf = (int24_t *)(obuf + rpos);
          int24_t *testbuf = (int24_t *)ochunk.get_rawdata();
          s = ochunk.get_size() / sizeof(int24_t);
          while (s--)
            if (*refbuf++ != *testbuf++)
            {
              printf("!!!Error: sample difference\n");
              return false;
            }
          rpos += ochunk.get_size();
          break;
        }

        case FORMAT_PCM32:
        {
          int32_t *refbuf = (int32_t *)(obuf + rpos);
          int32_t *testbuf = (int32_t *)ochunk.get_rawdata();
          s = ochunk.get_size() / sizeof (int32_t);
          while (s--)
            if (*refbuf++ != *testbuf++)
            {
              printf("!!!Error: sample difference\n");
              return false;
            }
          rpos += ochunk.get_size();
          break;
        }

        case FORMAT_PCMFLOAT:
        {
          float *refbuf = (float *)(obuf + rpos);
          float *testbuf = (float *)ochunk.get_rawdata();
          s = ochunk.get_size() / sizeof(float);
          while (s--)
            if (*refbuf++ != *testbuf++)
            {
              printf("!!!Error: sample difference\n");
              return false;
            }
          rpos += ochunk.get_size();
          break;
        }

      } // switch (spk.format)
    } // while (!chunk.is_empty())

  } // while (!f.eof())
  printf("                 \r");


  return true;
}


*/