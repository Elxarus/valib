#include <memory.h>
#include "common.h"
#include "source/noise.h"
#include "source/raw_source.h"


int compare(Log *log, Source *src, Filter *src_filter, Source *ref, Filter *ref_filter)
{
  // if reference format is FORMAT_UNKNOWN then filter output
  // format is supposed to be raw format (not FORMAT_LINEAR)

  // todo: add couters to prevent infinite loop

  Chunk si_chunk;
  Chunk so_chunk;
  Chunk ri_chunk;
  Chunk ro_chunk;

  size_t isize = 0;
  size_t osize = 0;
  size_t rsize = 0;

  Speakers spk;
  size_t len;
  int ch;

  #define SAFE_CALL(call)     \
  {                           \
    if (!call)                \
      return log->err(#call); \
  }


  while (1)
  {
    if (!so_chunk.size)
    {
      // try to fill the filter
      if (src_filter->is_empty())
      {
        if (src->is_empty()) break;
        SAFE_CALL(src->get_chunk(&si_chunk));
        SAFE_CALL(src_filter->process(&si_chunk));
      }

      // try to get data
      if (!src_filter->is_empty())
        SAFE_CALL(src_filter->get_chunk(&so_chunk))
      else
        continue;
    }

    if (!ro_chunk.size)
    {
      if (ref_filter)
      {
        // try to fill the filter
        if (ref_filter->is_empty())
        {
          if (ref->is_empty()) break;
          SAFE_CALL(ref->get_chunk(&ri_chunk));
          SAFE_CALL(ref_filter->process(&ri_chunk));
        }

        // try to get data
        if (!ref_filter->is_empty())
          SAFE_CALL(ref_filter->get_chunk(&ro_chunk))
        else
          continue;
      }
      else
      {
        // try to get data
        if (ref->is_empty()) break;
        SAFE_CALL(ref->get_chunk(&ro_chunk));
      }
    }

    // Now we have both output and reference chunks are loaded 
    // with data or end-of stream signaled

    ///////////////////////////////////////////////////////
    // Statistics

    if (osize < 10000)
      log->status("Pos: %u    ", osize);
    if (osize < 10000000)
      log->status("Pos: %uK    ", osize/1000);
    else
      log->status("Pos: %uM    ", osize/1000000);

    ///////////////////////////////////////////////////////
    // Check that stream configurstions are equal
    // Do not check if output is raw and reference format is FORMT_UNKNOWN 
    // (reference format is unspecified)

    if (ro_chunk.spk != so_chunk.spk)
      if (so_chunk.spk.format == FORMAT_LINEAR || ro_chunk.spk.format != FORMAT_UNKNOWN)
        return log->err("Different speaker configurations");

    ///////////////////////////////////////////////////////
    // Compare data

    spk = so_chunk.spk;
    len = MIN(ro_chunk.size, so_chunk.size);
    if (spk.format == FORMAT_LINEAR)
    {
      // compare linear
      for (ch = 0; ch < spk.nch(); ch++)
        if (memcmp(so_chunk.samples[ch], ro_chunk.samples[ch], len * sizeof(sample_t)))
          return log->err("Data differs");
    }
    else
    {
      // compare raw data
      if (memcmp(so_chunk.rawdata, ro_chunk.rawdata, len))
        return log->err("Data differs");
    }
    osize += len;
    rsize += len;
    so_chunk.drop(len);
    ro_chunk.drop(len);
  }

  if (so_chunk.size)
    log->err("output output is longer than reference");

  if (ro_chunk.size)
    log->err("reference is longer than output");

  return 0;
}

int compare_file(Log *log, Speakers spk_src, const char *fn_src, Filter *filter, const char *fn_ref)
{
  RAWSource src;
  RAWSource ref;

  Speakers spk_ref(FORMAT_UNKNOWN, 0, 0);

  log->msg("Testing transform %s => %s", fn_src, fn_ref);

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
