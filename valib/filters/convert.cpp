#include <math.h>
#include "convert.h"


#ifdef _M_IX86
inline int32_t s2i32(sample_t s)
{
  // FPU rounding mode?
  // Always check PCM passthrough test!
  register int32_t i;
  __asm fld [s]
  __asm fistp [i]
  return i;
}
inline int16_t s2i16(sample_t s)
{
  // FPU rounding mode?
  // Always check PCM passthrough test!
  register int16_t i;
  __asm fld [s]
  __asm fistp [i]
  return i;
}
#else
inline int32_t s2i32(sample_t s)
{
  return (int32_t)floor(s + 0.5);
}
inline int16_t s2i16(sample_t s)
{
  return (int16_t)floor(s + 0.5);
}
#endif


void
Converter::reset()
{
  chunk.set_empty();
  part_size = 0;
}

bool 
Converter::query_format(int _format) const
{
  return (_format == FORMAT_LINEAR)   ||
         (_format == FORMAT_PCM16)    ||
         (_format == FORMAT_PCM24)    ||
         (_format == FORMAT_PCM32)    ||
         (_format == FORMAT_PCMFLOAT) ||
         (_format == FORMAT_PCM16_LE) ||
         (_format == FORMAT_PCM24_LE) ||
         (_format == FORMAT_PCM32_LE) ||
         (_format == FORMAT_PCMFLOAT_LE);
}

bool 
Converter::linear2pcm(Chunk *out)
{
  int n = MIN(chunk.size, nsamples);
  int nch = chunk.spk.nch();
  int size = ::sample_size(format) * nch;
  samples_t samples = chunk.samples;
  samples.reorder_from_std(spk, order);

  // fill chunk chunk
  Speakers out_spk = chunk.spk;
  out_spk.format = format;
  out->set_spk(out_spk);
  out->set_time(chunk.timestamp, chunk.time);
  out->set_buf(buf.data(), n * size);

  switch (format)
  {
    case FORMAT_PCM16:
    {
      int16_t *buf_ptr = (int16_t *)buf.data();
      for (int s = 0; s < n; s++)
        for (int ch = 0; ch < nch; ch++)
          *buf_ptr++ = s2i16(samples[ch][s]);
      break;
    }
    case FORMAT_PCM24:
    {
      int24_t *buf_ptr = (int24_t *)buf.data();
      for (int s = 0; s < n; s++)
        for (int ch = 0; ch < nch; ch++)
          *buf_ptr++ = s2i32(samples[ch][s]);
      break;
    }
    case FORMAT_PCM32:
    {
      int32_t *buf_ptr = (int32_t *)buf.data();
      for (int s = 0; s < n; s++)
        for (int ch = 0; ch < nch; ch++)
          *buf_ptr++ = s2i32(samples[ch][s]);
      break;
    }
    case FORMAT_PCMFLOAT:
    {
      float *buf_ptr = (float *)buf.data();
      for (int s = 0; s < n; s++)
        for (int ch = 0; ch < nch; ch++)
          *buf_ptr++ = (float)samples[ch][s];
      break;
    }
    case FORMAT_PCM16_LE:
    {
      int16_t *buf_ptr = (int16_t *)buf.data();
      for (int s = 0; s < n; s++)
        for (int ch = 0; ch < nch; ch++)
          *buf_ptr++ = swab16(s2i16(samples[ch][s]));
      break;
    }
    case FORMAT_PCM24_LE:
    {
      int24_t *buf_ptr = (int24_t *)buf.data();
      for (int s = 0; s < n; s++)
        for (int ch = 0; ch < nch; ch++)
          *buf_ptr++ = swab24(s2i32(samples[ch][s]));
      break;
    }
    case FORMAT_PCM32_LE:
    {
      int32_t *buf_ptr = (int32_t *)buf.data();
      for (int s = 0; s < n; s++)
        for (int ch = 0; ch < nch; ch++)
          *buf_ptr++ = swab32(s2i32(samples[ch][s]));
      break;
    }
    case FORMAT_PCMFLOAT_LE:
    {
      int32_t *buf_ptr = (int32_t *)buf.data();
      for (int s = 0; s < n; s++)
        for (int ch = 0; ch < nch; ch++)
          *buf_ptr++ = swab32(*(uint32_t *)&samples[ch][s]);
      break;
    }

    default:
      return false;
  }

  chunk.drop(n);
  return true;
}

