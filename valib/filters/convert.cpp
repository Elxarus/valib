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


typedef void (Converter::*convert_t)();

static const int formats_tbl[] = {FORMAT_PCM16, FORMAT_PCM24, FORMAT_PCM32, FORMAT_PCMFLOAT, FORMAT_PCM16_LE, FORMAT_PCM24_LE, FORMAT_PCM32_LE, FORMAT_PCMFLOAT_LE };

const int formats = FORMAT_PCM16 | FORMAT_PCM24 | FORMAT_PCM32 | FORMAT_PCMFLOAT | FORMAT_PCM16_LE | FORMAT_PCM24_LE | FORMAT_PCM32_LE | FORMAT_PCMFLOAT_LE;

static const convert_t pcm2linear_tbl[NCHANNELS][8] = {
 { Converter::pcm16_linear_1ch, Converter::pcm24_linear_1ch, Converter::pcm32_linear_1ch, Converter::pcmfloat_linear_1ch, Converter::pcm16_le_linear_1ch, Converter::pcm24_le_linear_1ch, Converter::pcm32_le_linear_1ch, Converter::pcmfloat_le_linear_1ch },
 { Converter::pcm16_linear_2ch, Converter::pcm24_linear_2ch, Converter::pcm32_linear_2ch, Converter::pcmfloat_linear_2ch, Converter::pcm16_le_linear_2ch, Converter::pcm24_le_linear_2ch, Converter::pcm32_le_linear_2ch, Converter::pcmfloat_le_linear_2ch },
 { Converter::pcm16_linear_3ch, Converter::pcm24_linear_3ch, Converter::pcm32_linear_3ch, Converter::pcmfloat_linear_3ch, Converter::pcm16_le_linear_3ch, Converter::pcm24_le_linear_3ch, Converter::pcm32_le_linear_3ch, Converter::pcmfloat_le_linear_3ch },
 { Converter::pcm16_linear_4ch, Converter::pcm24_linear_4ch, Converter::pcm32_linear_4ch, Converter::pcmfloat_linear_4ch, Converter::pcm16_le_linear_4ch, Converter::pcm24_le_linear_4ch, Converter::pcm32_le_linear_4ch, Converter::pcmfloat_le_linear_4ch },
 { Converter::pcm16_linear_5ch, Converter::pcm24_linear_5ch, Converter::pcm32_linear_5ch, Converter::pcmfloat_linear_5ch, Converter::pcm16_le_linear_5ch, Converter::pcm24_le_linear_5ch, Converter::pcm32_le_linear_5ch, Converter::pcmfloat_le_linear_5ch },
 { Converter::pcm16_linear_6ch, Converter::pcm24_linear_6ch, Converter::pcm32_linear_6ch, Converter::pcmfloat_linear_6ch, Converter::pcm16_le_linear_6ch, Converter::pcm24_le_linear_6ch, Converter::pcm32_le_linear_6ch, Converter::pcmfloat_le_linear_6ch },
};




bool 
Converter::alloc_buffer()
{
  if (spk.format == format)
    return true; // no buffer required

  if (!buf.allocate(spk.nch() * nsamples * sample_size(format)))
    return false;

  if (format == FORMAT_LINEAR)
  {
    // set pointers
    out_samples[0] = (sample_t *)buf.get_data();
    for (int ch = 1; ch < spk.nch(); ch++)
      out_samples[ch] = out_samples[ch-1] + nsamples;
    out_rawdata = 0;

    // find conversion function
    convert = 0;
    for (int i = 0; i < array_size(formats_tbl); i++)
      if (spk.format == formats_tbl[i])
        convert = pcm2linear_tbl[spk.nch()-1][i];

    return convert != 0;
  }
  else
  {
    // set rawdata pointer
    out_rawdata = buf.get_data();
    out_samples.set_null();

    // todo:
/*
    // find conversion function
    convert = 0;
    for (int i = 0; i < array_size(formats_tbl); i++)
      if (spk.format == formats_tbl[i])
        convert = pcm2linear_tbl[spk.nch()][i];

    return convert != 0;
*/
  }
  return true;
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
    (this->*convert)();
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





void
Converter::pcm16_linear_1ch()
{
  // input: spk, nsamples, rawdata, size
  // output: out_samples, out_size
  // intermediate: part_buf, part_size

  const int nch = 1;
  const size_t sample_size = sizeof(int16_t) * 1;

  int16_t *src;
  int16_t *end;
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
      memcpy(part_buf + part_size, rawdata, size);
      part_size += size;
      size = 0;
      return;
    }
    else
    {
      // finish & convert incomplete sample 
      memcpy(part_buf + part_size, rawdata, delta);
      drop_rawdata(delta);
      part_size = 0;

      src = (int16_t *)part_buf;
    dst[nsamples * 0] = sample_t(src[0]);

      dst++;
      out_size++;
    }   
  }

  /////////////////////////////////////////////////////////
  // Set processing buffer start & end and remember 
  // remaining part of sample

  src = (int16_t *)rawdata;

  // integral number of samples (and its size in bytes)
  size_t n = nsamples - out_size;
  size_t n_size = n * sample_size;

  if (n_size < size)
  {
    drop_rawdata(n_size);
    end = (int16_t *)(((uint8_t *)src) + n_size);
    out_size += n;
  }
  else
  {
    n = size / sample_size;
    n_size = n * sample_size;

    drop_rawdata(n_size);
    end = (int16_t *)(((uint8_t *)src) + n_size);
    out_size += n;

    // remember part of sample
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
    dst[nsamples * 0] = sample_t(src[0]);

    src += nch;
    dst++;
  }
}
void
Converter::pcm24_linear_1ch()
{
  // input: spk, nsamples, rawdata, size
  // output: out_samples, out_size
  // intermediate: part_buf, part_size

  const int nch = 1;
  const size_t sample_size = sizeof(int24_t) * 1;

  int24_t *src;
  int24_t *end;
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
      memcpy(part_buf + part_size, rawdata, size);
      part_size += size;
      size = 0;
      return;
    }
    else
    {
      // finish & convert incomplete sample 
      memcpy(part_buf + part_size, rawdata, delta);
      drop_rawdata(delta);
      part_size = 0;

      src = (int24_t *)part_buf;
    dst[nsamples * 0] = sample_t(src[0]);

      dst++;
      out_size++;
    }   
  }

  /////////////////////////////////////////////////////////
  // Set processing buffer start & end and remember 
  // remaining part of sample

  src = (int24_t *)rawdata;

  // integral number of samples (and its size in bytes)
  size_t n = nsamples - out_size;
  size_t n_size = n * sample_size;

  if (n_size < size)
  {
    drop_rawdata(n_size);
    end = (int24_t *)(((uint8_t *)src) + n_size);
    out_size += n;
  }
  else
  {
    n = size / sample_size;
    n_size = n * sample_size;

    drop_rawdata(n_size);
    end = (int24_t *)(((uint8_t *)src) + n_size);
    out_size += n;

    // remember part of sample
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
    dst[nsamples * 0] = sample_t(src[0]);

    src += nch;
    dst++;
  }
}
void
Converter::pcm32_linear_1ch()
{
  // input: spk, nsamples, rawdata, size
  // output: out_samples, out_size
  // intermediate: part_buf, part_size

  const int nch = 1;
  const size_t sample_size = sizeof(int32_t) * 1;

  int32_t *src;
  int32_t *end;
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
      memcpy(part_buf + part_size, rawdata, size);
      part_size += size;
      size = 0;
      return;
    }
    else
    {
      // finish & convert incomplete sample 
      memcpy(part_buf + part_size, rawdata, delta);
      drop_rawdata(delta);
      part_size = 0;

      src = (int32_t *)part_buf;
    dst[nsamples * 0] = sample_t(src[0]);

      dst++;
      out_size++;
    }   
  }

  /////////////////////////////////////////////////////////
  // Set processing buffer start & end and remember 
  // remaining part of sample

  src = (int32_t *)rawdata;

  // integral number of samples (and its size in bytes)
  size_t n = nsamples - out_size;
  size_t n_size = n * sample_size;

  if (n_size < size)
  {
    drop_rawdata(n_size);
    end = (int32_t *)(((uint8_t *)src) + n_size);
    out_size += n;
  }
  else
  {
    n = size / sample_size;
    n_size = n * sample_size;

    drop_rawdata(n_size);
    end = (int32_t *)(((uint8_t *)src) + n_size);
    out_size += n;

    // remember part of sample
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
    dst[nsamples * 0] = sample_t(src[0]);

    src += nch;
    dst++;
  }
}
void
Converter::pcmfloat_linear_1ch()
{
  // input: spk, nsamples, rawdata, size
  // output: out_samples, out_size
  // intermediate: part_buf, part_size

  const int nch = 1;
  const size_t sample_size = sizeof(float) * 1;

  float *src;
  float *end;
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
      memcpy(part_buf + part_size, rawdata, size);
      part_size += size;
      size = 0;
      return;
    }
    else
    {
      // finish & convert incomplete sample 
      memcpy(part_buf + part_size, rawdata, delta);
      drop_rawdata(delta);
      part_size = 0;

      src = (float *)part_buf;
    dst[nsamples * 0] = sample_t(src[0]);

      dst++;
      out_size++;
    }   
  }

  /////////////////////////////////////////////////////////
  // Set processing buffer start & end and remember 
  // remaining part of sample

  src = (float *)rawdata;

  // integral number of samples (and its size in bytes)
  size_t n = nsamples - out_size;
  size_t n_size = n * sample_size;

  if (n_size < size)
  {
    drop_rawdata(n_size);
    end = (float *)(((uint8_t *)src) + n_size);
    out_size += n;
  }
  else
  {
    n = size / sample_size;
    n_size = n * sample_size;

    drop_rawdata(n_size);
    end = (float *)(((uint8_t *)src) + n_size);
    out_size += n;

    // remember part of sample
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
    dst[nsamples * 0] = sample_t(src[0]);

    src += nch;
    dst++;
  }
}
void
Converter::pcm16_le_linear_1ch()
{
  // input: spk, nsamples, rawdata, size
  // output: out_samples, out_size
  // intermediate: part_buf, part_size

  const int nch = 1;
  const size_t sample_size = sizeof(int16_t) * 1;

  int16_t *src;
  int16_t *end;
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
      memcpy(part_buf + part_size, rawdata, size);
      part_size += size;
      size = 0;
      return;
    }
    else
    {
      // finish & convert incomplete sample 
      memcpy(part_buf + part_size, rawdata, delta);
      drop_rawdata(delta);
      part_size = 0;

      src = (int16_t *)part_buf;
    dst[nsamples * 0] = swab16(src[0]);

      dst++;
      out_size++;
    }   
  }

  /////////////////////////////////////////////////////////
  // Set processing buffer start & end and remember 
  // remaining part of sample

  src = (int16_t *)rawdata;

  // integral number of samples (and its size in bytes)
  size_t n = nsamples - out_size;
  size_t n_size = n * sample_size;

  if (n_size < size)
  {
    drop_rawdata(n_size);
    end = (int16_t *)(((uint8_t *)src) + n_size);
    out_size += n;
  }
  else
  {
    n = size / sample_size;
    n_size = n * sample_size;

    drop_rawdata(n_size);
    end = (int16_t *)(((uint8_t *)src) + n_size);
    out_size += n;

    // remember part of sample
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
    dst[nsamples * 0] = swab16(src[0]);

    src += nch;
    dst++;
  }
}
void
Converter::pcm24_le_linear_1ch()
{
  // input: spk, nsamples, rawdata, size
  // output: out_samples, out_size
  // intermediate: part_buf, part_size

  const int nch = 1;
  const size_t sample_size = sizeof(int24_t) * 1;

  int24_t *src;
  int24_t *end;
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
      memcpy(part_buf + part_size, rawdata, size);
      part_size += size;
      size = 0;
      return;
    }
    else
    {
      // finish & convert incomplete sample 
      memcpy(part_buf + part_size, rawdata, delta);
      drop_rawdata(delta);
      part_size = 0;

      src = (int24_t *)part_buf;
    dst[nsamples * 0] = swab24(src[0]);

      dst++;
      out_size++;
    }   
  }

  /////////////////////////////////////////////////////////
  // Set processing buffer start & end and remember 
  // remaining part of sample

  src = (int24_t *)rawdata;

  // integral number of samples (and its size in bytes)
  size_t n = nsamples - out_size;
  size_t n_size = n * sample_size;

  if (n_size < size)
  {
    drop_rawdata(n_size);
    end = (int24_t *)(((uint8_t *)src) + n_size);
    out_size += n;
  }
  else
  {
    n = size / sample_size;
    n_size = n * sample_size;

    drop_rawdata(n_size);
    end = (int24_t *)(((uint8_t *)src) + n_size);
    out_size += n;

    // remember part of sample
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
    dst[nsamples * 0] = swab24(src[0]);

    src += nch;
    dst++;
  }
}
void
Converter::pcm32_le_linear_1ch()
{
  // input: spk, nsamples, rawdata, size
  // output: out_samples, out_size
  // intermediate: part_buf, part_size

  const int nch = 1;
  const size_t sample_size = sizeof(int32_t) * 1;

  int32_t *src;
  int32_t *end;
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
      memcpy(part_buf + part_size, rawdata, size);
      part_size += size;
      size = 0;
      return;
    }
    else
    {
      // finish & convert incomplete sample 
      memcpy(part_buf + part_size, rawdata, delta);
      drop_rawdata(delta);
      part_size = 0;

      src = (int32_t *)part_buf;
    dst[nsamples * 0] = swab32(src[0]);

      dst++;
      out_size++;
    }   
  }

  /////////////////////////////////////////////////////////
  // Set processing buffer start & end and remember 
  // remaining part of sample

  src = (int32_t *)rawdata;

  // integral number of samples (and its size in bytes)
  size_t n = nsamples - out_size;
  size_t n_size = n * sample_size;

  if (n_size < size)
  {
    drop_rawdata(n_size);
    end = (int32_t *)(((uint8_t *)src) + n_size);
    out_size += n;
  }
  else
  {
    n = size / sample_size;
    n_size = n * sample_size;

    drop_rawdata(n_size);
    end = (int32_t *)(((uint8_t *)src) + n_size);
    out_size += n;

    // remember part of sample
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
    dst[nsamples * 0] = swab32(src[0]);

    src += nch;
    dst++;
  }
}
void
Converter::pcmfloat_le_linear_1ch()
{
  // input: spk, nsamples, rawdata, size
  // output: out_samples, out_size
  // intermediate: part_buf, part_size

  const int nch = 1;
  const size_t sample_size = sizeof(float) * 1;

  float *src;
  float *end;
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
      memcpy(part_buf + part_size, rawdata, size);
      part_size += size;
      size = 0;
      return;
    }
    else
    {
      // finish & convert incomplete sample 
      memcpy(part_buf + part_size, rawdata, delta);
      drop_rawdata(delta);
      part_size = 0;

      src = (float *)part_buf;
    dst[nsamples * 0] = swab_float(src[0]);

      dst++;
      out_size++;
    }   
  }

  /////////////////////////////////////////////////////////
  // Set processing buffer start & end and remember 
  // remaining part of sample

  src = (float *)rawdata;

  // integral number of samples (and its size in bytes)
  size_t n = nsamples - out_size;
  size_t n_size = n * sample_size;

  if (n_size < size)
  {
    drop_rawdata(n_size);
    end = (float *)(((uint8_t *)src) + n_size);
    out_size += n;
  }
  else
  {
    n = size / sample_size;
    n_size = n * sample_size;

    drop_rawdata(n_size);
    end = (float *)(((uint8_t *)src) + n_size);
    out_size += n;

    // remember part of sample
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
    dst[nsamples * 0] = swab_float(src[0]);

    src += nch;
    dst++;
  }
}

