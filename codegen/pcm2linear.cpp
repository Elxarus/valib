  void pcm16_linear_1ch();
  void pcm24_linear_1ch();
  void pcm32_linear_1ch();
  void pcmfloat_linear_1ch();
  void pcm16_le_linear_1ch();
  void pcm24_le_linear_1ch();
  void pcm32_le_linear_1ch();
  void pcmfloat_le_linear_1ch();

  void pcm16_linear_2ch();
  void pcm24_linear_2ch();
  void pcm32_linear_2ch();
  void pcmfloat_linear_2ch();
  void pcm16_le_linear_2ch();
  void pcm24_le_linear_2ch();
  void pcm32_le_linear_2ch();
  void pcmfloat_le_linear_2ch();

  void pcm16_linear_3ch();
  void pcm24_linear_3ch();
  void pcm32_linear_3ch();
  void pcmfloat_linear_3ch();
  void pcm16_le_linear_3ch();
  void pcm24_le_linear_3ch();
  void pcm32_le_linear_3ch();
  void pcmfloat_le_linear_3ch();

  void pcm16_linear_4ch();
  void pcm24_linear_4ch();
  void pcm32_linear_4ch();
  void pcmfloat_linear_4ch();
  void pcm16_le_linear_4ch();
  void pcm24_le_linear_4ch();
  void pcm32_le_linear_4ch();
  void pcmfloat_le_linear_4ch();

  void pcm16_linear_5ch();
  void pcm24_linear_5ch();
  void pcm32_linear_5ch();
  void pcmfloat_linear_5ch();
  void pcm16_le_linear_5ch();
  void pcm24_le_linear_5ch();
  void pcm32_le_linear_5ch();
  void pcmfloat_le_linear_5ch();

  void pcm16_linear_6ch();
  void pcm24_linear_6ch();
  void pcm32_linear_6ch();
  void pcmfloat_linear_6ch();
  void pcm16_le_linear_6ch();
  void pcm24_le_linear_6ch();
  void pcm32_le_linear_6ch();
  void pcmfloat_le_linear_6ch();

typedef void (Converter::*convert_t)();

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

