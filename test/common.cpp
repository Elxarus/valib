#include <memory.h>
#include "common.h"
#include "source/noise.h"
#include "source/raw_source.h"


int compare(Log *log, Source *src, Filter *src_filter, Source *ref, Filter *ref_filter)
{
  // if reference format is FORMAT_UNKNOWN then filter output
  // format is supposed to be raw format (not FORMAT_LINEAR)

  // todo: add couters to prevent infinite loop

  Chunk si_chunk(unk_spk, 0, 0, false, 0, false);
  Chunk so_chunk(unk_spk, 0, 0, false, 0, false);
  Chunk ri_chunk(unk_spk, 0, 0, false, 0, false);
  Chunk ro_chunk(unk_spk, 0, 0, false, 0, false);

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
    //   if (!src_filter->get_from(&ochunk, src))
    //     return log->err("src_filter->get_from()");

    while (!so_chunk.size && !so_chunk.eos)
    {
      while (src_filter->is_empty())
      {
        if (!src->get_chunk(&si_chunk))
          return log->err("src->get_chunk()");

        if (!src_filter->process(&si_chunk))
          return log->err("src_filter->process()");
      }

      if (!src_filter->get_chunk(&so_chunk))
        return log->err("src_filter->get_chunk()");
    }

    if (ref_filter)
    {
      while (!ro_chunk.size && !ro_chunk.eos)
      {
        while (src_filter->is_empty())
        {
          if (!ref->get_chunk(&ri_chunk))
            return log->err("ref->get_chunk()");

          if (!ref_filter->process(&ri_chunk))
            return log->err("ref_filter->process()");
        }

        if (!ref_filter->get_chunk(&ro_chunk))
          return log->err("ref_filter->get_chunk()");
      }
    }
    else
    {
      while (!ro_chunk.size && !ro_chunk.eos)
        if (!ref->get_chunk(&ro_chunk))
          return log->err("ref->get_chunk()");
    }

    // Now we have both output and reference chunks are loaded 
    // with data or end-of stream signaled

    // Check that stream configurstions are equal
    // Do not check if output is raw and reference format is FORMT_UNKNOWN

    if (ro_chunk.spk != so_chunk.spk)
      if (so_chunk.spk.format == FORMAT_LINEAR || ro_chunk.spk.format != FORMAT_UNKNOWN)
        return log->err("Different speaker configurations");

    // Compare data

    spk = so_chunk.spk;
    len = MIN(ro_chunk.size, so_chunk.size);
    if (spk.format == FORMAT_LINEAR)
    {
      log->status("Pos: %.3fs (%usm)", double(osize) / spk.sample_rate, osize);

      // compare linear
      for (ch = 0; ch < spk.nch(); ch++)
        if (memcmp(so_chunk.samples[ch], ro_chunk.samples[ch], len * sizeof(sample_t)))
          return log->err("Data differs");
    }
    else
    {
      log->status("Pos: %u", osize);

      // compare raw data
      if (memcmp(so_chunk.rawdata, ro_chunk.rawdata, len))
        return log->err("Data differs");
    }
    osize += len;
    rsize += len;
    so_chunk.drop(len);
    ro_chunk.drop(len);

    // now we have either output or reference chunk empty

    if (so_chunk.eos && ro_chunk.size > so_chunk.size)
      return log->err("Source stream ends at %u. Reference stream is longer...", osize + so_chunk.size);

    if (ro_chunk.eos && so_chunk.size > ro_chunk.size)
      return log->err("Reference stream ends at %u. Processed stream is longer...", rsize + ro_chunk.size);

    if (ro_chunk.eos && so_chunk.eos)
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