void
Converter::pcm16_linear_2ch()
{
  // input: spk, nsamples, rawdata, size
  // output: out_samples, out_size
  // intermediate: part_buf, part_size

  const int nch = 2;
  const size_t sample_size = sizeof(int16_t) * 2;

  int16_t *src;
  int16_t *end;
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
      memcpy(part_buf + part_size, rawdata, size);
      part_size += size;
      size = 0;
      return;
    }
    else
    {
      // finish & convert incomplete sample 
      memcpy(part_buf + part_size, rawdata, delta);
      drop_rawdata(delta);
      part_size = 0;

      src = (int16_t *)part_buf;
    dst[nsamples * 0] = sample_t(src[0]);
    dst[nsamples * 1] = sample_t(src[1]);

      dst++;
      out_size++;
    }   
  }

  /////////////////////////////////////////////////////////
  // Set processing buffer start & end and remember 
  // remaining part of sample

  src = (int16_t *)rawdata;

  // integral number of samples (and its size in bytes)
  size_t n = nsamples - out_size;
  size_t n_size = n * sample_size;

  if (n_size < size)
  {
    drop_rawdata(n_size);
    end = (int16_t *)(((uint8_t *)src) + n_size);
    out_size += n;
  }
  else
  {
    n = size / sample_size;
    n_size = n * sample_size;

    drop_rawdata(n_size);
    end = (int16_t *)(((uint8_t *)src) + n_size);
    out_size += n;

    // remember part of sample
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
    dst[nsamples * 0] = sample_t(src[0]);
    dst[nsamples * 1] = sample_t(src[1]);

    src += nch;
    dst++;
  }
}
void
Converter::pcm24_linear_2ch()
{
  // input: spk, nsamples, rawdata, size
  // output: out_samples, out_size
  // intermediate: part_buf, part_size

  const int nch = 2;
  const size_t sample_size = sizeof(int24_t) * 2;

  int24_t *src;
  int24_t *end;
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
      memcpy(part_buf + part_size, rawdata, size);
      part_size += size;
      size = 0;
      return;
    }
    else
    {
      // finish & convert incomplete sample 
      memcpy(part_buf + part_size, rawdata, delta);
      drop_rawdata(delta);
      part_size = 0;

      src = (int24_t *)part_buf;
    dst[nsamples * 0] = sample_t(src[0]);
    dst[nsamples * 1] = sample_t(src[1]);

      dst++;
      out_size++;
    }   
  }

  /////////////////////////////////////////////////////////
  // Set processing buffer start & end and remember 
  // remaining part of sample

  src = (int24_t *)rawdata;

  // integral number of samples (and its size in bytes)
  size_t n = nsamples - out_size;
  size_t n_size = n * sample_size;

  if (n_size < size)
  {
    drop_rawdata(n_size);
    end = (int24_t *)(((uint8_t *)src) + n_size);
    out_size += n;
  }
  else
  {
    n = size / sample_size;
    n_size = n * sample_size;

    drop_rawdata(n_size);
    end = (int24_t *)(((uint8_t *)src) + n_size);
    out_size += n;

    // remember part of sample
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
    dst[nsamples * 0] = sample_t(src[0]);
    dst[nsamples * 1] = sample_t(src[1]);

    src += nch;
    dst++;
  }
}
void
Converter::pcm32_linear_2ch()
{
  // input: spk, nsamples, rawdata, size
  // output: out_samples, out_size
  // intermediate: part_buf, part_size

  const int nch = 2;
  const size_t sample_size = sizeof(int32_t) * 2;

  int32_t *src;
  int32_t *end;
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
      memcpy(part_buf + part_size, rawdata, size);
      part_size += size;
      size = 0;
      return;
    }
    else
    {
      // finish & convert incomplete sample 
      memcpy(part_buf + part_size, rawdata, delta);
      drop_rawdata(delta);
      part_size = 0;

      src = (int32_t *)part_buf;
    dst[nsamples * 0] = sample_t(src[0]);
    dst[nsamples * 1] = sample_t(src[1]);

      dst++;
      out_size++;
    }   
  }

  /////////////////////////////////////////////////////////
  // Set processing buffer start & end and remember 
  // remaining part of sample

  src = (int32_t *)rawdata;

  // integral number of samples (and its size in bytes)
  size_t n = nsamples - out_size;
  size_t n_size = n * sample_size;

  if (n_size < size)
  {
    drop_rawdata(n_size);
    end = (int32_t *)(((uint8_t *)src) + n_size);
    out_size += n;
  }
  else
  {
    n = size / sample_size;
    n_size = n * sample_size;

    drop_rawdata(n_size);
    end = (int32_t *)(((uint8_t *)src) + n_size);
    out_size += n;

    // remember part of sample
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
    dst[nsamples * 0] = sample_t(src[0]);
    dst[nsamples * 1] = sample_t(src[1]);

    src += nch;
    dst++;
  }
}
void
Converter::pcmfloat_linear_2ch()
{
  // input: spk, nsamples, rawdata, size
  // output: out_samples, out_size
  // intermediate: part_buf, part_size

  const int nch = 2;
  const size_t sample_size = sizeof(float) * 2;

  float *src;
  float *end;
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
      memcpy(part_buf + part_size, rawdata, size);
      part_size += size;
      size = 0;
      return;
    }
    else
    {
      // finish & convert incomplete sample 
      memcpy(part_buf + part_size, rawdata, delta);
      drop_rawdata(delta);
      part_size = 0;

      src = (float *)part_buf;
    dst[nsamples * 0] = sample_t(src[0]);
    dst[nsamples * 1] = sample_t(src[1]);

      dst++;
      out_size++;
    }   
  }

  /////////////////////////////////////////////////////////
  // Set processing buffer start & end and remember 
  // remaining part of sample

  src = (float *)rawdata;

  // integral number of samples (and its size in bytes)
  size_t n = nsamples - out_size;
  size_t n_size = n * sample_size;

  if (n_size < size)
  {
    drop_rawdata(n_size);
    end = (float *)(((uint8_t *)src) + n_size);
    out_size += n;
  }
  else
  {
    n = size / sample_size;
    n_size = n * sample_size;

    drop_rawdata(n_size);
    end = (float *)(((uint8_t *)src) + n_size);
    out_size += n;

    // remember part of sample
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
    dst[nsamples * 0] = sample_t(src[0]);
    dst[nsamples * 1] = sample_t(src[1]);

    src += nch;
    dst++;
  }
}
void
Converter::pcm16_le_linear_2ch()
{
  // input: spk, nsamples, rawdata, size
  // output: out_samples, out_size
  // intermediate: part_buf, part_size

  const int nch = 2;
  const size_t sample_size = sizeof(int16_t) * 2;

  int16_t *src;
  int16_t *end;
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
      memcpy(part_buf + part_size, rawdata, size);
      part_size += size;
      size = 0;
      return;
    }
    else
    {
      // finish & convert incomplete sample 
      memcpy(part_buf + part_size, rawdata, delta);
      drop_rawdata(delta);
      part_size = 0;

      src = (int16_t *)part_buf;
    dst[nsamples * 0] = swab16(src[0]);
    dst[nsamples * 1] = swab16(src[1]);

      dst++;
      out_size++;
    }   
  }

  /////////////////////////////////////////////////////////
  // Set processing buffer start & end and remember 
  // remaining part of sample

  src = (int16_t *)rawdata;

  // integral number of samples (and its size in bytes)
  size_t n = nsamples - out_size;
  size_t n_size = n * sample_size;

  if (n_size < size)
  {
    drop_rawdata(n_size);
    end = (int16_t *)(((uint8_t *)src) + n_size);
    out_size += n;
  }
  else
  {
    n = size / sample_size;
    n_size = n * sample_size;

    drop_rawdata(n_size);
    end = (int16_t *)(((uint8_t *)src) + n_size);
    out_size += n;

    // remember part of sample
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
    dst[nsamples * 0] = swab16(src[0]);
    dst[nsamples * 1] = swab16(src[1]);

    src += nch;
    dst++;
  }
}
void
Converter::pcm24_le_linear_2ch()
{
  // input: spk, nsamples, rawdata, size
  // output: out_samples, out_size
  // intermediate: part_buf, part_size

  const int nch = 2;
  const size_t sample_size = sizeof(int24_t) * 2;

  int24_t *src;
  int24_t *end;
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
      memcpy(part_buf + part_size, rawdata, size);
      part_size += size;
      size = 0;
      return;
    }
    else
    {
      // finish & convert incomplete sample 
      memcpy(part_buf + part_size, rawdata, delta);
      drop_rawdata(delta);
      part_size = 0;

      src = (int24_t *)part_buf;
    dst[nsamples * 0] = swab24(src[0]);
    dst[nsamples * 1] = swab24(src[1]);

      dst++;
      out_size++;
    }   
  }

  /////////////////////////////////////////////////////////
  // Set processing buffer start & end and remember 
  // remaining part of sample

  src = (int24_t *)rawdata;

  // integral number of samples (and its size in bytes)
  size_t n = nsamples - out_size;
  size_t n_size = n * sample_size;

  if (n_size < size)
  {
    drop_rawdata(n_size);
    end = (int24_t *)(((uint8_t *)src) + n_size);
    out_size += n;
  }
  else
  {
    n = size / sample_size;
    n_size = n * sample_size;

    drop_rawdata(n_size);
    end = (int24_t *)(((uint8_t *)src) + n_size);
    out_size += n;

    // remember part of sample
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
    dst[nsamples * 0] = swab24(src[0]);
    dst[nsamples * 1] = swab24(src[1]);

    src += nch;
    dst++;
  }
}
void
Converter::pcm32_le_linear_2ch()
{
  // input: spk, nsamples, rawdata, size
  // output: out_samples, out_size
  // intermediate: part_buf, part_size

  const int nch = 2;
  const size_t sample_size = sizeof(int32_t) * 2;

  int32_t *src;
  int32_t *end;
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
      memcpy(part_buf + part_size, rawdata, size);
      part_size += size;
      size = 0;
      return;
    }
    else
    {
      // finish & convert incomplete sample 
      memcpy(part_buf + part_size, rawdata, delta);
      drop_rawdata(delta);
      part_size = 0;

      src = (int32_t *)part_buf;
    dst[nsamples * 0] = swab32(src[0]);
    dst[nsamples * 1] = swab32(src[1]);

      dst++;
      out_size++;
    }   
  }

  /////////////////////////////////////////////////////////
  // Set processing buffer start & end and remember 
  // remaining part of sample

  src = (int32_t *)rawdata;

  // integral number of samples (and its size in bytes)
  size_t n = nsamples - out_size;
  size_t n_size = n * sample_size;

  if (n_size < size)
  {
    drop_rawdata(n_size);
    end = (int32_t *)(((uint8_t *)src) + n_size);
    out_size += n;
  }
  else
  {
    n = size / sample_size;
    n_size = n * sample_size;

    drop_rawdata(n_size);
    end = (int32_t *)(((uint8_t *)src) + n_size);
    out_size += n;

    // remember part of sample
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
    dst[nsamples * 0] = swab32(src[0]);
    dst[nsamples * 1] = swab32(src[1]);

    src += nch;
    dst++;
  }
}
void
Converter::pcmfloat_le_linear_2ch()
{
  // input: spk, nsamples, rawdata, size
  // output: out_samples, out_size
  // intermediate: part_buf, part_size

  const int nch = 2;
  const size_t sample_size = sizeof(float) * 2;

  float *src;
  float *end;
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
      memcpy(part_buf + part_size, rawdata, size);
      part_size += size;
      size = 0;
      return;
    }
    else
    {
      // finish & convert incomplete sample 
      memcpy(part_buf + part_size, rawdata, delta);
      drop_rawdata(delta);
      part_size = 0;

      src = (float *)part_buf;
    dst[nsamples * 0] = swab_float(src[0]);
    dst[nsamples * 1] = swab_float(src[1]);

      dst++;
      out_size++;
    }   
  }

  /////////////////////////////////////////////////////////
  // Set processing buffer start & end and remember 
  // remaining part of sample

  src = (float *)rawdata;

  // integral number of samples (and its size in bytes)
  size_t n = nsamples - out_size;
  size_t n_size = n * sample_size;

  if (n_size < size)
  {
    drop_rawdata(n_size);
    end = (float *)(((uint8_t *)src) + n_size);
    out_size += n;
  }
  else
  {
    n = size / sample_size;
    n_size = n * sample_size;

    drop_rawdata(n_size);
    end = (float *)(((uint8_t *)src) + n_size);
    out_size += n;

    // remember part of sample
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
    dst[nsamples * 0] = swab_float(src[0]);
    dst[nsamples * 1] = swab_float(src[1]);

    src += nch;
    dst++;
  }
}

