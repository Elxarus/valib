  bool pcm16_linear_1ch(Chunk *);
  bool pcm24_linear_1ch(Chunk *);
  bool pcm32_linear_1ch(Chunk *);
  bool pcmfloat_linear_1ch(Chunk *);
  bool pcm16_le_linear_1ch(Chunk *);
  bool pcm24_le_linear_1ch(Chunk *);
  bool pcm32_le_linear_1ch(Chunk *);
  bool pcmfloat_le_linear_1ch(Chunk *);

  bool pcm16_linear_2ch(Chunk *);
  bool pcm24_linear_2ch(Chunk *);
  bool pcm32_linear_2ch(Chunk *);
  bool pcmfloat_linear_2ch(Chunk *);
  bool pcm16_le_linear_2ch(Chunk *);
  bool pcm24_le_linear_2ch(Chunk *);
  bool pcm32_le_linear_2ch(Chunk *);
  bool pcmfloat_le_linear_2ch(Chunk *);

  bool pcm16_linear_3ch(Chunk *);
  bool pcm24_linear_3ch(Chunk *);
  bool pcm32_linear_3ch(Chunk *);
  bool pcmfloat_linear_3ch(Chunk *);
  bool pcm16_le_linear_3ch(Chunk *);
  bool pcm24_le_linear_3ch(Chunk *);
  bool pcm32_le_linear_3ch(Chunk *);
  bool pcmfloat_le_linear_3ch(Chunk *);

  bool pcm16_linear_4ch(Chunk *);
  bool pcm24_linear_4ch(Chunk *);
  bool pcm32_linear_4ch(Chunk *);
  bool pcmfloat_linear_4ch(Chunk *);
  bool pcm16_le_linear_4ch(Chunk *);
  bool pcm24_le_linear_4ch(Chunk *);
  bool pcm32_le_linear_4ch(Chunk *);
  bool pcmfloat_le_linear_4ch(Chunk *);

  bool pcm16_linear_5ch(Chunk *);
  bool pcm24_linear_5ch(Chunk *);
  bool pcm32_linear_5ch(Chunk *);
  bool pcmfloat_linear_5ch(Chunk *);
  bool pcm16_le_linear_5ch(Chunk *);
  bool pcm24_le_linear_5ch(Chunk *);
  bool pcm32_le_linear_5ch(Chunk *);
  bool pcmfloat_le_linear_5ch(Chunk *);

  bool pcm16_linear_6ch(Chunk *);
  bool pcm24_linear_6ch(Chunk *);
  bool pcm32_linear_6ch(Chunk *);
  bool pcmfloat_linear_6ch(Chunk *);
  bool pcm16_le_linear_6ch(Chunk *);
  bool pcm24_le_linear_6ch(Chunk *);
  bool pcm32_le_linear_6ch(Chunk *);
  bool pcmfloat_le_linear_6ch(Chunk *);

typedef bool (Converter::*convert_t)(Chunk *);

