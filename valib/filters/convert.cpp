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

bool 
Converter::alloc_buffer()
{
  if (spk.format == format)
    return true; // no buffer required

  if (!buffer.allocate(spk.nch() * nsamples * sample_size(format)))
    return false;

  if (format == FORMAT_LINEAR)
  {
    out_samples[0] = (sample_t *)buffer.data();
    for (int ch = 1; ch < spk.nch(); ch++)
      out_samples[ch] = out_samples[ch-1] + nsamples;
    out_buf = 0;
  }
  else
  {
    out_buf = buffer.data();
    out_samples.set_null();
  }
}

void
Converter::pcm2linear()
{
  int nch = spk.nch();
  size_t sample_size = spk.sample_size() * nch;

  out_size = 0;

  // load partial sample
  if (part_size)
  {
    if (sample_size - part_size > size)
    {
      memcpy(part_buf + part_size, buf, size);
      drop_buf(size);
      part_size += size;
      return;
    }
    else
    {
      memcpy(part_buf + part_size, buf, sample_size - part_size);
      pcm2linear(part_buf, out_samples, 1);
      drop_buf(sample_size - part_size);
      out_size++;
      time -= 1;
      part_size = 0;
    }
  }

  // convert buffer
  size_t n = size / sample_size;
  if (n + out_size > nsamples)
    n = nsamples - out_size;

  pcm2linear(buf, out_samples + out_size, n);
  drop_buf(n * sample_size);
  out_size += n;

  // load partial sample
  if (size)
  {
    // assert: size < max_sample_size
    memcpy(part_buf, buf, size);
    drop(size);
  }
}

void
Converter::pcm2linear(uint8_t *src, samples_t dst, size_t n)
{
  int ch;
  int nch = spk.nch();
  size_t s;

  switch (spk.format)
  {
    case FORMAT_PCM16:
    {
      int16_t *ptr = (int16_t *)src;
      for (s = 0; s < n; s++)
        for (ch = 0; ch < nch; ch++)
          samples[ch][s] = *ptr++;
      break;
    }

    case FORMAT_PCM16_LE:
    {
      int16_t *ptr = (int16_t *)src;
      for (s = 0; s < n; s++)
        for (ch = 0; ch < nch; ch++)
          samples[ch][s] = (int16_t)swab16(*ptr++);
      break;
    }

    case FORMAT_PCM24:
    {
      int24_t *ptr = (int24_t *)src;
      for (s = 0; s < n; s++)
        for (ch = 0; ch < nch; ch++)
          samples[ch][s] = *ptr++;
      break;
    }

    case FORMAT_PCM24_LE:
    {
      int24_t *ptr = (int24_t *)src;
      for (s = 0; s < n; s++)
        for (ch = 0; ch < nch; ch++)
          samples[ch][s] = swab24(*ptr++);
      break;
    }

    case FORMAT_PCM32:
    {
      int32_t *ptr = (int32_t *)src;
      for (s = 0; s < n; s++)
        for (ch = 0; ch < nch; ch++)
          samples[ch][s] = *ptr++;
      break;
    }

    case FORMAT_PCM32_LE:
    {
      int32_t *ptr = (int32_t *)src;
      for ( s = 0; s < n; s++)
        for (ch = 0; ch < nch; ch++)
          samples[ch][s] = (int32_t)swab32(*ptr++);
      break;
    }

    case FORMAT_PCMFLOAT:
    {
      float *ptr = (float *)src;
      for (s = 0; s < n; s++)
        for (ch = 0; ch < nch; ch++)
          samples[ch][s] = *ptr++;
      break;
    }

    case FORMAT_PCMFLOAT_LE:
    {
      uint32_t *ptr = (uint32_t *)src;
      for (s = 0; s < n; s++)
        for (ch = 0; ch < nch; ch++)
        {
          uint32_t i = swab32(*ptr++);
          samples[ch][s] = *(float*)(&i);
        }
      break;
    }
  }
}

void 
Converter::linear2pcm()
{
  int nch = spk.nch();
  size_t sample_size = spk.sample_size() * nch;

  size_t n = MIN(size, nsamples);
  linear2pcm(samples, out_buf, n);
  drop_samples(n);
  out_size = n * sample_size;
}

void
Converter::linear2pcm(samples_t src, uint8_t *dst, size_t n)
{
  int ch;
  int nch = spk.nch();
  size_t s;

  switch (format)
  {
    case FORMAT_PCM16:
    {
      int16_t *ptr = (int16_t *)dst;
      for (s = 0; s < n; s++)
        for (ch = 0; ch < nch; ch++)
          *ptr++ = s2i16(samples[ch][s]);
      break;
    }

    case FORMAT_PCM16_LE:
    {
      int16_t *ptr = (int16_t *)dst;
      for (s = 0; s < n; s++)
        for (ch = 0; ch < nch; ch++)
          *ptr++ = swab16(s2i16(samples[ch][s]));
      break;
    }

    case FORMAT_PCM24:
    {
      int24_t *ptr = (int24_t *)dst;
      for (s = 0; s < n; s++)
        for (ch = 0; ch < nch; ch++)
          *ptr++ = s2i32(samples[ch][s]);
      break;
    }

    case FORMAT_PCM24_LE:
    {
      int24_t *ptr = (int24_t *)dst;
      for (s = 0; s < n; s++)
        for (ch = 0; ch < nch; ch++)
          *ptr++ = swab24(s2i32(samples[ch][s]));
      break;
    }

    case FORMAT_PCM32:
    {
      int32_t *ptr = (int32_t *)dst;
      for (s = 0; s < n; s++)
        for (ch = 0; ch < nch; ch++)
          *ptr++ = s2i32(samples[ch][s]);
      break;
    }

    case FORMAT_PCM32_LE:
    {
      int32_t *ptr = (int32_t *)dst;
      for (s = 0; s < n; s++)
        for (ch = 0; ch < nch; ch++)
          *ptr++ = swab32(s2i32(samples[ch][s]));
      break;
    }

    case FORMAT_PCMFLOAT:
    {
      float *ptr = (float *)dst;
      for (s = 0; s < n; s++)
        for (ch = 0; ch < nch; ch++)
          *ptr++ = (float)samples[ch][s];
      break;
    }

    case FORMAT_PCMFLOAT_LE:
    {
      int32_t *ptr = (int32_t *)dst;
      for (s = 0; s < n; s++)
        for (ch = 0; ch < nch; ch++)
          *ptr++ = swab32(*(uint32_t *)&samples[ch][s]);
      break;
    }
  }
}

///////////////////////////////////////////////////////////
// Filter interface

void
Converter::reset()
{
  NullFilter::reset();
  part_size = 0;
}

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

  return alloc_buffer();
}

Speakers 
Converter::get_output() const
{
  Speakers out = spk;
  out.format = format;
  return out;
}

bool 
Converter::get_chunk(Chunk *_chunk)
{
  if (spk.format == format)
  {
    send_chunk_inplace(_chunk, size);
    return true;
  }

  if (format == FORMAT_LINEAR)
    pcm2linear();
  else if (spk.format == FORMAT_LINEAR)
    linear2pcm();
  else
  {
    // todo: pcm->pcm
    return false;
  }

  _chunk->set_spk(get_output());
  _chunk->set_sync(sync, time);
  _chunk->set_eos(flushing && !size);

  if (format == FORMAT_LINEAR)
    _chunk->set_samples(out_samples, out_size);
  else
    _chunk->set_buf(out_buf, out_size);

  flushing = flushing && size;
  sync = false;

  return true;
}