void
Converter::pcm16_linear_3ch()
{
  // input: spk, nsamples, rawdata, size
  // output: out_samples, out_size
  // intermediate: part_buf, part_size

  const int nch = 3;
  const size_t sample_size = sizeof(int16_t) * 3;

  int16_t *src;
  int16_t *end;
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
      memcpy(part_buf + part_size, rawdata, size);
      part_size += size;
      size = 0;
      return;
    }
    else
    {
      // finish & convert incomplete sample 
      memcpy(part_buf + part_size, rawdata, delta);
      drop_rawdata(delta);
      part_size = 0;

      src = (int16_t *)part_buf;
    dst[nsamples * 0] = sample_t(src[0]);
    dst[nsamples * 1] = sample_t(src[1]);
    dst[nsamples * 2] = sample_t(src[2]);

      dst++;
      out_size++;
    }   
  }

  /////////////////////////////////////////////////////////
  // Set processing buffer start & end and remember 
  // remaining part of sample

  src = (int16_t *)rawdata;

  // integral number of samples (and its size in bytes)
  size_t n = nsamples - out_size;
  size_t n_size = n * sample_size;

  if (n_size < size)
  {
    drop_rawdata(n_size);
    end = (int16_t *)(((uint8_t *)src) + n_size);
    out_size += n;
  }
  else
  {
    n = size / sample_size;
    n_size = n * sample_size;

    drop_rawdata(n_size);
    end = (int16_t *)(((uint8_t *)src) + n_size);
    out_size += n;

    // remember part of sample
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
    dst[nsamples * 0] = sample_t(src[0]);
    dst[nsamples * 1] = sample_t(src[1]);
    dst[nsamples * 2] = sample_t(src[2]);

    src += nch;
    dst++;
  }
}
void
Converter::pcm24_linear_3ch()
{
  // input: spk, nsamples, rawdata, size
  // output: out_samples, out_size
  // intermediate: part_buf, part_size

  const int nch = 3;
  const size_t sample_size = sizeof(int24_t) * 3;

  int24_t *src;
  int24_t *end;
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
      memcpy(part_buf + part_size, rawdata, size);
      part_size += size;
      size = 0;
      return;
    }
    else
    {
      // finish & convert incomplete sample 
      memcpy(part_buf + part_size, rawdata, delta);
      drop_rawdata(delta);
      part_size = 0;

      src = (int24_t *)part_buf;
    dst[nsamples * 0] = sample_t(src[0]);
    dst[nsamples * 1] = sample_t(src[1]);
    dst[nsamples * 2] = sample_t(src[2]);

      dst++;
      out_size++;
    }   
  }

  /////////////////////////////////////////////////////////
  // Set processing buffer start & end and remember 
  // remaining part of sample

  src = (int24_t *)rawdata;

  // integral number of samples (and its size in bytes)
  size_t n = nsamples - out_size;
  size_t n_size = n * sample_size;

  if (n_size < size)
  {
    drop_rawdata(n_size);
    end = (int24_t *)(((uint8_t *)src) + n_size);
    out_size += n;
  }
  else
  {
    n = size / sample_size;
    n_size = n * sample_size;

    drop_rawdata(n_size);
    end = (int24_t *)(((uint8_t *)src) + n_size);
    out_size += n;

    // remember part of sample
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
    dst[nsamples * 0] = sample_t(src[0]);
    dst[nsamples * 1] = sample_t(src[1]);
    dst[nsamples * 2] = sample_t(src[2]);

    src += nch;
    dst++;
  }
}
void
Converter::pcm32_linear_3ch()
{
  // input: spk, nsamples, rawdata, size
  // output: out_samples, out_size
  // intermediate: part_buf, part_size

  const int nch = 3;
  const size_t sample_size = sizeof(int32_t) * 3;

  int32_t *src;
  int32_t *end;
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
      memcpy(part_buf + part_size, rawdata, size);
      part_size += size;
      size = 0;
      return;
    }
    else
    {
      // finish & convert incomplete sample 
      memcpy(part_buf + part_size, rawdata, delta);
      drop_rawdata(delta);
      part_size = 0;

      src = (int32_t *)part_buf;
    dst[nsamples * 0] = sample_t(src[0]);
    dst[nsamples * 1] = sample_t(src[1]);
    dst[nsamples * 2] = sample_t(src[2]);

      dst++;
      out_size++;
    }   
  }

  /////////////////////////////////////////////////////////
  // Set processing buffer start & end and remember 
  // remaining part of sample

  src = (int32_t *)rawdata;

  // integral number of samples (and its size in bytes)
  size_t n = nsamples - out_size;
  size_t n_size = n * sample_size;

  if (n_size < size)
  {
    drop_rawdata(n_size);
    end = (int32_t *)(((uint8_t *)src) + n_size);
    out_size += n;
  }
  else
  {
    n = size / sample_size;
    n_size = n * sample_size;

    drop_rawdata(n_size);
    end = (int32_t *)(((uint8_t *)src) + n_size);
    out_size += n;

    // remember part of sample
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
    dst[nsamples * 0] = sample_t(src[0]);
    dst[nsamples * 1] = sample_t(src[1]);
    dst[nsamples * 2] = sample_t(src[2]);

    src += nch;
    dst++;
  }
}
void
Converter::pcmfloat_linear_3ch()
{
  // input: spk, nsamples, rawdata, size
  // output: out_samples, out_size
  // intermediate: part_buf, part_size

  const int nch = 3;
  const size_t sample_size = sizeof(float) * 3;

  float *src;
  float *end;
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
      memcpy(part_buf + part_size, rawdata, size);
      part_size += size;
      size = 0;
      return;
    }
    else
    {
      // finish & convert incomplete sample 
      memcpy(part_buf + part_size, rawdata, delta);
      drop_rawdata(delta);
      part_size = 0;

      src = (float *)part_buf;
    dst[nsamples * 0] = sample_t(src[0]);
    dst[nsamples * 1] = sample_t(src[1]);
    dst[nsamples * 2] = sample_t(src[2]);

      dst++;
      out_size++;
    }   
  }

  /////////////////////////////////////////////////////////
  // Set processing buffer start & end and remember 
  // remaining part of sample

  src = (float *)rawdata;

  // integral number of samples (and its size in bytes)
  size_t n = nsamples - out_size;
  size_t n_size = n * sample_size;

  if (n_size < size)
  {
    drop_rawdata(n_size);
    end = (float *)(((uint8_t *)src) + n_size);
    out_size += n;
  }
  else
  {
    n = size / sample_size;
    n_size = n * sample_size;

    drop_rawdata(n_size);
    end = (float *)(((uint8_t *)src) + n_size);
    out_size += n;

    // remember part of sample
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
    dst[nsamples * 0] = sample_t(src[0]);
    dst[nsamples * 1] = sample_t(src[1]);
    dst[nsamples * 2] = sample_t(src[2]);

    src += nch;
    dst++;
  }
}
void
Converter::pcm16_le_linear_3ch()
{
  // input: spk, nsamples, rawdata, size
  // output: out_samples, out_size
  // intermediate: part_buf, part_size

  const int nch = 3;
  const size_t sample_size = sizeof(int16_t) * 3;

  int16_t *src;
  int16_t *end;
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
      memcpy(part_buf + part_size, rawdata, size);
      part_size += size;
      size = 0;
      return;
    }
    else
    {
      // finish & convert incomplete sample 
      memcpy(part_buf + part_size, rawdata, delta);
      drop_rawdata(delta);
      part_size = 0;

      src = (int16_t *)part_buf;
    dst[nsamples * 0] = swab16(src[0]);
    dst[nsamples * 1] = swab16(src[1]);
    dst[nsamples * 2] = swab16(src[2]);

      dst++;
      out_size++;
    }   
  }

  /////////////////////////////////////////////////////////
  // Set processing buffer start & end and remember 
  // remaining part of sample

  src = (int16_t *)rawdata;

  // integral number of samples (and its size in bytes)
  size_t n = nsamples - out_size;
  size_t n_size = n * sample_size;

  if (n_size < size)
  {
    drop_rawdata(n_size);
    end = (int16_t *)(((uint8_t *)src) + n_size);
    out_size += n;
  }
  else
  {
    n = size / sample_size;
    n_size = n * sample_size;

    drop_rawdata(n_size);
    end = (int16_t *)(((uint8_t *)src) + n_size);
    out_size += n;

    // remember part of sample
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
    dst[nsamples * 0] = swab16(src[0]);
    dst[nsamples * 1] = swab16(src[1]);
    dst[nsamples * 2] = swab16(src[2]);

    src += nch;
    dst++;
  }
}
void
Converter::pcm24_le_linear_3ch()
{
  // input: spk, nsamples, rawdata, size
  // output: out_samples, out_size
  // intermediate: part_buf, part_size

  const int nch = 3;
  const size_t sample_size = sizeof(int24_t) * 3;

  int24_t *src;
  int24_t *end;
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
      memcpy(part_buf + part_size, rawdata, size);
      part_size += size;
      size = 0;
      return;
    }
    else
    {
      // finish & convert incomplete sample 
      memcpy(part_buf + part_size, rawdata, delta);
      drop_rawdata(delta);
      part_size = 0;

      src = (int24_t *)part_buf;
    dst[nsamples * 0] = swab24(src[0]);
    dst[nsamples * 1] = swab24(src[1]);
    dst[nsamples * 2] = swab24(src[2]);

      dst++;
      out_size++;
    }   
  }

  /////////////////////////////////////////////////////////
  // Set processing buffer start & end and remember 
  // remaining part of sample

  src = (int24_t *)rawdata;

  // integral number of samples (and its size in bytes)
  size_t n = nsamples - out_size;
  size_t n_size = n * sample_size;

  if (n_size < size)
  {
    drop_rawdata(n_size);
    end = (int24_t *)(((uint8_t *)src) + n_size);
    out_size += n;
  }
  else
  {
    n = size / sample_size;
    n_size = n * sample_size;

    drop_rawdata(n_size);
    end = (int24_t *)(((uint8_t *)src) + n_size);
    out_size += n;

    // remember part of sample
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
    dst[nsamples * 0] = swab24(src[0]);
    dst[nsamples * 1] = swab24(src[1]);
    dst[nsamples * 2] = swab24(src[2]);

    src += nch;
    dst++;
  }
}
void
Converter::pcm32_le_linear_3ch()
{
  // input: spk, nsamples, rawdata, size
  // output: out_samples, out_size
  // intermediate: part_buf, part_size

  const int nch = 3;
  const size_t sample_size = sizeof(int32_t) * 3;

  int32_t *src;
  int32_t *end;
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
      memcpy(part_buf + part_size, rawdata, size);
      part_size += size;
      size = 0;
      return;
    }
    else
    {
      // finish & convert incomplete sample 
      memcpy(part_buf + part_size, rawdata, delta);
      drop_rawdata(delta);
      part_size = 0;

      src = (int32_t *)part_buf;
    dst[nsamples * 0] = swab32(src[0]);
    dst[nsamples * 1] = swab32(src[1]);
    dst[nsamples * 2] = swab32(src[2]);

      dst++;
      out_size++;
    }   
  }

  /////////////////////////////////////////////////////////
  // Set processing buffer start & end and remember 
  // remaining part of sample

  src = (int32_t *)rawdata;

  // integral number of samples (and its size in bytes)
  size_t n = nsamples - out_size;
  size_t n_size = n * sample_size;

  if (n_size < size)
  {
    drop_rawdata(n_size);
    end = (int32_t *)(((uint8_t *)src) + n_size);
    out_size += n;
  }
  else
  {
    n = size / sample_size;
    n_size = n * sample_size;

    drop_rawdata(n_size);
    end = (int32_t *)(((uint8_t *)src) + n_size);
    out_size += n;

    // remember part of sample
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
    dst[nsamples * 0] = swab32(src[0]);
    dst[nsamples * 1] = swab32(src[1]);
    dst[nsamples * 2] = swab32(src[2]);

    src += nch;
    dst++;
  }
}
void
Converter::pcmfloat_le_linear_3ch()
{
  // input: spk, nsamples, rawdata, size
  // output: out_samples, out_size
  // intermediate: part_buf, part_size

  const int nch = 3;
  const size_t sample_size = sizeof(float) * 3;

  float *src;
  float *end;
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
      memcpy(part_buf + part_size, rawdata, size);
      part_size += size;
      size = 0;
      return;
    }
    else
    {
      // finish & convert incomplete sample 
      memcpy(part_buf + part_size, rawdata, delta);
      drop_rawdata(delta);
      part_size = 0;

      src = (float *)part_buf;
    dst[nsamples * 0] = swab_float(src[0]);
    dst[nsamples * 1] = swab_float(src[1]);
    dst[nsamples * 2] = swab_float(src[2]);

      dst++;
      out_size++;
    }   
  }

  /////////////////////////////////////////////////////////
  // Set processing buffer start & end and remember 
  // remaining part of sample

  src = (float *)rawdata;

  // integral number of samples (and its size in bytes)
  size_t n = nsamples - out_size;
  size_t n_size = n * sample_size;

  if (n_size < size)
  {
    drop_rawdata(n_size);
    end = (float *)(((uint8_t *)src) + n_size);
    out_size += n;
  }
  else
  {
    n = size / sample_size;
    n_size = n * sample_size;

    drop_rawdata(n_size);
    end = (float *)(((uint8_t *)src) + n_size);
    out_size += n;

    // remember part of sample
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
    dst[nsamples * 0] = swab_float(src[0]);
    dst[nsamples * 1] = swab_float(src[1]);
    dst[nsamples * 2] = swab_float(src[2]);

    src += nch;
    dst++;
  }
}