static const int formats_tbl[] = {FORMAT_PCM16, FORMAT_PCM24, FORMAT_PCM32, FORMAT_PCMFLOAT, FORMAT_PCM16_LE, FORMAT_PCM24_LE, FORMAT_PCM32_LE, FORMAT_PCMFLOAT_LE}

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
Converter::pcm16_linear_1ch(Chunk *_chunk)
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
      send_empty(_chunk);
      return true;
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

  /////////////////////////////////////////////////////////
  // Fill output chunk

  send_samples(_chunk);
  return true;
}
bool
Converter::pcm24_linear_1ch(Chunk *_chunk)
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
      send_empty(_chunk);
      return true;
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

  /////////////////////////////////////////////////////////
  // Fill output chunk

  send_samples(_chunk);
  return true;
}
bool
Converter::pcm32_linear_1ch(Chunk *_chunk)
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
      send_empty(_chunk);
      return true;
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

  /////////////////////////////////////////////////////////
  // Fill output chunk

  send_samples(_chunk);
  return true;
}
bool
Converter::pcmfloat_linear_1ch(Chunk *_chunk)
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
      send_empty(_chunk);
      return true;
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

  /////////////////////////////////////////////////////////
  // Fill output chunk

  send_samples(_chunk);
  return true;
}
bool
Converter::pcm16_le_linear_1ch(Chunk *_chunk)
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
      send_empty(_chunk);
      return true;
    }
    else
    {
      // finish & convert incomplete sample 
      memcpy(part_buf + part_size, rawdata, delta);
      drop_rawdata(delta);
      part_size = 0;

      src = (int16_t *)part_buf;
    dst[nsamples * 0] = swab_s16(src[0]);

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
    dst[nsamples * 0] = swab_s16(src[0]);

    src += nch;
    dst++;
  }

  /////////////////////////////////////////////////////////
  // Fill output chunk

  send_samples(_chunk);
  return true;
}
bool
Converter::pcm24_le_linear_1ch(Chunk *_chunk)
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
      send_empty(_chunk);
      return true;
    }
    else
    {
      // finish & convert incomplete sample 
      memcpy(part_buf + part_size, rawdata, delta);
      drop_rawdata(delta);
      part_size = 0;

      src = (int24_t *)part_buf;
    dst[nsamples * 0] = swab_s24(src[0]);

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
    dst[nsamples * 0] = swab_s24(src[0]);

    src += nch;
    dst++;
  }

  /////////////////////////////////////////////////////////
  // Fill output chunk

  send_samples(_chunk);
  return true;
}
bool
Converter::pcm32_le_linear_1ch(Chunk *_chunk)
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
      send_empty(_chunk);
      return true;
    }
    else
    {
      // finish & convert incomplete sample 
      memcpy(part_buf + part_size, rawdata, delta);
      drop_rawdata(delta);
      part_size = 0;

      src = (int32_t *)part_buf;
    dst[nsamples * 0] = swab_s32(src[0]);

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
    dst[nsamples * 0] = swab_s32(src[0]);

    src += nch;
    dst++;
  }

  /////////////////////////////////////////////////////////
  // Fill output chunk

  send_samples(_chunk);
  return true;
}
bool
Converter::pcmfloat_le_linear_1ch(Chunk *_chunk)
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
      send_empty(_chunk);
      return true;
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

  /////////////////////////////////////////////////////////
  // Fill output chunk

  send_samples(_chunk);
  return true;
}

