#include <boost/test/unit_test.hpp>
#include <math.h>

#include "source/raw_source.h"
#include "source/source_filter.h"
#include "filters/convert.h"
#include "suite.h"

void compare(Source *src, Source *ref)
{
  // if reference format is FORMAT_RAWDATA then filter output
  // format is supposed to be raw format (not FORMAT_LINEAR)
  // todo: add couters to prevent infinite loop

  size_t pos = 0;
  size_t len;

  Speakers src_spk, ref_spk;
  Chunk src_chunk, ref_chunk;

  while (true)
  {
    if (src_chunk.is_empty())
      if (!src->get_chunk(src_chunk))
      {
        src_chunk.clear();
        break;
      }

    if (ref_chunk.is_empty())
      if (!ref->get_chunk(ref_chunk))
      {
        src_chunk.clear();
        break;
      }

    if (src_chunk.is_empty() || ref_chunk.is_empty())
      continue;

    ///////////////////////////////////////////////////////
    // Check that stream configurations are equal
    // Do not check if output is raw and reference format is FORMAT_RAWDATA 
    // (reference format is unspecified)

    src_spk = src->get_output();
    ref_spk = ref->get_output();
    if (ref_spk != src_spk)
      if (src_spk.is_linear() || ref_spk.format != FORMAT_RAWDATA)
      {
        BOOST_ERROR("Different speaker configurations");
        return;
      }

    ///////////////////////////////////////////////////////
    // Compare data

    len = MIN(ref_chunk.size, src_chunk.size);
    if (src_spk.is_linear())
    {
      // compare linear format
      for (int ch = 0; ch < src_spk.nch(); ch++)
        for (size_t i = 0; i < len; i++)
          if (!EQUAL_SAMPLES(src_chunk.samples[ch][i], ref_chunk.samples[ch][i]))
          {
            BOOST_ERROR("Data differs at channel = " << ch << " pos = " << pos + i);
            return;
          }

      src_chunk.drop_samples(len);
      ref_chunk.drop_samples(len);
    }
    else
    {
      for (size_t i = 0; i < len; i++)
        if (src_chunk.rawdata[i] != ref_chunk.rawdata[i])
        {
          BOOST_ERROR("Data differs at pos = " << pos + i);
          return;
        }

      src_chunk.drop_rawdata(len);
      ref_chunk.drop_rawdata(len);
    }
    pos += len;
  } // while (true)

  /////////////////////////////////////////////////////////
  // Verify stream lengths

  if (src_chunk.is_empty())
    src->get_chunk(src_chunk);

  if (ref_chunk.is_empty())
    ref->get_chunk(ref_chunk);

  if (src_chunk.size)
    BOOST_ERROR("Output is longer than reference");

  if (ref_chunk.size)
    BOOST_ERROR("Reference is longer than output");
}

void compare(Source *src, Filter *src_filter, Source *ref, Filter *ref_filter)
{
  SourceFilter sf(src, src_filter);
  SourceFilter rf(ref, ref_filter);
  compare(&sf, &rf);
}

///////////////////////////////////////////////////////////////////////////////

void check_streams_chunks(Source *src, int correct_streams, int correct_chunks)
{
  Chunk chunk;
  int streams = 0;
  int chunks = 0;
  while (src->get_chunk(chunk))
  {
    if (src->new_stream())
      streams++;
    chunks++;
  }

  BOOST_CHECK_EQUAL(streams, correct_streams);
  if (correct_chunks >= 0)
    BOOST_CHECK_EQUAL(chunks, correct_chunks); 
}

void check_streams_chunks(Source *src, Filter *f, int streams, int chunks)
{
  SourceFilter sf(src, f);
  check_streams_chunks(&sf, streams, chunks);
}

///////////////////////////////////////////////////////////////////////////////

sample_t calc_peak(Source *source)
{
  assert(source != 0);

  Chunk chunk;
  sample_t peak = 0.0;
  while (source->get_chunk(chunk))
  {
    Speakers spk = source->get_output();
    BOOST_REQUIRE(spk.is_linear());

    for (int ch = 0; ch < spk.nch(); ch++)
      for (size_t i = 0; i < chunk.size; i++)
        if (peak < fabs(chunk.samples[ch][i]))
          peak = fabs(chunk.samples[ch][i]);
  }

  return peak;
}

sample_t calc_peak(Source *source, Filter *filter)
{
  assert(source != 0);
  SourceFilter source_filter(source, filter);
  return calc_peak(&source_filter);
}

///////////////////////////////////////////////////////////////////////////////

