#include <memory.h>
#include "common.h"
#include "source/noise.h"
#include "source/raw_source.h"


int compare(Log *log, Source *src, Filter *filter, Source *ref)
{
  // if reference format is FORMAT_UNKNOWN then filter output
  // format is supposed to be raw format (not FORMAT_LINEAR)

  // todo: add couters to prevent infinite loop

  Chunk ichunk(unk_spk, 0, 0, false, 0, false);
  Chunk ochunk(unk_spk, 0, 0, false, 0, false);
  Chunk rchunk(unk_spk, 0, 0, false, 0, false);

  size_t isize = 0;
  size_t osize = 0;
  size_t rsize = 0;

  Speakers spk;
  size_t len;
  int ch;

  while (1)
  {
    // We need detailed information in case of 
    // errors, so we have to expand cycles.
    // The following cycle may be rewrited as:
    //
    // while (!ochunk.get_size() && !ochunk.is_eos())
    //   if (!filter->get_from(&ochunk, src))
    //     return log->err("filter->get_from()");

    while (!ochunk.get_size() && !ochunk.is_eos())
    {
      while (filter->is_empty())
      {
        ichunk.set_empty();
        while (!ichunk.get_size() && !ichunk.is_eos())
        {
          if (!src->get_chunk(&ichunk))
            return log->err("src->get_chunk()");
          isize += ichunk.get_size();
        }

        if (!filter->process(&ichunk))
          return log->err("filter->process()");
      }

      while (!ochunk.get_size() && !ochunk.is_eos() && !filter->is_empty())
        if (!filter->get_chunk(&ochunk))
          return log->err("filter->get_chunk()");
    }

    while (!rchunk.get_size() && !rchunk.is_eos())
      if (!ref->get_chunk(&rchunk))
        return log->err("ref->get_chunk()");

    // Now we have both output and reference chunks loaded or 
    // end-of stream signaled

    if (rchunk.get_spk() != ochunk.get_spk())
      if (ochunk.get_spk().format == FORMAT_LINEAR || rchunk.get_spk().format != FORMAT_UNKNOWN)
        return log->err("Different speaker configurations");

    spk = ochunk.get_spk();
    len = MIN(rchunk.get_size(), ochunk.get_size());
    if (spk.format == FORMAT_LINEAR)
    {
      log->status("Pos: %.3fs (%usm)", double(osize) / spk.sample_rate, osize);

      // compare linear
      for (ch = 0; ch < spk.nch(); ch++)
        if (memcmp(ochunk[ch], rchunk[ch], len * sizeof(sample_t)))
          return log->err("Data differs");
    }
    else
    {
      log->status("Pos: %u", osize);

      // compare raw data
      if (memcmp(ochunk.get_rawdata(), rchunk.get_rawdata(), len))
        return log->err("Data differs");
    }
    osize += len;
    rsize += len;
    ichunk.drop(len);
    ochunk.drop(len);
    rchunk.drop(len);

    // now we have either output or reference chunk empty

    if (ochunk.is_eos() && rchunk.get_size() > ochunk.get_size())
      return log->err("Processed stream ends at %u. Reference stream is longer...", isize);

    if (rchunk.is_eos() && ochunk.get_size() > rchunk.get_size())
      return log->err("Reference stream ends at %u. Processed stream is longer...", rsize);

    if (rchunk.is_eos() && ochunk.is_eos())
      return 0;
  }
}

int compare_file(Log *log, Speakers spk_src, const char *fn_src, Filter *filter, const char *fn_ref)
{
  RAWSource src;
  RAWSource ref;

  Speakers spk_ref(FORMAT_UNKNOWN, 0, 0);

  if (!src.open(spk_src, fn_src))
    return log->err("cannot open source file '%s' of format %s %s %ikHz", fn_src, spk_src.format_text(), spk_src.mode_text(), spk_src.sample_rate);

  if (!ref.open(spk_ref, fn_ref))
    return log->err("cannot open reference file '%s'", fn_ref);

  if (!filter->set_input(spk_src))
    return log->err("cannot set filter input format to %s %s %ikHz", spk_src.format_text(), spk_src.mode_text(), spk_src.sample_rate);

  return compare(log, &src, filter, &ref);
}

int crash_test(Log *log, Speakers spk, Filter *filter)
{
  Noise noise;
  NullSink null;
  
  noise.set_seed(2356437);
  if (!noise.set_output(spk, 1024*1024*1024))
    return log->err("cannot open noise source with format %s %s %ikHz", spk.format_text(), spk.mode_text(), spk.sample_rate);

  if (!filter->set_input(spk))
    return log->err("cannot set filter input format to %s %s %ikHz", spk.format_text(), spk.mode_text(), spk.sample_rate);

  if (!filter->transform(&noise, &null))
    return log->err("filter failed");

  return 0;
};