bool
Converter::pcm16_linear_2ch(Chunk *_chunk)
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
      send_empty(_chunk);
      return true;
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

  /////////////////////////////////////////////////////////
  // Fill output chunk

  send_samples(_chunk);
  return true;
}
bool
Converter::pcm24_linear_2ch(Chunk *_chunk)
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
      send_empty(_chunk);
      return true;
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

  /////////////////////////////////////////////////////////
  // Fill output chunk

  send_samples(_chunk);
  return true;
}
bool
Converter::pcm32_linear_2ch(Chunk *_chunk)
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
      send_empty(_chunk);
      return true;
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

  /////////////////////////////////////////////////////////
  // Fill output chunk

  send_samples(_chunk);
  return true;
}
bool
Converter::pcmfloat_linear_2ch(Chunk *_chunk)
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
      send_empty(_chunk);
      return true;
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

  /////////////////////////////////////////////////////////
  // Fill output chunk

  send_samples(_chunk);
  return true;
}
bool
Converter::pcm16_le_linear_2ch(Chunk *_chunk)
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
      send_empty(_chunk);
      return true;
    }
    else
    {
      // finish & convert incomplete sample 
      memcpy(part_buf + part_size, rawdata, delta);
      drop_rawdata(delta);
      part_size = 0;

      src = (int16_t *)part_buf;
    dst[nsamples * 0] = swab_s16(src[0]);
    dst[nsamples * 1] = swab_s16(src[1]);

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
    dst[nsamples * 0] = swab_s16(src[0]);
    dst[nsamples * 1] = swab_s16(src[1]);

    src += nch;
    dst++;
  }

  /////////////////////////////////////////////////////////
  // Fill output chunk

  send_samples(_chunk);
  return true;
}
bool
Converter::pcm24_le_linear_2ch(Chunk *_chunk)
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
      send_empty(_chunk);
      return true;
    }
    else
    {
      // finish & convert incomplete sample 
      memcpy(part_buf + part_size, rawdata, delta);
      drop_rawdata(delta);
      part_size = 0;

      src = (int24_t *)part_buf;
    dst[nsamples * 0] = swab_s24(src[0]);
    dst[nsamples * 1] = swab_s24(src[1]);

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
    dst[nsamples * 0] = swab_s24(src[0]);
    dst[nsamples * 1] = swab_s24(src[1]);

    src += nch;
    dst++;
  }

  /////////////////////////////////////////////////////////
  // Fill output chunk

  send_samples(_chunk);
  return true;
}
bool
Converter::pcm32_le_linear_2ch(Chunk *_chunk)
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
      send_empty(_chunk);
      return true;
    }
    else
    {
      // finish & convert incomplete sample 
      memcpy(part_buf + part_size, rawdata, delta);
      drop_rawdata(delta);
      part_size = 0;

      src = (int32_t *)part_buf;
    dst[nsamples * 0] = swab_s32(src[0]);
    dst[nsamples * 1] = swab_s32(src[1]);

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
    dst[nsamples * 0] = swab_s32(src[0]);
    dst[nsamples * 1] = swab_s32(src[1]);

    src += nch;
    dst++;
  }

  /////////////////////////////////////////////////////////
  // Fill output chunk

  send_samples(_chunk);
  return true;
}
bool
Converter::pcmfloat_le_linear_2ch(Chunk *_chunk)
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
      send_empty(_chunk);
      return true;
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

  /////////////////////////////////////////////////////////
  // Fill output chunk

  send_samples(_chunk);
  return true;
}

bool
Converter::pcm16_linear_3ch(Chunk *_chunk)
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
      send_empty(_chunk);
      return true;
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

  /////////////////////////////////////////////////////////
  // Fill output chunk

  send_samples(_chunk);
  return true;
}
bool
Converter::pcm24_linear_3ch(Chunk *_chunk)
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
      send_empty(_chunk);
      return true;
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

  /////////////////////////////////////////////////////////
  // Fill output chunk

  send_samples(_chunk);
  return true;
}
bool
Converter::pcm32_linear_3ch(Chunk *_chunk)
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
      send_empty(_chunk);
      return true;
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

  /////////////////////////////////////////////////////////
  // Fill output chunk

  send_samples(_chunk);
  return true;
}
bool
Converter::pcmfloat_linear_3ch(Chunk *_chunk)
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
      send_empty(_chunk);
      return true;
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

  /////////////////////////////////////////////////////////
  // Fill output chunk

  send_samples(_chunk);
  return true;
}
bool
Converter::pcm16_le_linear_3ch(Chunk *_chunk)
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
      send_empty(_chunk);
      return true;
    }
    else
    {
      // finish & convert incomplete sample 
      memcpy(part_buf + part_size, rawdata, delta);
      drop_rawdata(delta);
      part_size = 0;

      src = (int16_t *)part_buf;
    dst[nsamples * 0] = swab_s16(src[0]);
    dst[nsamples * 1] = swab_s16(src[1]);
    dst[nsamples * 2] = swab_s16(src[2]);

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
    dst[nsamples * 0] = swab_s16(src[0]);
    dst[nsamples * 1] = swab_s16(src[1]);
    dst[nsamples * 2] = swab_s16(src[2]);

    src += nch;
    dst++;
  }

  /////////////////////////////////////////////////////////
  // Fill output chunk

  send_samples(_chunk);
  return true;
}
bool
Converter::pcm24_le_linear_3ch(Chunk *_chunk)
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
      send_empty(_chunk);
      return true;
    }
    else
    {
      // finish & convert incomplete sample 
      memcpy(part_buf + part_size, rawdata, delta);
      drop_rawdata(delta);
      part_size = 0;

      src = (int24_t *)part_buf;
    dst[nsamples * 0] = swab_s24(src[0]);
    dst[nsamples * 1] = swab_s24(src[1]);
    dst[nsamples * 2] = swab_s24(src[2]);

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
    dst[nsamples * 0] = swab_s24(src[0]);
    dst[nsamples * 1] = swab_s24(src[1]);
    dst[nsamples * 2] = swab_s24(src[2]);

    src += nch;
    dst++;
  }

  /////////////////////////////////////////////////////////
  // Fill output chunk

  send_samples(_chunk);
  return true;
}
bool
Converter::pcm32_le_linear_3ch(Chunk *_chunk)
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
      send_empty(_chunk);
      return true;
    }
    else
    {
      // finish & convert incomplete sample 
      memcpy(part_buf + part_size, rawdata, delta);
      drop_rawdata(delta);
      part_size = 0;

      src = (int32_t *)part_buf;
    dst[nsamples * 0] = swab_s32(src[0]);
    dst[nsamples * 1] = swab_s32(src[1]);
    dst[nsamples * 2] = swab_s32(src[2]);

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
    dst[nsamples * 0] = swab_s32(src[0]);
    dst[nsamples * 1] = swab_s32(src[1]);
    dst[nsamples * 2] = swab_s32(src[2]);

    src += nch;
    dst++;
  }

  /////////////////////////////////////////////////////////
  // Fill output chunk

  send_samples(_chunk);
  return true;
}
bool
Converter::pcmfloat_le_linear_3ch(Chunk *_chunk)
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
      send_empty(_chunk);
      return true;
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

  /////////////////////////////////////////////////////////
  // Fill output chunk

  send_samples(_chunk);
  return true;
}