bool 
Converter::pcm2linear(Chunk *out)
{
  int ch, s;
  int nch = spk.nch();                 // number of channels
  int size = spk.sample_size() * nch;  // full sample size

  // fill chunk
  Speakers out_spk = chunk.spk;
  out_spk.format = FORMAT_LINEAR;
  out->set_spk(out_spk);
  out->set_time(chunk.timestamp, chunk.time);

  // process partial sample
  if (part_size && part_size < size)
  {
    int l = MIN(size - part_size, chunk.size);
    memcpy(part_buf + part_size, chunk.buf, l);
    part_size += l;
    chunk.drop(l);
  }

  // number of samples
  int n = chunk.size / size;
  if (part_size == size) n++;
  if (n > nsamples) 
    n = nsamples;

  out->set_samples(samples, n);
  out->samples.reorder_to_std(spk, order);

  // convert
  switch (spk.format)
  {
    case FORMAT_PCM16:
    {
      s = 0; 
      int16_t *buf;
      if (part_size == size)
      {
        buf = (int16_t *)part_buf;
        for (ch = 0; ch < nch; ch++)
          samples[ch][0] = *buf++;
        s++;
      }
      buf = (int16_t *)chunk.buf;
      for (; s < n; s++)
        for (ch = 0; ch < nch; ch++)
          samples[ch][s] = *buf++;
      break;
    }
    case FORMAT_PCM24:
    {
      s = 0; 
      int24_t *buf;
      if (part_size == size)
      {
        buf = (int24_t *)part_buf;
        for (ch = 0; ch < nch; ch++)
          samples[ch][0] = *buf++;
        s++;
      }
      buf = (int24_t *)chunk.buf;
      for (; s < n; s++)
        for (ch = 0; ch < nch; ch++)
          samples[ch][s] = *buf++;
      break;
    }
    case FORMAT_PCM32:
    {
      s = 0; 
      int32_t *buf;
      if (part_size == size)
      {
        buf = (int32_t *)part_buf;
        for (ch = 0; ch < nch; ch++)
          samples[ch][0] = *buf++;
        s++;
      }
      buf = (int32_t *)chunk.buf;
      for (; s < n; s++)
        for (ch = 0; ch < nch; ch++)
          samples[ch][s] = *buf++;
      break;
    }
    case FORMAT_PCMFLOAT:
    {
      s = 0; 
      float *buf;
      if (part_size == size)
      {
        buf = (float *)part_buf;
        for (ch = 0; ch < nch; ch++)
          samples[ch][0] = *buf++;
        s++;
      }
      buf = (float *)chunk.buf;
      for (; s < n; s++)
        for (ch = 0; ch < nch; ch++)
          samples[ch][s] = *buf++;
      break;
    }
    case FORMAT_PCM16_LE:
    {
      s = 0; 
      int16_t *buf;
      if (part_size == size)
      {
        buf = (int16_t *)part_buf;
        for (ch = 0; ch < nch; ch++)
          samples[ch][0] = (int16_t)swab16(*buf++);
        s++;
      }
      buf = (int16_t *)chunk.buf;
      for (; s < n; s++)
        for (ch = 0; ch < nch; ch++)
          samples[ch][s] = (int16_t)swab16(*buf++);
      break;
    }
    case FORMAT_PCM24_LE:
    {
      s = 0; 
      int24_t *buf;
      if (part_size == size)
      {
        buf = (int24_t *)part_buf;
        for (ch = 0; ch < nch; ch++)
          samples[ch][0] = (int24_t)swab24(*buf++);
        s++;
      }
      buf = (int24_t *)chunk.buf;
      for (; s < n; s++)
        for (ch = 0; ch < nch; ch++)
          samples[ch][s] = (int24_t)swab24(*buf++);
      break;
    }
    case FORMAT_PCM32_LE:
    {
      s = 0; 
      int32_t *buf;
      if (part_size == size)
      {
        buf = (int32_t *)part_buf;
        for (ch = 0; ch < nch; ch++)
          samples[ch][0] = (int32_t)swab32(*buf++);
        s++;
      }
      buf = (int32_t *)chunk.buf;
      for (; s < n; s++)
        for (ch = 0; ch < nch; ch++)
          samples[ch][s] = (int32_t)swab32(*buf++);
      break;
    }
    case FORMAT_PCMFLOAT_LE:
    {
      s = 0; 
      int32_t *buf;
      int32_t f;
      if (part_size == size)
      {
        buf = (int32_t *)part_buf;
        for (ch = 0; ch < nch; ch++)
        {
          f = (int32_t)swab32(*buf++);
          samples[ch][0] = *(float *)&f;
        }
        s++;
      }
      buf = (int32_t *)chunk.buf;
      for (; s < n; s++)
        for (ch = 0; ch < nch; ch++)
        {
          f = (int32_t)swab32(*buf++);
          samples[ch][s] = *(float *)&f;
        }
      break;
    }

    default:
      return false;
  }

  // process partial sample
  if (part_size == size)
  {
    chunk.drop(n * size - size);
    part_size = 0;
  }
  else
    chunk.drop(n * size);

  if (chunk.size && chunk.size < size)
  {
    memcpy(part_buf, chunk.buf, chunk.size);
    part_size = chunk.size;
    chunk.drop(chunk.size);
  }

  return true;
}

bool 
Converter::pcm2pcm(Chunk *out)
{
  return false;
}


///////////////////////////////////////////////////////////
// Filter interface

bool 
Converter::query_input(Speakers _spk) const
{
  return query_format(_spk.format);
}

bool 
Converter::set_input(Speakers _spk)
{
  if (!NullFilter::set_input(_spk))
    return false;

  buf.allocate(spk.nch() * nsamples * ::sample_size(format));
  samples[0] = (sample_t *)buf.data();
  for (int ch = 1; ch < spk.nch(); ch++)
    samples[ch] = samples[ch-1] + nsamples;

  return true;
}

Speakers 
Converter::get_output()
{
  Speakers out = spk;
  out.format = format;
  return out;
}

bool 
Converter::get_chunk(Chunk *out)
{
  if (chunk.is_empty())
  {
    out->set_empty();
    return true;
  }

  if (spk.format == format)
  {
    *out = chunk;
    chunk.set_empty();
    return true;
  }

  if (format == FORMAT_LINEAR)
    return pcm2linear(out);

  if (chunk.spk.format == FORMAT_LINEAR)
    return linear2pcm(out);

  // note: never be here
  return false;
}