double calc_rms(Source *source)
{
  assert(source != 0);

  size_t n = 0;
  double sum = 0.0;

  Chunk chunk;
  while (source->get_chunk(chunk))
  {
    Speakers spk = source->get_output();
    BOOST_REQUIRE(spk.is_linear());

    for (int ch = 0; ch < spk.nch(); ch++)
      for (size_t i = 0; i < chunk.size; i++)
        sum += chunk.samples[ch][i] * chunk.samples[ch][i];

    n += chunk.size * spk.nch();
  }

  return n? sqrt(sum / n): 0.0;
}

double calc_rms(Source *source, Filter *filter)
{
  assert(source != 0);
  SourceFilter source_filter(source, filter);
  return calc_rms(&source_filter);
}

///////////////////////////////////////////////////////////////////////////////

sample_t calc_diff_linear(Source *s1, Source *s2)
{
  assert(s1 != 0 && s2 != 0);

  size_t len;
  sample_t diff = 0.0;

  Chunk chunk1, chunk2;
  while (true)
  {
    if (chunk1.is_empty())
      if (!s1->get_chunk(chunk1))
        break;

    if (chunk2.is_empty())
      if (!s2->get_chunk(chunk2))
        break;

    if (chunk1.is_empty() || chunk2.is_empty())
      continue;

    Speakers spk1 = s1->get_output();
    Speakers spk2 = s2->get_output();
    if (spk1 != spk2 || spk1.is_unknown())
      BOOST_REQUIRE_EQUAL(spk1, spk2);

    len = MIN(chunk1.size, chunk2.size);
    for (int ch = 0; ch < spk1.nch(); ch++)
      for (size_t i = 0; i < len; i++)
        if (diff < fabs(chunk1.samples[ch][i] - chunk2.samples[ch][i]))
          diff = fabs(chunk1.samples[ch][i] - chunk2.samples[ch][i]);

    chunk1.drop_samples(len);
    chunk2.drop_samples(len);
  }

  return diff;
}

sample_t calc_diff(Source *s1, Source *s2)
{
  assert(s1 != 0 && s2 != 0);

  Converter conv1(1024), conv2(1024);
  conv1.set_format(FORMAT_LINEAR);
  conv1.set_order(win_order);
  conv2.set_format(FORMAT_LINEAR);
  conv2.set_order(win_order);

  SourceFilter sf1(s1, &conv1);
  SourceFilter sf2(s2, &conv2);
  return calc_diff_linear(&sf1, &sf2);
}

sample_t calc_diff(Source *s1, Filter *f1, Source *s2, Filter *f2)
{
  assert(s1 != 0 && s2 != 0);
  SourceFilter sf1(s1, f1);
  SourceFilter sf2(s2, f2);
  return calc_diff(&sf1, &sf2);
}

///////////////////////////////////////////////////////////////////////////////

double calc_rms_diff_linear(Source *s1, Source *s2)
{
  assert(s1 != 0 && s2 != 0);

  size_t len;
  size_t n = 0;
  double sum = 0.0;

  Chunk chunk1, chunk2;
  while (true)
  {
    if (chunk1.is_empty())
      if (!s1->get_chunk(chunk1))
        break;

    if (chunk2.is_empty())
      if (!s2->get_chunk(chunk2))
        break;

    if (chunk1.is_empty() || chunk2.is_empty())
      continue;

    Speakers spk1 = s1->get_output();
    Speakers spk2 = s2->get_output();
    if (spk1 != spk2 || spk1.is_unknown())
      BOOST_REQUIRE_EQUAL(spk1, spk2);

    len = MIN(chunk1.size, chunk2.size);
    for (int ch = 0; ch < spk1.nch(); ch++)
      for (size_t i = 0; i < len; i++)
        sum += (chunk1.samples[ch][i] - chunk2.samples[ch][i]) * (chunk1.samples[ch][i] - chunk2.samples[ch][i]);

    n += len * spk1.nch();
    chunk1.drop_samples(len);
    chunk2.drop_samples(len);
  }

  return n? sqrt(sum / n): 0.0;
}

sample_t calc_rms_diff(Source *s1, Source *s2)
{
  assert(s1 != 0 && s2 != 0);

  Converter conv1(1024), conv2(1024);
  conv1.set_format(FORMAT_LINEAR);
  conv1.set_order(win_order);
  conv2.set_format(FORMAT_LINEAR);
  conv2.set_order(win_order);

  SourceFilter sf1(s1, &conv1);
  SourceFilter sf2(s2, &conv2);
  return calc_rms_diff_linear(&sf1, &sf2);
}

double calc_rms_diff(Source *s1, Filter *f1, Source *s2, Filter *f2)
{
  assert(s1 != 0 && s2 != 0);
  SourceFilter sf1(s1, f1);
  SourceFilter sf2(s2, f2);
  return calc_rms_diff(&sf1, &sf2);
}