bool
Converter::pcm16_linear_4ch(Chunk *_chunk)
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
      send_empty(_chunk);
      return true;
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

  /////////////////////////////////////////////////////////
  // Fill output chunk

  send_samples(_chunk);
  return true;
}
bool
Converter::pcm24_linear_4ch(Chunk *_chunk)
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
      send_empty(_chunk);
      return true;
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

  /////////////////////////////////////////////////////////
  // Fill output chunk

  send_samples(_chunk);
  return true;
}
bool
Converter::pcm32_linear_4ch(Chunk *_chunk)
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
      send_empty(_chunk);
      return true;
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

  /////////////////////////////////////////////////////////
  // Fill output chunk

  send_samples(_chunk);
  return true;
}
bool
Converter::pcmfloat_linear_4ch(Chunk *_chunk)
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
      send_empty(_chunk);
      return true;
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

  /////////////////////////////////////////////////////////
  // Fill output chunk

  send_samples(_chunk);
  return true;
}
bool
Converter::pcm16_le_linear_4ch(Chunk *_chunk)
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
      send_empty(_chunk);
      return true;
    }
    else
    {
      // finish & convert incomplete sample 
      memcpy(part_buf + part_size, rawdata, delta);
      drop_rawdata(delta);
      part_size = 0;

      src = (int16_t *)part_buf;
    dst[nsamples * 0] = swab_s16(src[0]);
    dst[nsamples * 1] = swab_s16(src[1]);
    dst[nsamples * 2] = swab_s16(src[2]);
    dst[nsamples * 3] = swab_s16(src[3]);

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
    dst[nsamples * 0] = swab_s16(src[0]);
    dst[nsamples * 1] = swab_s16(src[1]);
    dst[nsamples * 2] = swab_s16(src[2]);
    dst[nsamples * 3] = swab_s16(src[3]);

    src += nch;
    dst++;
  }

  /////////////////////////////////////////////////////////
  // Fill output chunk

  send_samples(_chunk);
  return true;
}
bool
Converter::pcm24_le_linear_4ch(Chunk *_chunk)
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
      send_empty(_chunk);
      return true;
    }
    else
    {
      // finish & convert incomplete sample 
      memcpy(part_buf + part_size, rawdata, delta);
      drop_rawdata(delta);
      part_size = 0;

      src = (int24_t *)part_buf;
    dst[nsamples * 0] = swab_s24(src[0]);
    dst[nsamples * 1] = swab_s24(src[1]);
    dst[nsamples * 2] = swab_s24(src[2]);
    dst[nsamples * 3] = swab_s24(src[3]);

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
    dst[nsamples * 0] = swab_s24(src[0]);
    dst[nsamples * 1] = swab_s24(src[1]);
    dst[nsamples * 2] = swab_s24(src[2]);
    dst[nsamples * 3] = swab_s24(src[3]);

    src += nch;
    dst++;
  }

  /////////////////////////////////////////////////////////
  // Fill output chunk

  send_samples(_chunk);
  return true;
}
bool
Converter::pcm32_le_linear_4ch(Chunk *_chunk)
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
      send_empty(_chunk);
      return true;
    }
    else
    {
      // finish & convert incomplete sample 
      memcpy(part_buf + part_size, rawdata, delta);
      drop_rawdata(delta);
      part_size = 0;

      src = (int32_t *)part_buf;
    dst[nsamples * 0] = swab_s32(src[0]);
    dst[nsamples * 1] = swab_s32(src[1]);
    dst[nsamples * 2] = swab_s32(src[2]);
    dst[nsamples * 3] = swab_s32(src[3]);

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
    dst[nsamples * 0] = swab_s32(src[0]);
    dst[nsamples * 1] = swab_s32(src[1]);
    dst[nsamples * 2] = swab_s32(src[2]);
    dst[nsamples * 3] = swab_s32(src[3]);

    src += nch;
    dst++;
  }

  /////////////////////////////////////////////////////////
  // Fill output chunk

  send_samples(_chunk);
  return true;
}
bool
Converter::pcmfloat_le_linear_4ch(Chunk *_chunk)
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
      send_empty(_chunk);
      return true;
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

  /////////////////////////////////////////////////////////
  // Fill output chunk

  send_samples(_chunk);
  return true;
}