void
Converter::pcm16_linear_1ch()
{
  // input: spk, nsamples, rawdata, size
  // output: out_samples, out_size
  // intermediate: part_buf, part_size

  const int nch = 1;
  const size_t sample_size = sizeof(int16_t) * 1;

  int16_t *src;
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

      src = (int16_t *)part_size;

    dst[nsamples * 0] = sample_t(src[0]);


      dst++;
      out_size++;
    }   
  }

  /////////////////////////////////////////////////////////
  // Find end of buffer & remember remaining part of sample

  src = (int16_t *)rawdata;
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

      src = (int24_t *)part_size;

    dst[nsamples * 0] = sample_t(src[0]);


      dst++;
      out_size++;
    }   
  }

  /////////////////////////////////////////////////////////
  // Find end of buffer & remember remaining part of sample

  src = (int24_t *)rawdata;
  int24_t *end;

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

      src = (int32_t *)part_size;

    dst[nsamples * 0] = sample_t(src[0]);


      dst++;
      out_size++;
    }   
  }

  /////////////////////////////////////////////////////////
  // Find end of buffer & remember remaining part of sample

  src = (int32_t *)rawdata;
  int32_t *end;

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

      src = (float *)part_size;

    dst[nsamples * 0] = sample_t(src[0]);


      dst++;
      out_size++;
    }   
  }

  /////////////////////////////////////////////////////////
  // Find end of buffer & remember remaining part of sample

  src = (float *)rawdata;
  float *end;

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

      src = (int16_t *)part_size;

    dst[nsamples * 0] = swab16(src[0]);


      dst++;
      out_size++;
    }   
  }

  /////////////////////////////////////////////////////////
  // Find end of buffer & remember remaining part of sample

  src = (int16_t *)rawdata;
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

      src = (int24_t *)part_size;

    dst[nsamples * 0] = swab24(src[0]);


      dst++;
      out_size++;
    }   
  }

  /////////////////////////////////////////////////////////
  // Find end of buffer & remember remaining part of sample

  src = (int24_t *)rawdata;
  int24_t *end;

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

      src = (int32_t *)part_size;

    dst[nsamples * 0] = swab32(src[0]);


      dst++;
      out_size++;
    }   
  }

  /////////////////////////////////////////////////////////
  // Find end of buffer & remember remaining part of sample

  src = (int32_t *)rawdata;
  int32_t *end;

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

      src = (float *)part_size;

    dst[nsamples * 0] = swab_float(src[0]);


      dst++;
      out_size++;
    }   
  }

  /////////////////////////////////////////////////////////
  // Find end of buffer & remember remaining part of sample

  src = (float *)rawdata;
  float *end;

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

      src = (int16_t *)part_size;

    dst[nsamples * 0] = sample_t(src[0]);
    dst[nsamples * 1] = sample_t(src[1]);


      dst++;
      out_size++;
    }   
  }

  /////////////////////////////////////////////////////////
  // Find end of buffer & remember remaining part of sample

  src = (int16_t *)rawdata;
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

      src = (int24_t *)part_size;

    dst[nsamples * 0] = sample_t(src[0]);
    dst[nsamples * 1] = sample_t(src[1]);


      dst++;
      out_size++;
    }   
  }

  /////////////////////////////////////////////////////////
  // Find end of buffer & remember remaining part of sample

  src = (int24_t *)rawdata;
  int24_t *end;

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

      src = (int32_t *)part_size;

    dst[nsamples * 0] = sample_t(src[0]);
    dst[nsamples * 1] = sample_t(src[1]);


      dst++;
      out_size++;
    }   
  }

  /////////////////////////////////////////////////////////
  // Find end of buffer & remember remaining part of sample

  src = (int32_t *)rawdata;
  int32_t *end;

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

      src = (float *)part_size;

    dst[nsamples * 0] = sample_t(src[0]);
    dst[nsamples * 1] = sample_t(src[1]);


      dst++;
      out_size++;
    }   
  }

  /////////////////////////////////////////////////////////
  // Find end of buffer & remember remaining part of sample

  src = (float *)rawdata;
  float *end;

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

      src = (int16_t *)part_size;

    dst[nsamples * 0] = swab16(src[0]);
    dst[nsamples * 1] = swab16(src[1]);


      dst++;
      out_size++;
    }   
  }

  /////////////////////////////////////////////////////////
  // Find end of buffer & remember remaining part of sample

  src = (int16_t *)rawdata;
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

      src = (int24_t *)part_size;

    dst[nsamples * 0] = swab24(src[0]);
    dst[nsamples * 1] = swab24(src[1]);


      dst++;
      out_size++;
    }   
  }

  /////////////////////////////////////////////////////////
  // Find end of buffer & remember remaining part of sample

  src = (int24_t *)rawdata;
  int24_t *end;

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

      src = (int32_t *)part_size;

    dst[nsamples * 0] = swab32(src[0]);
    dst[nsamples * 1] = swab32(src[1]);


      dst++;
      out_size++;
    }   
  }

  /////////////////////////////////////////////////////////
  // Find end of buffer & remember remaining part of sample

  src = (int32_t *)rawdata;
  int32_t *end;

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

      src = (float *)part_size;

    dst[nsamples * 0] = swab_float(src[0]);
    dst[nsamples * 1] = swab_float(src[1]);


      dst++;
      out_size++;
    }   
  }

  /////////////////////////////////////////////////////////
  // Find end of buffer & remember remaining part of sample

  src = (float *)rawdata;
  float *end;

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

      src = (int16_t *)part_size;

    dst[nsamples * 0] = sample_t(src[0]);
    dst[nsamples * 1] = sample_t(src[1]);
    dst[nsamples * 2] = sample_t(src[2]);


      dst++;
      out_size++;
    }   
  }

  /////////////////////////////////////////////////////////
  // Find end of buffer & remember remaining part of sample

  src = (int16_t *)rawdata;
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

      src = (int24_t *)part_size;

    dst[nsamples * 0] = sample_t(src[0]);
    dst[nsamples * 1] = sample_t(src[1]);
    dst[nsamples * 2] = sample_t(src[2]);


      dst++;
      out_size++;
    }   
  }

  /////////////////////////////////////////////////////////
  // Find end of buffer & remember remaining part of sample

  src = (int24_t *)rawdata;
  int24_t *end;

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

      src = (int32_t *)part_size;

    dst[nsamples * 0] = sample_t(src[0]);
    dst[nsamples * 1] = sample_t(src[1]);
    dst[nsamples * 2] = sample_t(src[2]);


      dst++;
      out_size++;
    }   
  }

  /////////////////////////////////////////////////////////
  // Find end of buffer & remember remaining part of sample

  src = (int32_t *)rawdata;
  int32_t *end;

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

      src = (float *)part_size;

    dst[nsamples * 0] = sample_t(src[0]);
    dst[nsamples * 1] = sample_t(src[1]);
    dst[nsamples * 2] = sample_t(src[2]);


      dst++;
      out_size++;
    }   
  }

  /////////////////////////////////////////////////////////
  // Find end of buffer & remember remaining part of sample

  src = (float *)rawdata;
  float *end;

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

      src = (int16_t *)part_size;

    dst[nsamples * 0] = swab16(src[0]);
    dst[nsamples * 1] = swab16(src[1]);
    dst[nsamples * 2] = swab16(src[2]);


      dst++;
      out_size++;
    }   
  }

  /////////////////////////////////////////////////////////
  // Find end of buffer & remember remaining part of sample

  src = (int16_t *)rawdata;
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

      src = (int24_t *)part_size;

    dst[nsamples * 0] = swab24(src[0]);
    dst[nsamples * 1] = swab24(src[1]);
    dst[nsamples * 2] = swab24(src[2]);


      dst++;
      out_size++;
    }   
  }

  /////////////////////////////////////////////////////////
  // Find end of buffer & remember remaining part of sample

  src = (int24_t *)rawdata;
  int24_t *end;

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

      src = (int32_t *)part_size;

    dst[nsamples * 0] = swab32(src[0]);
    dst[nsamples * 1] = swab32(src[1]);
    dst[nsamples * 2] = swab32(src[2]);


      dst++;
      out_size++;
    }   
  }

  /////////////////////////////////////////////////////////
  // Find end of buffer & remember remaining part of sample

  src = (int32_t *)rawdata;
  int32_t *end;

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

      src = (float *)part_size;

    dst[nsamples * 0] = swab_float(src[0]);
    dst[nsamples * 1] = swab_float(src[1]);
    dst[nsamples * 2] = swab_float(src[2]);


      dst++;
      out_size++;
    }   
  }

  /////////////////////////////////////////////////////////
  // Find end of buffer & remember remaining part of sample

  src = (float *)rawdata;
  float *end;

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

      src = (int16_t *)part_size;

    dst[nsamples * 0] = sample_t(src[0]);
    dst[nsamples * 1] = sample_t(src[1]);
    dst[nsamples * 2] = sample_t(src[2]);
    dst[nsamples * 3] = sample_t(src[3]);


      dst++;
      out_size++;
    }   
  }

  /////////////////////////////////////////////////////////
  // Find end of buffer & remember remaining part of sample

  src = (int16_t *)rawdata;
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

      src = (int24_t *)part_size;

    dst[nsamples * 0] = sample_t(src[0]);
    dst[nsamples * 1] = sample_t(src[1]);
    dst[nsamples * 2] = sample_t(src[2]);
    dst[nsamples * 3] = sample_t(src[3]);


      dst++;
      out_size++;
    }   
  }

  /////////////////////////////////////////////////////////
  // Find end of buffer & remember remaining part of sample

  src = (int24_t *)rawdata;
  int24_t *end;

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

      src = (int32_t *)part_size;

    dst[nsamples * 0] = sample_t(src[0]);
    dst[nsamples * 1] = sample_t(src[1]);
    dst[nsamples * 2] = sample_t(src[2]);
    dst[nsamples * 3] = sample_t(src[3]);


      dst++;
      out_size++;
    }   
  }

  /////////////////////////////////////////////////////////
  // Find end of buffer & remember remaining part of sample

  src = (int32_t *)rawdata;
  int32_t *end;

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

      src = (float *)part_size;

    dst[nsamples * 0] = sample_t(src[0]);
    dst[nsamples * 1] = sample_t(src[1]);
    dst[nsamples * 2] = sample_t(src[2]);
    dst[nsamples * 3] = sample_t(src[3]);


      dst++;
      out_size++;
    }   
  }

  /////////////////////////////////////////////////////////
  // Find end of buffer & remember remaining part of sample

  src = (float *)rawdata;
  float *end;

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

      src = (int16_t *)part_size;

    dst[nsamples * 0] = swab16(src[0]);
    dst[nsamples * 1] = swab16(src[1]);
    dst[nsamples * 2] = swab16(src[2]);
    dst[nsamples * 3] = swab16(src[3]);


      dst++;
      out_size++;
    }   
  }

  /////////////////////////////////////////////////////////
  // Find end of buffer & remember remaining part of sample

  src = (int16_t *)rawdata;
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

      src = (int24_t *)part_size;

    dst[nsamples * 0] = swab24(src[0]);
    dst[nsamples * 1] = swab24(src[1]);
    dst[nsamples * 2] = swab24(src[2]);
    dst[nsamples * 3] = swab24(src[3]);


      dst++;
      out_size++;
    }   
  }

  /////////////////////////////////////////////////////////
  // Find end of buffer & remember remaining part of sample

  src = (int24_t *)rawdata;
  int24_t *end;

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

      src = (int32_t *)part_size;

    dst[nsamples * 0] = swab32(src[0]);
    dst[nsamples * 1] = swab32(src[1]);
    dst[nsamples * 2] = swab32(src[2]);
    dst[nsamples * 3] = swab32(src[3]);


      dst++;
      out_size++;
    }   
  }

  /////////////////////////////////////////////////////////
  // Find end of buffer & remember remaining part of sample

  src = (int32_t *)rawdata;
  int32_t *end;

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

      src = (float *)part_size;

    dst[nsamples * 0] = swab_float(src[0]);
    dst[nsamples * 1] = swab_float(src[1]);
    dst[nsamples * 2] = swab_float(src[2]);
    dst[nsamples * 3] = swab_float(src[3]);


      dst++;
      out_size++;
    }   
  }

  /////////////////////////////////////////////////////////
  // Find end of buffer & remember remaining part of sample

  src = (float *)rawdata;
  float *end;

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

      src = (int16_t *)part_size;

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
  // Find end of buffer & remember remaining part of sample

  src = (int16_t *)rawdata;
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

      src = (int24_t *)part_size;

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
  // Find end of buffer & remember remaining part of sample

  src = (int24_t *)rawdata;
  int24_t *end;

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

      src = (int32_t *)part_size;

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
  // Find end of buffer & remember remaining part of sample

  src = (int32_t *)rawdata;
  int32_t *end;

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

      src = (float *)part_size;

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
  // Find end of buffer & remember remaining part of sample

  src = (float *)rawdata;
  float *end;

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

      src = (int16_t *)part_size;

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
  // Find end of buffer & remember remaining part of sample

  src = (int16_t *)rawdata;
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

      src = (int24_t *)part_size;

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
  // Find end of buffer & remember remaining part of sample

  src = (int24_t *)rawdata;
  int24_t *end;

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

      src = (int32_t *)part_size;

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
  // Find end of buffer & remember remaining part of sample

  src = (int32_t *)rawdata;
  int32_t *end;

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

      src = (float *)part_size;

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
  // Find end of buffer & remember remaining part of sample

  src = (float *)rawdata;
  float *end;

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

      src = (int16_t *)part_size;

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
  // Find end of buffer & remember remaining part of sample

  src = (int16_t *)rawdata;
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

      src = (int24_t *)part_size;

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
  // Find end of buffer & remember remaining part of sample

  src = (int24_t *)rawdata;
  int24_t *end;

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

      src = (int32_t *)part_size;

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
  // Find end of buffer & remember remaining part of sample

  src = (int32_t *)rawdata;
  int32_t *end;

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

      src = (float *)part_size;

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
  // Find end of buffer & remember remaining part of sample

  src = (float *)rawdata;
  float *end;

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

      src = (int16_t *)part_size;

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
  // Find end of buffer & remember remaining part of sample

  src = (int16_t *)rawdata;
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

      src = (int24_t *)part_size;

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
  // Find end of buffer & remember remaining part of sample

  src = (int24_t *)rawdata;
  int24_t *end;

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

      src = (int32_t *)part_size;

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
  // Find end of buffer & remember remaining part of sample

  src = (int32_t *)rawdata;
  int32_t *end;

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

      src = (float *)part_size;

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
  // Find end of buffer & remember remaining part of sample

  src = (float *)rawdata;
  float *end;

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


