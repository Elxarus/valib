#include <math.h>
#include "generator.h"

///////////////////////////////////////////////////////////////////////////////
// Generator
// Base class for signal generation

bool 
Generator::init(Speakers spk_, uint64_t stream_len_, size_t chunk_size_)
{
  stream_len = 0;
  chunk_size = 0;

  if (!chunk_size_ || !query_spk(spk_))
    return false;

  if (spk_.format == FORMAT_LINEAR)
  {
    rawdata.free();
    if (!samples.allocate(spk_.nch(), chunk_size_))
      return false;
  }
  else
  {
    samples.free();
    if (!rawdata.allocate(chunk_size_))
      return false;
  }

  spk = spk_;
  chunk_size = chunk_size_;
  stream_len = stream_len_;
  return true;
}

bool 
Generator::get_chunk(Chunk2 &chunk)
{
  if (!stream_len)
    return false;

  size_t n = chunk_size;
  if (n >= stream_len)
    n = (size_t)stream_len;
  stream_len -= n;

  if (spk.format == FORMAT_LINEAR)
  {
    gen_samples(samples, n);
    chunk.set_linear(samples, n);
    return true;
  }
  else
  {
    gen_rawdata(rawdata, n);
    chunk.set_rawdata(rawdata, n);
    return true;
  }
}

///////////////////////////////////////////////////////////////////////////////
// ToneGen
// Tone generator

void
ToneGen::gen_samples(samples_t samples, size_t n)
{
  double w = 2 * M_PI * double(freq) / double(spk.sample_rate);
  for (size_t i = 0; i < n; i++)
    samples[0][i] = sin(phase + i*w);
  phase += n*w;

  for (int ch = 1; ch < spk.nch(); ch++)
    copy_samples(samples[ch], samples[0], n);
}

///////////////////////////////////////////////////////////////////////////////
// LineGen
// Line generator

void
LineGen::gen_samples(samples_t samples, size_t n)
{
  for (size_t i = 0; i < n; i++)
    samples[0][i] = phase + i*k;
  phase += n*k;

  for (int ch = 1; ch < spk.nch(); ch++)
    copy_samples(samples[ch], samples[0], n);
}