bool
Converter::pcm16_linear_5ch(Chunk *_chunk)
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
      send_empty(_chunk);
      return true;
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

  /////////////////////////////////////////////////////////
  // Fill output chunk

  send_samples(_chunk);
  return true;
}
bool
Converter::pcm24_linear_5ch(Chunk *_chunk)
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
      send_empty(_chunk);
      return true;
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

  /////////////////////////////////////////////////////////
  // Fill output chunk

  send_samples(_chunk);
  return true;
}
bool
Converter::pcm32_linear_5ch(Chunk *_chunk)
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
      send_empty(_chunk);
      return true;
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

  /////////////////////////////////////////////////////////
  // Fill output chunk

  send_samples(_chunk);
  return true;
}
bool
Converter::pcmfloat_linear_5ch(Chunk *_chunk)
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
      send_empty(_chunk);
      return true;
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

  /////////////////////////////////////////////////////////
  // Fill output chunk

  send_samples(_chunk);
  return true;
}
bool
Converter::pcm16_le_linear_5ch(Chunk *_chunk)
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
      send_empty(_chunk);
      return true;
    }
    else
    {
      // finish & convert incomplete sample 
      memcpy(part_buf + part_size, rawdata, delta);
      drop_rawdata(delta);
      part_size = 0;

      src = (int16_t *)part_buf;
    dst[nsamples * 0] = swab_s16(src[0]);
    dst[nsamples * 1] = swab_s16(src[1]);
    dst[nsamples * 2] = swab_s16(src[2]);
    dst[nsamples * 3] = swab_s16(src[3]);
    dst[nsamples * 4] = swab_s16(src[4]);

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
    dst[nsamples * 0] = swab_s16(src[0]);
    dst[nsamples * 1] = swab_s16(src[1]);
    dst[nsamples * 2] = swab_s16(src[2]);
    dst[nsamples * 3] = swab_s16(src[3]);
    dst[nsamples * 4] = swab_s16(src[4]);

    src += nch;
    dst++;
  }

  /////////////////////////////////////////////////////////
  // Fill output chunk

  send_samples(_chunk);
  return true;
}
bool
Converter::pcm24_le_linear_5ch(Chunk *_chunk)
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
      send_empty(_chunk);
      return true;
    }
    else
    {
      // finish & convert incomplete sample 
      memcpy(part_buf + part_size, rawdata, delta);
      drop_rawdata(delta);
      part_size = 0;

      src = (int24_t *)part_buf;
    dst[nsamples * 0] = swab_s24(src[0]);
    dst[nsamples * 1] = swab_s24(src[1]);
    dst[nsamples * 2] = swab_s24(src[2]);
    dst[nsamples * 3] = swab_s24(src[3]);
    dst[nsamples * 4] = swab_s24(src[4]);

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
    dst[nsamples * 0] = swab_s24(src[0]);
    dst[nsamples * 1] = swab_s24(src[1]);
    dst[nsamples * 2] = swab_s24(src[2]);
    dst[nsamples * 3] = swab_s24(src[3]);
    dst[nsamples * 4] = swab_s24(src[4]);

    src += nch;
    dst++;
  }

  /////////////////////////////////////////////////////////
  // Fill output chunk

  send_samples(_chunk);
  return true;
}
bool
Converter::pcm32_le_linear_5ch(Chunk *_chunk)
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
      send_empty(_chunk);
      return true;
    }
    else
    {
      // finish & convert incomplete sample 
      memcpy(part_buf + part_size, rawdata, delta);
      drop_rawdata(delta);
      part_size = 0;

      src = (int32_t *)part_buf;
    dst[nsamples * 0] = swab_s32(src[0]);
    dst[nsamples * 1] = swab_s32(src[1]);
    dst[nsamples * 2] = swab_s32(src[2]);
    dst[nsamples * 3] = swab_s32(src[3]);
    dst[nsamples * 4] = swab_s32(src[4]);

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
    dst[nsamples * 0] = swab_s32(src[0]);
    dst[nsamples * 1] = swab_s32(src[1]);
    dst[nsamples * 2] = swab_s32(src[2]);
    dst[nsamples * 3] = swab_s32(src[3]);
    dst[nsamples * 4] = swab_s32(src[4]);

    src += nch;
    dst++;
  }

  /////////////////////////////////////////////////////////
  // Fill output chunk

  send_samples(_chunk);
  return true;
}
bool
Converter::pcmfloat_le_linear_5ch(Chunk *_chunk)
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
      send_empty(_chunk);
      return true;
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

  /////////////////////////////////////////////////////////
  // Fill output chunk

  send_samples(_chunk);
  return true;
}