void
Converter::pcm16_linear_4ch()
{
  // input: spk, nsamples, rawdata, size
  // output: out_samples, out_size
  // intermediate: part_buf, part_size

  const int nch = 4;
  const size_t sample_size = sizeof(int16_t) * 4;

  int16_t *src;
  int16_t *end;
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
      memcpy(part_buf + part_size, rawdata, size);
      part_size += size;
      size = 0;
      return;
    }
    else
    {
      // finish & convert incomplete sample 
      memcpy(part_buf + part_size, rawdata, delta);
      drop_rawdata(delta);
      part_size = 0;

      src = (int16_t *)part_buf;
    dst[nsamples * 0] = sample_t(src[0]);
    dst[nsamples * 1] = sample_t(src[1]);
    dst[nsamples * 2] = sample_t(src[2]);
    dst[nsamples * 3] = sample_t(src[3]);

      dst++;
      out_size++;
    }   
  }

  /////////////////////////////////////////////////////////
  // Set processing buffer start & end and remember 
  // remaining part of sample

  src = (int16_t *)rawdata;

  // integral number of samples (and its size in bytes)
  size_t n = nsamples - out_size;
  size_t n_size = n * sample_size;

  if (n_size < size)
  {
    drop_rawdata(n_size);
    end = (int16_t *)(((uint8_t *)src) + n_size);
    out_size += n;
  }
  else
  {
    n = size / sample_size;
    n_size = n * sample_size;

    drop_rawdata(n_size);
    end = (int16_t *)(((uint8_t *)src) + n_size);
    out_size += n;

    // remember part of sample
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
    dst[nsamples * 0] = sample_t(src[0]);
    dst[nsamples * 1] = sample_t(src[1]);
    dst[nsamples * 2] = sample_t(src[2]);
    dst[nsamples * 3] = sample_t(src[3]);

    src += nch;
    dst++;
  }
}
void
Converter::pcm24_linear_4ch()
{
  // input: spk, nsamples, rawdata, size
  // output: out_samples, out_size
  // intermediate: part_buf, part_size

  const int nch = 4;
  const size_t sample_size = sizeof(int24_t) * 4;

  int24_t *src;
  int24_t *end;
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
      memcpy(part_buf + part_size, rawdata, size);
      part_size += size;
      size = 0;
      return;
    }
    else
    {
      // finish & convert incomplete sample 
      memcpy(part_buf + part_size, rawdata, delta);
      drop_rawdata(delta);
      part_size = 0;

      src = (int24_t *)part_buf;
    dst[nsamples * 0] = sample_t(src[0]);
    dst[nsamples * 1] = sample_t(src[1]);
    dst[nsamples * 2] = sample_t(src[2]);
    dst[nsamples * 3] = sample_t(src[3]);

      dst++;
      out_size++;
    }   
  }

  /////////////////////////////////////////////////////////
  // Set processing buffer start & end and remember 
  // remaining part of sample

  src = (int24_t *)rawdata;

  // integral number of samples (and its size in bytes)
  size_t n = nsamples - out_size;
  size_t n_size = n * sample_size;

  if (n_size < size)
  {
    drop_rawdata(n_size);
    end = (int24_t *)(((uint8_t *)src) + n_size);
    out_size += n;
  }
  else
  {
    n = size / sample_size;
    n_size = n * sample_size;

    drop_rawdata(n_size);
    end = (int24_t *)(((uint8_t *)src) + n_size);
    out_size += n;

    // remember part of sample
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
    dst[nsamples * 0] = sample_t(src[0]);
    dst[nsamples * 1] = sample_t(src[1]);
    dst[nsamples * 2] = sample_t(src[2]);
    dst[nsamples * 3] = sample_t(src[3]);

    src += nch;
    dst++;
  }
}
void
Converter::pcm32_linear_4ch()
{
  // input: spk, nsamples, rawdata, size
  // output: out_samples, out_size
  // intermediate: part_buf, part_size

  const int nch = 4;
  const size_t sample_size = sizeof(int32_t) * 4;

  int32_t *src;
  int32_t *end;
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
      memcpy(part_buf + part_size, rawdata, size);
      part_size += size;
      size = 0;
      return;
    }
    else
    {
      // finish & convert incomplete sample 
      memcpy(part_buf + part_size, rawdata, delta);
      drop_rawdata(delta);
      part_size = 0;

      src = (int32_t *)part_buf;
    dst[nsamples * 0] = sample_t(src[0]);
    dst[nsamples * 1] = sample_t(src[1]);
    dst[nsamples * 2] = sample_t(src[2]);
    dst[nsamples * 3] = sample_t(src[3]);

      dst++;
      out_size++;
    }   
  }

  /////////////////////////////////////////////////////////
  // Set processing buffer start & end and remember 
  // remaining part of sample

  src = (int32_t *)rawdata;

  // integral number of samples (and its size in bytes)
  size_t n = nsamples - out_size;
  size_t n_size = n * sample_size;

  if (n_size < size)
  {
    drop_rawdata(n_size);
    end = (int32_t *)(((uint8_t *)src) + n_size);
    out_size += n;
  }
  else
  {
    n = size / sample_size;
    n_size = n * sample_size;

    drop_rawdata(n_size);
    end = (int32_t *)(((uint8_t *)src) + n_size);
    out_size += n;

    // remember part of sample
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
    dst[nsamples * 0] = sample_t(src[0]);
    dst[nsamples * 1] = sample_t(src[1]);
    dst[nsamples * 2] = sample_t(src[2]);
    dst[nsamples * 3] = sample_t(src[3]);

    src += nch;
    dst++;
  }
}
void
Converter::pcmfloat_linear_4ch()
{
  // input: spk, nsamples, rawdata, size
  // output: out_samples, out_size
  // intermediate: part_buf, part_size

  const int nch = 4;
  const size_t sample_size = sizeof(float) * 4;

  float *src;
  float *end;
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
      memcpy(part_buf + part_size, rawdata, size);
      part_size += size;
      size = 0;
      return;
    }
    else
    {
      // finish & convert incomplete sample 
      memcpy(part_buf + part_size, rawdata, delta);
      drop_rawdata(delta);
      part_size = 0;

      src = (float *)part_buf;
    dst[nsamples * 0] = sample_t(src[0]);
    dst[nsamples * 1] = sample_t(src[1]);
    dst[nsamples * 2] = sample_t(src[2]);
    dst[nsamples * 3] = sample_t(src[3]);

      dst++;
      out_size++;
    }   
  }

  /////////////////////////////////////////////////////////
  // Set processing buffer start & end and remember 
  // remaining part of sample

  src = (float *)rawdata;

  // integral number of samples (and its size in bytes)
  size_t n = nsamples - out_size;
  size_t n_size = n * sample_size;

  if (n_size < size)
  {
    drop_rawdata(n_size);
    end = (float *)(((uint8_t *)src) + n_size);
    out_size += n;
  }
  else
  {
    n = size / sample_size;
    n_size = n * sample_size;

    drop_rawdata(n_size);
    end = (float *)(((uint8_t *)src) + n_size);
    out_size += n;

    // remember part of sample
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
    dst[nsamples * 0] = sample_t(src[0]);
    dst[nsamples * 1] = sample_t(src[1]);
    dst[nsamples * 2] = sample_t(src[2]);
    dst[nsamples * 3] = sample_t(src[3]);

    src += nch;
    dst++;
  }
}
void
Converter::pcm16_le_linear_4ch()
{
  // input: spk, nsamples, rawdata, size
  // output: out_samples, out_size
  // intermediate: part_buf, part_size

  const int nch = 4;
  const size_t sample_size = sizeof(int16_t) * 4;

  int16_t *src;
  int16_t *end;
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
      memcpy(part_buf + part_size, rawdata, size);
      part_size += size;
      size = 0;
      return;
    }
    else
    {
      // finish & convert incomplete sample 
      memcpy(part_buf + part_size, rawdata, delta);
      drop_rawdata(delta);
      part_size = 0;

      src = (int16_t *)part_buf;
    dst[nsamples * 0] = swab16(src[0]);
    dst[nsamples * 1] = swab16(src[1]);
    dst[nsamples * 2] = swab16(src[2]);
    dst[nsamples * 3] = swab16(src[3]);

      dst++;
      out_size++;
    }   
  }

  /////////////////////////////////////////////////////////
  // Set processing buffer start & end and remember 
  // remaining part of sample

  src = (int16_t *)rawdata;

  // integral number of samples (and its size in bytes)
  size_t n = nsamples - out_size;
  size_t n_size = n * sample_size;

  if (n_size < size)
  {
    drop_rawdata(n_size);
    end = (int16_t *)(((uint8_t *)src) + n_size);
    out_size += n;
  }
  else
  {
    n = size / sample_size;
    n_size = n * sample_size;

    drop_rawdata(n_size);
    end = (int16_t *)(((uint8_t *)src) + n_size);
    out_size += n;

    // remember part of sample
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
    dst[nsamples * 0] = swab16(src[0]);
    dst[nsamples * 1] = swab16(src[1]);
    dst[nsamples * 2] = swab16(src[2]);
    dst[nsamples * 3] = swab16(src[3]);

    src += nch;
    dst++;
  }
}
void
Converter::pcm24_le_linear_4ch()
{
  // input: spk, nsamples, rawdata, size
  // output: out_samples, out_size
  // intermediate: part_buf, part_size

  const int nch = 4;
  const size_t sample_size = sizeof(int24_t) * 4;

  int24_t *src;
  int24_t *end;
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
      memcpy(part_buf + part_size, rawdata, size);
      part_size += size;
      size = 0;
      return;
    }
    else
    {
      // finish & convert incomplete sample 
      memcpy(part_buf + part_size, rawdata, delta);
      drop_rawdata(delta);
      part_size = 0;

      src = (int24_t *)part_buf;
    dst[nsamples * 0] = swab24(src[0]);
    dst[nsamples * 1] = swab24(src[1]);
    dst[nsamples * 2] = swab24(src[2]);
    dst[nsamples * 3] = swab24(src[3]);

      dst++;
      out_size++;
    }   
  }

  /////////////////////////////////////////////////////////
  // Set processing buffer start & end and remember 
  // remaining part of sample

  src = (int24_t *)rawdata;

  // integral number of samples (and its size in bytes)
  size_t n = nsamples - out_size;
  size_t n_size = n * sample_size;

  if (n_size < size)
  {
    drop_rawdata(n_size);
    end = (int24_t *)(((uint8_t *)src) + n_size);
    out_size += n;
  }
  else
  {
    n = size / sample_size;
    n_size = n * sample_size;

    drop_rawdata(n_size);
    end = (int24_t *)(((uint8_t *)src) + n_size);
    out_size += n;

    // remember part of sample
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
    dst[nsamples * 0] = swab24(src[0]);
    dst[nsamples * 1] = swab24(src[1]);
    dst[nsamples * 2] = swab24(src[2]);
    dst[nsamples * 3] = swab24(src[3]);

    src += nch;
    dst++;
  }
}
void
Converter::pcm32_le_linear_4ch()
{
  // input: spk, nsamples, rawdata, size
  // output: out_samples, out_size
  // intermediate: part_buf, part_size

  const int nch = 4;
  const size_t sample_size = sizeof(int32_t) * 4;

  int32_t *src;
  int32_t *end;
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
      memcpy(part_buf + part_size, rawdata, size);
      part_size += size;
      size = 0;
      return;
    }
    else
    {
      // finish & convert incomplete sample 
      memcpy(part_buf + part_size, rawdata, delta);
      drop_rawdata(delta);
      part_size = 0;

      src = (int32_t *)part_buf;
    dst[nsamples * 0] = swab32(src[0]);
    dst[nsamples * 1] = swab32(src[1]);
    dst[nsamples * 2] = swab32(src[2]);
    dst[nsamples * 3] = swab32(src[3]);

      dst++;
      out_size++;
    }   
  }

  /////////////////////////////////////////////////////////
  // Set processing buffer start & end and remember 
  // remaining part of sample

  src = (int32_t *)rawdata;

  // integral number of samples (and its size in bytes)
  size_t n = nsamples - out_size;
  size_t n_size = n * sample_size;

  if (n_size < size)
  {
    drop_rawdata(n_size);
    end = (int32_t *)(((uint8_t *)src) + n_size);
    out_size += n;
  }
  else
  {
    n = size / sample_size;
    n_size = n * sample_size;

    drop_rawdata(n_size);
    end = (int32_t *)(((uint8_t *)src) + n_size);
    out_size += n;

    // remember part of sample
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
    dst[nsamples * 0] = swab32(src[0]);
    dst[nsamples * 1] = swab32(src[1]);
    dst[nsamples * 2] = swab32(src[2]);
    dst[nsamples * 3] = swab32(src[3]);

    src += nch;
    dst++;
  }
}
void
Converter::pcmfloat_le_linear_4ch()
{
  // input: spk, nsamples, rawdata, size
  // output: out_samples, out_size
  // intermediate: part_buf, part_size

  const int nch = 4;
  const size_t sample_size = sizeof(float) * 4;

  float *src;
  float *end;
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
      memcpy(part_buf + part_size, rawdata, size);
      part_size += size;
      size = 0;
      return;
    }
    else
    {
      // finish & convert incomplete sample 
      memcpy(part_buf + part_size, rawdata, delta);
      drop_rawdata(delta);
      part_size = 0;

      src = (float *)part_buf;
    dst[nsamples * 0] = swab_float(src[0]);
    dst[nsamples * 1] = swab_float(src[1]);
    dst[nsamples * 2] = swab_float(src[2]);
    dst[nsamples * 3] = swab_float(src[3]);

      dst++;
      out_size++;
    }   
  }

  /////////////////////////////////////////////////////////
  // Set processing buffer start & end and remember 
  // remaining part of sample

  src = (float *)rawdata;

  // integral number of samples (and its size in bytes)
  size_t n = nsamples - out_size;
  size_t n_size = n * sample_size;

  if (n_size < size)
  {
    drop_rawdata(n_size);
    end = (float *)(((uint8_t *)src) + n_size);
    out_size += n;
  }
  else
  {
    n = size / sample_size;
    n_size = n * sample_size;

    drop_rawdata(n_size);
    end = (float *)(((uint8_t *)src) + n_size);
    out_size += n;

    // remember part of sample
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
    dst[nsamples * 0] = swab_float(src[0]);
    dst[nsamples * 1] = swab_float(src[1]);
    dst[nsamples * 2] = swab_float(src[2]);
    dst[nsamples * 3] = swab_float(src[3]);

    src += nch;
    dst++;
  }
}

