#include "source/raw_source.h"
#include "suite.h"

const TestResult test_passed(true);
const TestResult test_failed(false);

const char *compact_suffix(int size)
{
  if (size > 10000000) return "M";
  if (size > 10000) return "K";
  return "";
}

size_t compact_size(size_t size)
{
  if (size > 10000000) return size / 1000000;
  if (size > 10000) return size / 1000;
  return size;
}

int compare(Log *log, Source *src, Source *ref)
{
  // if reference format is FORMAT_RAWDATA then filter output
  // format is supposed to be raw format (not FORMAT_LINEAR)

  // todo: add couters to prevent infinite loop

  Chunk src_chunk;
  Chunk ref_chunk;

  size_t src_size = 0;
  size_t ref_size = 0;

  Speakers spk;
  size_t len;
  int ch;

  while (1)
  {
    if (!src_chunk.size)
    {
      if (src->is_empty()) break;
      if (!src->get_chunk(&src_chunk)) return log->err("src->get_chunk() fails");
      if (ref_chunk.is_dummy()) continue;
    }

    if (!ref_chunk.size)
    {
      if (ref->is_empty()) break;
      if (!ref->get_chunk(&ref_chunk)) return log->err("ref->get_chunk() fails");
      if (ref_chunk.is_dummy()) continue;
    }

    ///////////////////////////////////////////////////////
    // Check that stream configurstions are equal
    // Do not check if output is raw and reference format is FORMAT_RAWDATA 
    // (reference format is unspecified)

    if (ref_chunk.spk != src_chunk.spk)
      if (src_chunk.spk.format == FORMAT_LINEAR || ref_chunk.spk.format != FORMAT_RAWDATA)
        return log->err("Different speaker configurations");

    ///////////////////////////////////////////////////////
    // Compare data

    spk = src_chunk.spk;
    len = MIN(ref_chunk.size, src_chunk.size);
    if (spk.format == FORMAT_LINEAR)
    {
      // compare linear
      for (ch = 0; ch < spk.nch(); ch++)
        if (memcmp(src_chunk.samples[ch], ref_chunk.samples[ch], len * sizeof(sample_t)))
          return log->err("Data differs");
    }
    else
    {
      for (size_t i = 0; i < len; i++)
        if (src_chunk.rawdata[i] != ref_chunk.rawdata[i])
          return log->err("Data differs at pos %i (0x%x), chunk pos %i (0x%x)", src_size + i, src_size + i, i, i);
    }
    src_size += len;
    ref_size += len;
    src_chunk.drop(len);
    ref_chunk.drop(len);

    ///////////////////////////////////////////////////////
    // Statistics

    log->status("%u%s      ", compact_size(src_size), compact_suffix(src_size));

  } // while (1)

  /////////////////////////////////////////////////////////
  // Verify stream lengths

  if (!src->is_empty() || src_chunk.size)
    return log->err("output is longer than reference");

  if (!ref->is_empty() || ref_chunk.size)
    return log->err("reference is longer than output");

  return 0;
}

int compare(Log *log, Source *src, Filter *src_filter, Source *ref, Filter *ref_filter)
{
  SourceFilter sf(src, src_filter);
  SourceFilter rf(ref, ref_filter);
  return compare(log, &sf, &rf);
}

int compare_file(Log *log, Speakers spk_src, const char *fn_src, Filter *filter, const char *fn_ref)
{
  RAWSource src;
  RAWSource ref;

  Speakers spk_ref(FORMAT_RAWDATA, 0, 0);

  log->msg("Testing transform %s => %s", fn_src, fn_ref);

  if (!src.open(spk_src, fn_src))
    return log->err("cannot open source file '%s' of format %s %s %ikHz", fn_src, spk_src.format_text(), spk_src.mode_text(), spk_src.sample_rate);

  if (!ref.open(spk_ref, fn_ref))
    return log->err("cannot open reference file '%s'", fn_ref);

  if (!filter->set_input(spk_src))
    return log->err("cannot set filter input format to %s %s %ikHz", spk_src.format_text(), spk_src.mode_text(), spk_src.sample_rate);

  int result = compare(log, &src, filter, &ref);

  // Now we MUST reset the filter to drop 
  // possible dependency on our private source.
  // If compare function detects an error
  // filter may be full and hold reference to
  // buffer allocated by our file source.
  filter->reset();

  return result;
}