bool
Converter::pcm16_linear_6ch(Chunk *_chunk)
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
      send_empty(_chunk);
      return true;
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

  /////////////////////////////////////////////////////////
  // Fill output chunk

  send_samples(_chunk);
  return true;
}
bool
Converter::pcm24_linear_6ch(Chunk *_chunk)
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
      send_empty(_chunk);
      return true;
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

  /////////////////////////////////////////////////////////
  // Fill output chunk

  send_samples(_chunk);
  return true;
}
bool
Converter::pcm32_linear_6ch(Chunk *_chunk)
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
      send_empty(_chunk);
      return true;
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

  /////////////////////////////////////////////////////////
  // Fill output chunk

  send_samples(_chunk);
  return true;
}
bool
Converter::pcmfloat_linear_6ch(Chunk *_chunk)
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
      send_empty(_chunk);
      return true;
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

  /////////////////////////////////////////////////////////
  // Fill output chunk

  send_samples(_chunk);
  return true;
}
bool
Converter::pcm16_le_linear_6ch(Chunk *_chunk)
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
      send_empty(_chunk);
      return true;
    }
    else
    {
      // finish & convert incomplete sample 
      memcpy(part_buf + part_size, rawdata, delta);
      drop_rawdata(delta);
      part_size = 0;

      src = (int16_t *)part_buf;
    dst[nsamples * 0] = swab_s16(src[0]);
    dst[nsamples * 1] = swab_s16(src[1]);
    dst[nsamples * 2] = swab_s16(src[2]);
    dst[nsamples * 3] = swab_s16(src[3]);
    dst[nsamples * 4] = swab_s16(src[4]);
    dst[nsamples * 5] = swab_s16(src[5]);

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
    dst[nsamples * 0] = swab_s16(src[0]);
    dst[nsamples * 1] = swab_s16(src[1]);
    dst[nsamples * 2] = swab_s16(src[2]);
    dst[nsamples * 3] = swab_s16(src[3]);
    dst[nsamples * 4] = swab_s16(src[4]);
    dst[nsamples * 5] = swab_s16(src[5]);

    src += nch;
    dst++;
  }

  /////////////////////////////////////////////////////////
  // Fill output chunk

  send_samples(_chunk);
  return true;
}
bool
Converter::pcm24_le_linear_6ch(Chunk *_chunk)
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
      send_empty(_chunk);
      return true;
    }
    else
    {
      // finish & convert incomplete sample 
      memcpy(part_buf + part_size, rawdata, delta);
      drop_rawdata(delta);
      part_size = 0;

      src = (int24_t *)part_buf;
    dst[nsamples * 0] = swab_s24(src[0]);
    dst[nsamples * 1] = swab_s24(src[1]);
    dst[nsamples * 2] = swab_s24(src[2]);
    dst[nsamples * 3] = swab_s24(src[3]);
    dst[nsamples * 4] = swab_s24(src[4]);
    dst[nsamples * 5] = swab_s24(src[5]);

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
    dst[nsamples * 0] = swab_s24(src[0]);
    dst[nsamples * 1] = swab_s24(src[1]);
    dst[nsamples * 2] = swab_s24(src[2]);
    dst[nsamples * 3] = swab_s24(src[3]);
    dst[nsamples * 4] = swab_s24(src[4]);
    dst[nsamples * 5] = swab_s24(src[5]);

    src += nch;
    dst++;
  }

  /////////////////////////////////////////////////////////
  // Fill output chunk

  send_samples(_chunk);
  return true;
}
bool
Converter::pcm32_le_linear_6ch(Chunk *_chunk)
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
      send_empty(_chunk);
      return true;
    }
    else
    {
      // finish & convert incomplete sample 
      memcpy(part_buf + part_size, rawdata, delta);
      drop_rawdata(delta);
      part_size = 0;

      src = (int32_t *)part_buf;
    dst[nsamples * 0] = swab_s32(src[0]);
    dst[nsamples * 1] = swab_s32(src[1]);
    dst[nsamples * 2] = swab_s32(src[2]);
    dst[nsamples * 3] = swab_s32(src[3]);
    dst[nsamples * 4] = swab_s32(src[4]);
    dst[nsamples * 5] = swab_s32(src[5]);

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
    dst[nsamples * 0] = swab_s32(src[0]);
    dst[nsamples * 1] = swab_s32(src[1]);
    dst[nsamples * 2] = swab_s32(src[2]);
    dst[nsamples * 3] = swab_s32(src[3]);
    dst[nsamples * 4] = swab_s32(src[4]);
    dst[nsamples * 5] = swab_s32(src[5]);

    src += nch;
    dst++;
  }

  /////////////////////////////////////////////////////////
  // Fill output chunk

  send_samples(_chunk);
  return true;
}
bool
Converter::pcmfloat_le_linear_6ch(Chunk *_chunk)
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
      send_empty(_chunk);
      return true;
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

  /////////////////////////////////////////////////////////
  // Fill output chunk

  send_samples(_chunk);
  return true;
}