void
Converter::pcm16_linear_5ch()
{
  // input: spk, nsamples, rawdata, size
  // output: out_samples, out_size
  // intermediate: part_buf, part_size

  const int nch = 5;
  const size_t sample_size = sizeof(int16_t) * 5;

  int16_t *src;
  int16_t *end;
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
      memcpy(part_buf + part_size, rawdata, size);
      part_size += size;
      size = 0;
      return;
    }
    else
    {
      // finish & convert incomplete sample 
      memcpy(part_buf + part_size, rawdata, delta);
      drop_rawdata(delta);
      part_size = 0;

      src = (int16_t *)part_buf;
    dst[nsamples * 0] = sample_t(src[0]);
    dst[nsamples * 1] = sample_t(src[1]);
    dst[nsamples * 2] = sample_t(src[2]);
    dst[nsamples * 3] = sample_t(src[3]);
    dst[nsamples * 4] = sample_t(src[4]);

      dst++;
      out_size++;
    }   
  }

  /////////////////////////////////////////////////////////
  // Set processing buffer start & end and remember 
  // remaining part of sample

  src = (int16_t *)rawdata;

  // integral number of samples (and its size in bytes)
  size_t n = nsamples - out_size;
  size_t n_size = n * sample_size;

  if (n_size < size)
  {
    drop_rawdata(n_size);
    end = (int16_t *)(((uint8_t *)src) + n_size);
    out_size += n;
  }
  else
  {
    n = size / sample_size;
    n_size = n * sample_size;

    drop_rawdata(n_size);
    end = (int16_t *)(((uint8_t *)src) + n_size);
    out_size += n;

    // remember part of sample
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
    dst[nsamples * 0] = sample_t(src[0]);
    dst[nsamples * 1] = sample_t(src[1]);
    dst[nsamples * 2] = sample_t(src[2]);
    dst[nsamples * 3] = sample_t(src[3]);
    dst[nsamples * 4] = sample_t(src[4]);

    src += nch;
    dst++;
  }
}
void
Converter::pcm24_linear_5ch()
{
  // input: spk, nsamples, rawdata, size
  // output: out_samples, out_size
  // intermediate: part_buf, part_size

  const int nch = 5;
  const size_t sample_size = sizeof(int24_t) * 5;

  int24_t *src;
  int24_t *end;
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
      memcpy(part_buf + part_size, rawdata, size);
      part_size += size;
      size = 0;
      return;
    }
    else
    {
      // finish & convert incomplete sample 
      memcpy(part_buf + part_size, rawdata, delta);
      drop_rawdata(delta);
      part_size = 0;

      src = (int24_t *)part_buf;
    dst[nsamples * 0] = sample_t(src[0]);
    dst[nsamples * 1] = sample_t(src[1]);
    dst[nsamples * 2] = sample_t(src[2]);
    dst[nsamples * 3] = sample_t(src[3]);
    dst[nsamples * 4] = sample_t(src[4]);

      dst++;
      out_size++;
    }   
  }

  /////////////////////////////////////////////////////////
  // Set processing buffer start & end and remember 
  // remaining part of sample

  src = (int24_t *)rawdata;

  // integral number of samples (and its size in bytes)
  size_t n = nsamples - out_size;
  size_t n_size = n * sample_size;

  if (n_size < size)
  {
    drop_rawdata(n_size);
    end = (int24_t *)(((uint8_t *)src) + n_size);
    out_size += n;
  }
  else
  {
    n = size / sample_size;
    n_size = n * sample_size;

    drop_rawdata(n_size);
    end = (int24_t *)(((uint8_t *)src) + n_size);
    out_size += n;

    // remember part of sample
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
    dst[nsamples * 0] = sample_t(src[0]);
    dst[nsamples * 1] = sample_t(src[1]);
    dst[nsamples * 2] = sample_t(src[2]);
    dst[nsamples * 3] = sample_t(src[3]);
    dst[nsamples * 4] = sample_t(src[4]);

    src += nch;
    dst++;
  }
}
void
Converter::pcm32_linear_5ch()
{
  // input: spk, nsamples, rawdata, size
  // output: out_samples, out_size
  // intermediate: part_buf, part_size

  const int nch = 5;
  const size_t sample_size = sizeof(int32_t) * 5;

  int32_t *src;
  int32_t *end;
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
      memcpy(part_buf + part_size, rawdata, size);
      part_size += size;
      size = 0;
      return;
    }
    else
    {
      // finish & convert incomplete sample 
      memcpy(part_buf + part_size, rawdata, delta);
      drop_rawdata(delta);
      part_size = 0;

      src = (int32_t *)part_buf;
    dst[nsamples * 0] = sample_t(src[0]);
    dst[nsamples * 1] = sample_t(src[1]);
    dst[nsamples * 2] = sample_t(src[2]);
    dst[nsamples * 3] = sample_t(src[3]);
    dst[nsamples * 4] = sample_t(src[4]);

      dst++;
      out_size++;
    }   
  }

  /////////////////////////////////////////////////////////
  // Set processing buffer start & end and remember 
  // remaining part of sample

  src = (int32_t *)rawdata;

  // integral number of samples (and its size in bytes)
  size_t n = nsamples - out_size;
  size_t n_size = n * sample_size;

  if (n_size < size)
  {
    drop_rawdata(n_size);
    end = (int32_t *)(((uint8_t *)src) + n_size);
    out_size += n;
  }
  else
  {
    n = size / sample_size;
    n_size = n * sample_size;

    drop_rawdata(n_size);
    end = (int32_t *)(((uint8_t *)src) + n_size);
    out_size += n;

    // remember part of sample
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
    dst[nsamples * 0] = sample_t(src[0]);
    dst[nsamples * 1] = sample_t(src[1]);
    dst[nsamples * 2] = sample_t(src[2]);
    dst[nsamples * 3] = sample_t(src[3]);
    dst[nsamples * 4] = sample_t(src[4]);

    src += nch;
    dst++;
  }
}
void
Converter::pcmfloat_linear_5ch()
{
  // input: spk, nsamples, rawdata, size
  // output: out_samples, out_size
  // intermediate: part_buf, part_size

  const int nch = 5;
  const size_t sample_size = sizeof(float) * 5;

  float *src;
  float *end;
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
      memcpy(part_buf + part_size, rawdata, size);
      part_size += size;
      size = 0;
      return;
    }
    else
    {
      // finish & convert incomplete sample 
      memcpy(part_buf + part_size, rawdata, delta);
      drop_rawdata(delta);
      part_size = 0;

      src = (float *)part_buf;
    dst[nsamples * 0] = sample_t(src[0]);
    dst[nsamples * 1] = sample_t(src[1]);
    dst[nsamples * 2] = sample_t(src[2]);
    dst[nsamples * 3] = sample_t(src[3]);
    dst[nsamples * 4] = sample_t(src[4]);

      dst++;
      out_size++;
    }   
  }

  /////////////////////////////////////////////////////////
  // Set processing buffer start & end and remember 
  // remaining part of sample

  src = (float *)rawdata;

  // integral number of samples (and its size in bytes)
  size_t n = nsamples - out_size;
  size_t n_size = n * sample_size;

  if (n_size < size)
  {
    drop_rawdata(n_size);
    end = (float *)(((uint8_t *)src) + n_size);
    out_size += n;
  }
  else
  {
    n = size / sample_size;
    n_size = n * sample_size;

    drop_rawdata(n_size);
    end = (float *)(((uint8_t *)src) + n_size);
    out_size += n;

    // remember part of sample
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
    dst[nsamples * 0] = sample_t(src[0]);
    dst[nsamples * 1] = sample_t(src[1]);
    dst[nsamples * 2] = sample_t(src[2]);
    dst[nsamples * 3] = sample_t(src[3]);
    dst[nsamples * 4] = sample_t(src[4]);

    src += nch;
    dst++;
  }
}
void
Converter::pcm16_le_linear_5ch()
{
  // input: spk, nsamples, rawdata, size
  // output: out_samples, out_size
  // intermediate: part_buf, part_size

  const int nch = 5;
  const size_t sample_size = sizeof(int16_t) * 5;

  int16_t *src;
  int16_t *end;
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
      memcpy(part_buf + part_size, rawdata, size);
      part_size += size;
      size = 0;
      return;
    }
    else
    {
      // finish & convert incomplete sample 
      memcpy(part_buf + part_size, rawdata, delta);
      drop_rawdata(delta);
      part_size = 0;

      src = (int16_t *)part_buf;
    dst[nsamples * 0] = swab16(src[0]);
    dst[nsamples * 1] = swab16(src[1]);
    dst[nsamples * 2] = swab16(src[2]);
    dst[nsamples * 3] = swab16(src[3]);
    dst[nsamples * 4] = swab16(src[4]);

      dst++;
      out_size++;
    }   
  }

  /////////////////////////////////////////////////////////
  // Set processing buffer start & end and remember 
  // remaining part of sample

  src = (int16_t *)rawdata;

  // integral number of samples (and its size in bytes)
  size_t n = nsamples - out_size;
  size_t n_size = n * sample_size;

  if (n_size < size)
  {
    drop_rawdata(n_size);
    end = (int16_t *)(((uint8_t *)src) + n_size);
    out_size += n;
  }
  else
  {
    n = size / sample_size;
    n_size = n * sample_size;

    drop_rawdata(n_size);
    end = (int16_t *)(((uint8_t *)src) + n_size);
    out_size += n;

    // remember part of sample
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
    dst[nsamples * 0] = swab16(src[0]);
    dst[nsamples * 1] = swab16(src[1]);
    dst[nsamples * 2] = swab16(src[2]);
    dst[nsamples * 3] = swab16(src[3]);
    dst[nsamples * 4] = swab16(src[4]);

    src += nch;
    dst++;
  }
}
void
Converter::pcm24_le_linear_5ch()
{
  // input: spk, nsamples, rawdata, size
  // output: out_samples, out_size
  // intermediate: part_buf, part_size

  const int nch = 5;
  const size_t sample_size = sizeof(int24_t) * 5;

  int24_t *src;
  int24_t *end;
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
      memcpy(part_buf + part_size, rawdata, size);
      part_size += size;
      size = 0;
      return;
    }
    else
    {
      // finish & convert incomplete sample 
      memcpy(part_buf + part_size, rawdata, delta);
      drop_rawdata(delta);
      part_size = 0;

      src = (int24_t *)part_buf;
    dst[nsamples * 0] = swab24(src[0]);
    dst[nsamples * 1] = swab24(src[1]);
    dst[nsamples * 2] = swab24(src[2]);
    dst[nsamples * 3] = swab24(src[3]);
    dst[nsamples * 4] = swab24(src[4]);

      dst++;
      out_size++;
    }   
  }

  /////////////////////////////////////////////////////////
  // Set processing buffer start & end and remember 
  // remaining part of sample

  src = (int24_t *)rawdata;

  // integral number of samples (and its size in bytes)
  size_t n = nsamples - out_size;
  size_t n_size = n * sample_size;

  if (n_size < size)
  {
    drop_rawdata(n_size);
    end = (int24_t *)(((uint8_t *)src) + n_size);
    out_size += n;
  }
  else
  {
    n = size / sample_size;
    n_size = n * sample_size;

    drop_rawdata(n_size);
    end = (int24_t *)(((uint8_t *)src) + n_size);
    out_size += n;

    // remember part of sample
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
    dst[nsamples * 0] = swab24(src[0]);
    dst[nsamples * 1] = swab24(src[1]);
    dst[nsamples * 2] = swab24(src[2]);
    dst[nsamples * 3] = swab24(src[3]);
    dst[nsamples * 4] = swab24(src[4]);

    src += nch;
    dst++;
  }
}
void
Converter::pcm32_le_linear_5ch()
{
  // input: spk, nsamples, rawdata, size
  // output: out_samples, out_size
  // intermediate: part_buf, part_size

  const int nch = 5;
  const size_t sample_size = sizeof(int32_t) * 5;

  int32_t *src;
  int32_t *end;
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
      memcpy(part_buf + part_size, rawdata, size);
      part_size += size;
      size = 0;
      return;
    }
    else
    {
      // finish & convert incomplete sample 
      memcpy(part_buf + part_size, rawdata, delta);
      drop_rawdata(delta);
      part_size = 0;

      src = (int32_t *)part_buf;
    dst[nsamples * 0] = swab32(src[0]);
    dst[nsamples * 1] = swab32(src[1]);
    dst[nsamples * 2] = swab32(src[2]);
    dst[nsamples * 3] = swab32(src[3]);
    dst[nsamples * 4] = swab32(src[4]);

      dst++;
      out_size++;
    }   
  }

  /////////////////////////////////////////////////////////
  // Set processing buffer start & end and remember 
  // remaining part of sample

  src = (int32_t *)rawdata;

  // integral number of samples (and its size in bytes)
  size_t n = nsamples - out_size;
  size_t n_size = n * sample_size;

  if (n_size < size)
  {
    drop_rawdata(n_size);
    end = (int32_t *)(((uint8_t *)src) + n_size);
    out_size += n;
  }
  else
  {
    n = size / sample_size;
    n_size = n * sample_size;

    drop_rawdata(n_size);
    end = (int32_t *)(((uint8_t *)src) + n_size);
    out_size += n;

    // remember part of sample
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
    dst[nsamples * 0] = swab32(src[0]);
    dst[nsamples * 1] = swab32(src[1]);
    dst[nsamples * 2] = swab32(src[2]);
    dst[nsamples * 3] = swab32(src[3]);
    dst[nsamples * 4] = swab32(src[4]);

    src += nch;
    dst++;
  }
}
void
Converter::pcmfloat_le_linear_5ch()
{
  // input: spk, nsamples, rawdata, size
  // output: out_samples, out_size
  // intermediate: part_buf, part_size

  const int nch = 5;
  const size_t sample_size = sizeof(float) * 5;

  float *src;
  float *end;
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
      memcpy(part_buf + part_size, rawdata, size);
      part_size += size;
      size = 0;
      return;
    }
    else
    {
      // finish & convert incomplete sample 
      memcpy(part_buf + part_size, rawdata, delta);
      drop_rawdata(delta);
      part_size = 0;

      src = (float *)part_buf;
    dst[nsamples * 0] = swab_float(src[0]);
    dst[nsamples * 1] = swab_float(src[1]);
    dst[nsamples * 2] = swab_float(src[2]);
    dst[nsamples * 3] = swab_float(src[3]);
    dst[nsamples * 4] = swab_float(src[4]);

      dst++;
      out_size++;
    }   
  }

  /////////////////////////////////////////////////////////
  // Set processing buffer start & end and remember 
  // remaining part of sample

  src = (float *)rawdata;

  // integral number of samples (and its size in bytes)
  size_t n = nsamples - out_size;
  size_t n_size = n * sample_size;

  if (n_size < size)
  {
    drop_rawdata(n_size);
    end = (float *)(((uint8_t *)src) + n_size);
    out_size += n;
  }
  else
  {
    n = size / sample_size;
    n_size = n * sample_size;

    drop_rawdata(n_size);
    end = (float *)(((uint8_t *)src) + n_size);
    out_size += n;

    // remember part of sample
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
    dst[nsamples * 0] = swab_float(src[0]);
    dst[nsamples * 1] = swab_float(src[1]);
    dst[nsamples * 2] = swab_float(src[2]);
    dst[nsamples * 3] = swab_float(src[3]);
    dst[nsamples * 4] = swab_float(src[4]);

    src += nch;
    dst++;
  }
}

