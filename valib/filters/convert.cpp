#include <math.h>
#include "convert.h"

// todo: separate conversion functions for each conversion mode (8*8

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

  if (!buf.allocate(spk.nch() * nsamples * sample_size(format)))
    return false;

  if (format == FORMAT_LINEAR)
  {
    out_samples[0] = (sample_t *)buf.data();
    for (int ch = 1; ch < spk.nch(); ch++)
      out_samples[ch] = out_samples[ch-1] + nsamples;
    out_rawdata = 0;
  }
  else
  {
    out_rawdata = buf.data();
    out_samples.set_null();
  }
  return true;
}

void
Converter::pcm2linear()
{
  if (spk.format == FORMAT_PCM16)
  {
    pcm16_linear();
    return;
  }

  int nch = spk.nch();
  size_t sample_size = spk.sample_size() * nch;

  out_size = 0;

  // load partial sample
  if (part_size)
  {
    if (sample_size - part_size > size)
    {
      memcpy(part_buf + part_size, rawdata, size);
      drop_rawdata(size);
      part_size += size;
      return;
    }
    else
    {
      memcpy(part_buf + part_size, rawdata, sample_size - part_size);
      pcm2linear(part_buf, out_samples, 1);
      drop_rawdata(sample_size - part_size);
      out_size++;
      time -= 1;
      part_size = 0;
    }
  }

  // convert buffer
  size_t n = size / sample_size;
  if (n + out_size > nsamples)
    n = nsamples - out_size;

  samples_t tmp_samples = out_samples;
  tmp_samples += out_size;
  pcm2linear(rawdata, tmp_samples, n);
  drop_rawdata(n * sample_size);
  out_size += n;

  // load partial sample
  if (size)
  {
    // assert: size < max_sample_size
    memcpy(part_buf + part_size, rawdata, size);
    drop_rawdata(size);
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
          dst[ch][s] = *ptr++;
      break;
    }

    case FORMAT_PCM16_LE:
    {
      int16_t *ptr = (int16_t *)src;
      for (s = 0; s < n; s++)
        for (ch = 0; ch < nch; ch++)
          dst[ch][s] = (int16_t)swab16(*ptr++);
      break;
    }

    case FORMAT_PCM24:
    {
      int24_t *ptr = (int24_t *)src;
      for (s = 0; s < n; s++)
        for (ch = 0; ch < nch; ch++)
          dst[ch][s] = *ptr++;
      break;
    }

    case FORMAT_PCM24_LE:
    {
      int24_t *ptr = (int24_t *)src;
      for (s = 0; s < n; s++)
        for (ch = 0; ch < nch; ch++)
          dst[ch][s] = swab24(*ptr++);
      break;
    }

    case FORMAT_PCM32:
    {
      int32_t *ptr = (int32_t *)src;
      for (s = 0; s < n; s++)
        for (ch = 0; ch < nch; ch++)
          dst[ch][s] = *ptr++;
      break;
    }

    case FORMAT_PCM32_LE:
    {
      int32_t *ptr = (int32_t *)src;
      for ( s = 0; s < n; s++)
        for (ch = 0; ch < nch; ch++)
          dst[ch][s] = (int32_t)swab32(*ptr++);
      break;
    }

    case FORMAT_PCMFLOAT:
    {
      float *ptr = (float *)src;
      for (s = 0; s < n; s++)
        for (ch = 0; ch < nch; ch++)
          dst[ch][s] = *ptr++;
      break;
    }

    case FORMAT_PCMFLOAT_LE:
    {
      uint32_t *ptr = (uint32_t *)src;
      for (s = 0; s < n; s++)
        for (ch = 0; ch < nch; ch++)
        {
          uint32_t i = swab32(*ptr++);
          dst[ch][s] = *(float*)(&i);
        }
      break;
    }
  }
}

void 
Converter::linear2pcm()
{
  int nch = spk.nch();
  size_t sample_size = ::sample_size(format) * nch;

  size_t n = MIN(size, nsamples);
  linear2pcm(samples, out_rawdata, n);
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

  if (format == FORMAT_LINEAR)
    _chunk->set
    (
      get_output(), 
      out_samples, out_size,
      sync, time, 
      flushing && !size
    );
  else
    _chunk->set
    (
      get_output(), 
      out_rawdata, out_size,
      sync, time, 
      flushing && !size
    );

  flushing = flushing && size;
  sync = false;

  return true;
}



// todo: generate function versions for each number of channels
void
Converter::pcm16_linear()
{
  // input: spk, nsamples, rawdata, size
  // output: out_samples, out_size
  // intermediate: part_buf, part_size

  int ch;
  int nch = spk.nch();                          // will be const
  size_t sample_size = sizeof(int16_t) * nch;   // will be const

  int16_t  *src = (int16_t *)rawdata;
  sample_t *dst = out_samples[0];
  out_size = 0;

  /////////////////////////////////////////////////////////
  // Process part of sample

  if (part_size)
  {
    // assert: part_size < sample_size
    size_t delta = sample_size - part_size;
    if (size < delta)
    {
      // not enough data to fill sample buffer
      memcpy(part_buf + part_size, src, size);
      part_size += size;
      size = 0;
      return;
    }
    else
    {
      // finish & convert incomplete sample 
      memcpy(part_buf + part_size, src, delta);
      drop_rawdata(delta);
      part_size = 0;

      for (ch = 0; ch < nch; ch++)
        dst[nsamples * ch] = src[ch];
      src += nch;
      dst++;

      out_size++;
    }   
  }

  /////////////////////////////////////////////////////////
  // Find end of buffer & remember remaining part of sample

  int16_t *end;
  if ((nsamples - out_size) * sample_size < size)
  {
    drop_rawdata((nsamples - out_size) * sample_size);
    end = src + nsamples - out_size;
    out_size += nsamples - out_size;
  }
  else
  {
    out_size += size / sample_size;
    end = src + size / sample_size;
    drop_rawdata((size / sample_size) * sample_size);

    // remember part of smaple
    if (size)
    {
      memcpy(part_buf, end, size);
      part_size = size;
      size = 0;
    }
  }

  /////////////////////////////////////////////////////////
  // Convert

  while (src < end)
  {
    for (ch = 0; ch < nch; ch++)
      dst[nsamples * ch] = src[ch];
    src += nch;
    dst++;
  }

}
