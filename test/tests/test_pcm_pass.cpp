/*
  PCM passthrough test
  AudioProcessor should not alter regular input stream with default settings
*/

#include "..\log.h"
#include "auto_file.h"
#include "filters\proc.h"


int test_compare_file(Log *log, Filter *filter, const char *data_file, const char *ref_file)
{
  const size_t buf_size = 256*1024;

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

  Chunk ichunk;
  Chunk ochunk;
  bool flushing = false;

  while (1)
  {
    // read input data
    data_size = fdata.read(data_buf, buf_size);

    if (data_size)
      ichunk.set(filter->get_input(), data_buf, data_size);
    else
    {
      ichunk.set(filter->get_input(), 0, 0, 0, 0, true);
      flushing = true;
    }

    // process data
    if (!filter->process(&ichunk))
      return log->err("process() failed");

    if (flushing && filter->is_empty())
      return log->err("Filter is empty after receiving end-of-stream");

    while (!filter->is_empty())
    {
      if (!filter->get_chunk(&ochunk))
        return log->err("get_chunk() failed");

      if (ochunk.get_spk().format == FORMAT_LINEAR)
        return log->err("cannot work with linear output");

      while (!ochunk.is_empty())
      {
        // read reference data buffer
        if (!ref_size)
        {
          ref_size = fref.read(ref_buf, buf_size);
          if (!ref_size)
            return log->err("processed output is longer than reference data");
          ref_pos = ref_buf;

          log->status("File pos: %uKb", fref.pos() >> 10);
        }

        // compare
        size_t len = MIN(ochunk.get_size(), ref_size);
        if (memcmp(ref_pos, ochunk.get_rawdata(), len))
          return log->err("data differs");
        ochunk.drop(len);
        ref_pos += len;
        ref_size -= len;
      } // while (!chunk.is_empty())
    } // while (!filter->is_empty())

    if (flushing)
    {
      if (!ochunk.is_eos())
        return log->err("Last chunk is not end-of-stream");

      if (ref_size)
        return log->err("processed output is less than reference data");

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