void
Converter::pcm16_linear_6ch()
{
  // input: spk, nsamples, rawdata, size
  // output: out_samples, out_size
  // intermediate: part_buf, part_size

  const int nch = 6;
  const size_t sample_size = sizeof(int16_t) * 6;

  int16_t *src;
  int16_t *end;
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
      memcpy(part_buf + part_size, rawdata, size);
      part_size += size;
      size = 0;
      return;
    }
    else
    {
      // finish & convert incomplete sample 
      memcpy(part_buf + part_size, rawdata, delta);
      drop_rawdata(delta);
      part_size = 0;

      src = (int16_t *)part_buf;
    dst[nsamples * 0] = sample_t(src[0]);
    dst[nsamples * 1] = sample_t(src[1]);
    dst[nsamples * 2] = sample_t(src[2]);
    dst[nsamples * 3] = sample_t(src[3]);
    dst[nsamples * 4] = sample_t(src[4]);
    dst[nsamples * 5] = sample_t(src[5]);

      dst++;
      out_size++;
    }   
  }

  /////////////////////////////////////////////////////////
  // Set processing buffer start & end and remember 
  // remaining part of sample

  src = (int16_t *)rawdata;

  // integral number of samples (and its size in bytes)
  size_t n = nsamples - out_size;
  size_t n_size = n * sample_size;

  if (n_size < size)
  {
    drop_rawdata(n_size);
    end = (int16_t *)(((uint8_t *)src) + n_size);
    out_size += n;
  }
  else
  {
    n = size / sample_size;
    n_size = n * sample_size;

    drop_rawdata(n_size);
    end = (int16_t *)(((uint8_t *)src) + n_size);
    out_size += n;

    // remember part of sample
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
    dst[nsamples * 0] = sample_t(src[0]);
    dst[nsamples * 1] = sample_t(src[1]);
    dst[nsamples * 2] = sample_t(src[2]);
    dst[nsamples * 3] = sample_t(src[3]);
    dst[nsamples * 4] = sample_t(src[4]);
    dst[nsamples * 5] = sample_t(src[5]);

    src += nch;
    dst++;
  }
}
void
Converter::pcm24_linear_6ch()
{
  // input: spk, nsamples, rawdata, size
  // output: out_samples, out_size
  // intermediate: part_buf, part_size

  const int nch = 6;
  const size_t sample_size = sizeof(int24_t) * 6;

  int24_t *src;
  int24_t *end;
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
      memcpy(part_buf + part_size, rawdata, size);
      part_size += size;
      size = 0;
      return;
    }
    else
    {
      // finish & convert incomplete sample 
      memcpy(part_buf + part_size, rawdata, delta);
      drop_rawdata(delta);
      part_size = 0;

      src = (int24_t *)part_buf;
    dst[nsamples * 0] = sample_t(src[0]);
    dst[nsamples * 1] = sample_t(src[1]);
    dst[nsamples * 2] = sample_t(src[2]);
    dst[nsamples * 3] = sample_t(src[3]);
    dst[nsamples * 4] = sample_t(src[4]);
    dst[nsamples * 5] = sample_t(src[5]);

      dst++;
      out_size++;
    }   
  }

  /////////////////////////////////////////////////////////
  // Set processing buffer start & end and remember 
  // remaining part of sample

  src = (int24_t *)rawdata;

  // integral number of samples (and its size in bytes)
  size_t n = nsamples - out_size;
  size_t n_size = n * sample_size;

  if (n_size < size)
  {
    drop_rawdata(n_size);
    end = (int24_t *)(((uint8_t *)src) + n_size);
    out_size += n;
  }
  else
  {
    n = size / sample_size;
    n_size = n * sample_size;

    drop_rawdata(n_size);
    end = (int24_t *)(((uint8_t *)src) + n_size);
    out_size += n;

    // remember part of sample
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
    dst[nsamples * 0] = sample_t(src[0]);
    dst[nsamples * 1] = sample_t(src[1]);
    dst[nsamples * 2] = sample_t(src[2]);
    dst[nsamples * 3] = sample_t(src[3]);
    dst[nsamples * 4] = sample_t(src[4]);
    dst[nsamples * 5] = sample_t(src[5]);

    src += nch;
    dst++;
  }
}
void
Converter::pcm32_linear_6ch()
{
  // input: spk, nsamples, rawdata, size
  // output: out_samples, out_size
  // intermediate: part_buf, part_size

  const int nch = 6;
  const size_t sample_size = sizeof(int32_t) * 6;

  int32_t *src;
  int32_t *end;
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
      memcpy(part_buf + part_size, rawdata, size);
      part_size += size;
      size = 0;
      return;
    }
    else
    {
      // finish & convert incomplete sample 
      memcpy(part_buf + part_size, rawdata, delta);
      drop_rawdata(delta);
      part_size = 0;

      src = (int32_t *)part_buf;
    dst[nsamples * 0] = sample_t(src[0]);
    dst[nsamples * 1] = sample_t(src[1]);
    dst[nsamples * 2] = sample_t(src[2]);
    dst[nsamples * 3] = sample_t(src[3]);
    dst[nsamples * 4] = sample_t(src[4]);
    dst[nsamples * 5] = sample_t(src[5]);

      dst++;
      out_size++;
    }   
  }

  /////////////////////////////////////////////////////////
  // Set processing buffer start & end and remember 
  // remaining part of sample

  src = (int32_t *)rawdata;

  // integral number of samples (and its size in bytes)
  size_t n = nsamples - out_size;
  size_t n_size = n * sample_size;

  if (n_size < size)
  {
    drop_rawdata(n_size);
    end = (int32_t *)(((uint8_t *)src) + n_size);
    out_size += n;
  }
  else
  {
    n = size / sample_size;
    n_size = n * sample_size;

    drop_rawdata(n_size);
    end = (int32_t *)(((uint8_t *)src) + n_size);
    out_size += n;

    // remember part of sample
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
    dst[nsamples * 0] = sample_t(src[0]);
    dst[nsamples * 1] = sample_t(src[1]);
    dst[nsamples * 2] = sample_t(src[2]);
    dst[nsamples * 3] = sample_t(src[3]);
    dst[nsamples * 4] = sample_t(src[4]);
    dst[nsamples * 5] = sample_t(src[5]);

    src += nch;
    dst++;
  }
}
void
Converter::pcmfloat_linear_6ch()
{
  // input: spk, nsamples, rawdata, size
  // output: out_samples, out_size
  // intermediate: part_buf, part_size

  const int nch = 6;
  const size_t sample_size = sizeof(float) * 6;

  float *src;
  float *end;
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
      memcpy(part_buf + part_size, rawdata, size);
      part_size += size;
      size = 0;
      return;
    }
    else
    {
      // finish & convert incomplete sample 
      memcpy(part_buf + part_size, rawdata, delta);
      drop_rawdata(delta);
      part_size = 0;

      src = (float *)part_buf;
    dst[nsamples * 0] = sample_t(src[0]);
    dst[nsamples * 1] = sample_t(src[1]);
    dst[nsamples * 2] = sample_t(src[2]);
    dst[nsamples * 3] = sample_t(src[3]);
    dst[nsamples * 4] = sample_t(src[4]);
    dst[nsamples * 5] = sample_t(src[5]);

      dst++;
      out_size++;
    }   
  }

  /////////////////////////////////////////////////////////
  // Set processing buffer start & end and remember 
  // remaining part of sample

  src = (float *)rawdata;

  // integral number of samples (and its size in bytes)
  size_t n = nsamples - out_size;
  size_t n_size = n * sample_size;

  if (n_size < size)
  {
    drop_rawdata(n_size);
    end = (float *)(((uint8_t *)src) + n_size);
    out_size += n;
  }
  else
  {
    n = size / sample_size;
    n_size = n * sample_size;

    drop_rawdata(n_size);
    end = (float *)(((uint8_t *)src) + n_size);
    out_size += n;

    // remember part of sample
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
    dst[nsamples * 0] = sample_t(src[0]);
    dst[nsamples * 1] = sample_t(src[1]);
    dst[nsamples * 2] = sample_t(src[2]);
    dst[nsamples * 3] = sample_t(src[3]);
    dst[nsamples * 4] = sample_t(src[4]);
    dst[nsamples * 5] = sample_t(src[5]);

    src += nch;
    dst++;
  }
}
void
Converter::pcm16_le_linear_6ch()
{
  // input: spk, nsamples, rawdata, size
  // output: out_samples, out_size
  // intermediate: part_buf, part_size

  const int nch = 6;
  const size_t sample_size = sizeof(int16_t) * 6;

  int16_t *src;
  int16_t *end;
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
      memcpy(part_buf + part_size, rawdata, size);
      part_size += size;
      size = 0;
      return;
    }
    else
    {
      // finish & convert incomplete sample 
      memcpy(part_buf + part_size, rawdata, delta);
      drop_rawdata(delta);
      part_size = 0;

      src = (int16_t *)part_buf;
    dst[nsamples * 0] = swab16(src[0]);
    dst[nsamples * 1] = swab16(src[1]);
    dst[nsamples * 2] = swab16(src[2]);
    dst[nsamples * 3] = swab16(src[3]);
    dst[nsamples * 4] = swab16(src[4]);
    dst[nsamples * 5] = swab16(src[5]);

      dst++;
      out_size++;
    }   
  }

  /////////////////////////////////////////////////////////
  // Set processing buffer start & end and remember 
  // remaining part of sample

  src = (int16_t *)rawdata;

  // integral number of samples (and its size in bytes)
  size_t n = nsamples - out_size;
  size_t n_size = n * sample_size;

  if (n_size < size)
  {
    drop_rawdata(n_size);
    end = (int16_t *)(((uint8_t *)src) + n_size);
    out_size += n;
  }
  else
  {
    n = size / sample_size;
    n_size = n * sample_size;

    drop_rawdata(n_size);
    end = (int16_t *)(((uint8_t *)src) + n_size);
    out_size += n;

    // remember part of sample
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
    dst[nsamples * 0] = swab16(src[0]);
    dst[nsamples * 1] = swab16(src[1]);
    dst[nsamples * 2] = swab16(src[2]);
    dst[nsamples * 3] = swab16(src[3]);
    dst[nsamples * 4] = swab16(src[4]);
    dst[nsamples * 5] = swab16(src[5]);

    src += nch;
    dst++;
  }
}
void
Converter::pcm24_le_linear_6ch()
{
  // input: spk, nsamples, rawdata, size
  // output: out_samples, out_size
  // intermediate: part_buf, part_size

  const int nch = 6;
  const size_t sample_size = sizeof(int24_t) * 6;

  int24_t *src;
  int24_t *end;
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
      memcpy(part_buf + part_size, rawdata, size);
      part_size += size;
      size = 0;
      return;
    }
    else
    {
      // finish & convert incomplete sample 
      memcpy(part_buf + part_size, rawdata, delta);
      drop_rawdata(delta);
      part_size = 0;

      src = (int24_t *)part_buf;
    dst[nsamples * 0] = swab24(src[0]);
    dst[nsamples * 1] = swab24(src[1]);
    dst[nsamples * 2] = swab24(src[2]);
    dst[nsamples * 3] = swab24(src[3]);
    dst[nsamples * 4] = swab24(src[4]);
    dst[nsamples * 5] = swab24(src[5]);

      dst++;
      out_size++;
    }   
  }

  /////////////////////////////////////////////////////////
  // Set processing buffer start & end and remember 
  // remaining part of sample

  src = (int24_t *)rawdata;

  // integral number of samples (and its size in bytes)
  size_t n = nsamples - out_size;
  size_t n_size = n * sample_size;

  if (n_size < size)
  {
    drop_rawdata(n_size);
    end = (int24_t *)(((uint8_t *)src) + n_size);
    out_size += n;
  }
  else
  {
    n = size / sample_size;
    n_size = n * sample_size;

    drop_rawdata(n_size);
    end = (int24_t *)(((uint8_t *)src) + n_size);
    out_size += n;

    // remember part of sample
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
    dst[nsamples * 0] = swab24(src[0]);
    dst[nsamples * 1] = swab24(src[1]);
    dst[nsamples * 2] = swab24(src[2]);
    dst[nsamples * 3] = swab24(src[3]);
    dst[nsamples * 4] = swab24(src[4]);
    dst[nsamples * 5] = swab24(src[5]);

    src += nch;
    dst++;
  }
}
void
Converter::pcm32_le_linear_6ch()
{
  // input: spk, nsamples, rawdata, size
  // output: out_samples, out_size
  // intermediate: part_buf, part_size

  const int nch = 6;
  const size_t sample_size = sizeof(int32_t) * 6;

  int32_t *src;
  int32_t *end;
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
      memcpy(part_buf + part_size, rawdata, size);
      part_size += size;
      size = 0;
      return;
    }
    else
    {
      // finish & convert incomplete sample 
      memcpy(part_buf + part_size, rawdata, delta);
      drop_rawdata(delta);
      part_size = 0;

      src = (int32_t *)part_buf;
    dst[nsamples * 0] = swab32(src[0]);
    dst[nsamples * 1] = swab32(src[1]);
    dst[nsamples * 2] = swab32(src[2]);
    dst[nsamples * 3] = swab32(src[3]);
    dst[nsamples * 4] = swab32(src[4]);
    dst[nsamples * 5] = swab32(src[5]);

      dst++;
      out_size++;
    }   
  }

  /////////////////////////////////////////////////////////
  // Set processing buffer start & end and remember 
  // remaining part of sample

  src = (int32_t *)rawdata;

  // integral number of samples (and its size in bytes)
  size_t n = nsamples - out_size;
  size_t n_size = n * sample_size;

  if (n_size < size)
  {
    drop_rawdata(n_size);
    end = (int32_t *)(((uint8_t *)src) + n_size);
    out_size += n;
  }
  else
  {
    n = size / sample_size;
    n_size = n * sample_size;

    drop_rawdata(n_size);
    end = (int32_t *)(((uint8_t *)src) + n_size);
    out_size += n;

    // remember part of sample
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
    dst[nsamples * 0] = swab32(src[0]);
    dst[nsamples * 1] = swab32(src[1]);
    dst[nsamples * 2] = swab32(src[2]);
    dst[nsamples * 3] = swab32(src[3]);
    dst[nsamples * 4] = swab32(src[4]);
    dst[nsamples * 5] = swab32(src[5]);

    src += nch;
    dst++;
  }
}
void
Converter::pcmfloat_le_linear_6ch()
{
  // input: spk, nsamples, rawdata, size
  // output: out_samples, out_size
  // intermediate: part_buf, part_size

  const int nch = 6;
  const size_t sample_size = sizeof(float) * 6;

  float *src;
  float *end;
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
      memcpy(part_buf + part_size, rawdata, size);
      part_size += size;
      size = 0;
      return;
    }
    else
    {
      // finish & convert incomplete sample 
      memcpy(part_buf + part_size, rawdata, delta);
      drop_rawdata(delta);
      part_size = 0;

      src = (float *)part_buf;
    dst[nsamples * 0] = swab_float(src[0]);
    dst[nsamples * 1] = swab_float(src[1]);
    dst[nsamples * 2] = swab_float(src[2]);
    dst[nsamples * 3] = swab_float(src[3]);
    dst[nsamples * 4] = swab_float(src[4]);
    dst[nsamples * 5] = swab_float(src[5]);

      dst++;
      out_size++;
    }   
  }

  /////////////////////////////////////////////////////////
  // Set processing buffer start & end and remember 
  // remaining part of sample

  src = (float *)rawdata;

  // integral number of samples (and its size in bytes)
  size_t n = nsamples - out_size;
  size_t n_size = n * sample_size;

  if (n_size < size)
  {
    drop_rawdata(n_size);
    end = (float *)(((uint8_t *)src) + n_size);
    out_size += n;
  }
  else
  {
    n = size / sample_size;
    n_size = n * sample_size;

    drop_rawdata(n_size);
    end = (float *)(((uint8_t *)src) + n_size);
    out_size += n;

    // remember part of sample
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
    dst[nsamples * 0] = swab_float(src[0]);
    dst[nsamples * 1] = swab_float(src[1]);
    dst[nsamples * 2] = swab_float(src[2]);
    dst[nsamples * 3] = swab_float(src[3]);
    dst[nsamples * 4] = swab_float(src[4]);
    dst[nsamples * 5] = swab_float(src[5]);

    src += nch;
    dst++;
  }
}

