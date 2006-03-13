  void linear_pcm16_1ch();
  void linear_pcm24_1ch();
  void linear_pcm32_1ch();
  void linear_pcm16_be_1ch();
  void linear_pcm24_be_1ch();
  void linear_pcm32_be_1ch();
  void linear_pcmfloat_1ch();

  void linear_pcm16_2ch();
  void linear_pcm24_2ch();
  void linear_pcm32_2ch();
  void linear_pcm16_be_2ch();
  void linear_pcm24_be_2ch();
  void linear_pcm32_be_2ch();
  void linear_pcmfloat_2ch();

  void linear_pcm16_3ch();
  void linear_pcm24_3ch();
  void linear_pcm32_3ch();
  void linear_pcm16_be_3ch();
  void linear_pcm24_be_3ch();
  void linear_pcm32_be_3ch();
  void linear_pcmfloat_3ch();

  void linear_pcm16_4ch();
  void linear_pcm24_4ch();
  void linear_pcm32_4ch();
  void linear_pcm16_be_4ch();
  void linear_pcm24_be_4ch();
  void linear_pcm32_be_4ch();
  void linear_pcmfloat_4ch();

  void linear_pcm16_5ch();
  void linear_pcm24_5ch();
  void linear_pcm32_5ch();
  void linear_pcm16_be_5ch();
  void linear_pcm24_be_5ch();
  void linear_pcm32_be_5ch();
  void linear_pcmfloat_5ch();

  void linear_pcm16_6ch();
  void linear_pcm24_6ch();
  void linear_pcm32_6ch();
  void linear_pcm16_be_6ch();
  void linear_pcm24_be_6ch();
  void linear_pcm32_be_6ch();
  void linear_pcmfloat_6ch();

typedef void (Converter::*convert_t)();

static const int formats_tbl[] = { FORMAT_PCM16, FORMAT_PCM24, FORMAT_PCM32, FORMAT_PCM16_BE, FORMAT_PCM24_BE, FORMAT_PCM32_BE, FORMAT_PCMFLOAT };

static const int formats = FORMAT_PCM16 | FORMAT_PCM24 | FORMAT_PCM32 | FORMAT_PCM16_BE | FORMAT_PCM24_BE | FORMAT_PCM32_BE | FORMAT_PCMFLOAT;

static const convert_t linear2pcm_tbl[NCHANNELS][7] = {
 { &Converter::linear_pcm16_1ch, &Converter::linear_pcm24_1ch, &Converter::linear_pcm32_1ch, &Converter::linear_pcm16_be_1ch, &Converter::linear_pcm24_be_1ch, &Converter::linear_pcm32_be_1ch, &Converter::linear_pcmfloat_1ch },
 { &Converter::linear_pcm16_2ch, &Converter::linear_pcm24_2ch, &Converter::linear_pcm32_2ch, &Converter::linear_pcm16_be_2ch, &Converter::linear_pcm24_be_2ch, &Converter::linear_pcm32_be_2ch, &Converter::linear_pcmfloat_2ch },
 { &Converter::linear_pcm16_3ch, &Converter::linear_pcm24_3ch, &Converter::linear_pcm32_3ch, &Converter::linear_pcm16_be_3ch, &Converter::linear_pcm24_be_3ch, &Converter::linear_pcm32_be_3ch, &Converter::linear_pcmfloat_3ch },
 { &Converter::linear_pcm16_4ch, &Converter::linear_pcm24_4ch, &Converter::linear_pcm32_4ch, &Converter::linear_pcm16_be_4ch, &Converter::linear_pcm24_be_4ch, &Converter::linear_pcm32_be_4ch, &Converter::linear_pcmfloat_4ch },
 { &Converter::linear_pcm16_5ch, &Converter::linear_pcm24_5ch, &Converter::linear_pcm32_5ch, &Converter::linear_pcm16_be_5ch, &Converter::linear_pcm24_be_5ch, &Converter::linear_pcm32_be_5ch, &Converter::linear_pcmfloat_5ch },
 { &Converter::linear_pcm16_6ch, &Converter::linear_pcm24_6ch, &Converter::linear_pcm32_6ch, &Converter::linear_pcm16_be_6ch, &Converter::linear_pcm24_be_6ch, &Converter::linear_pcm32_be_6ch, &Converter::linear_pcmfloat_6ch },
};

void
Converter::linear_pcm16_1ch()
{
  const int nch = 1;
  const size_t sample_size = sizeof(int16_t) * 1;

  samples_t src = samples;
  int16_t *dst = (int16_t *)out_rawdata;
  size_t n = MIN(size, nsamples);
  drop_samples(n);
  out_size = n * sample_size;

  while (n--)
  {
    dst[0] = int2le16(s2i16(src[0][0])); src[0]++;

    dst += nch;
  }
}
void
Converter::linear_pcm24_1ch()
{
  const int nch = 1;
  const size_t sample_size = sizeof(int24_t) * 1;

  samples_t src = samples;
  int24_t *dst = (int24_t *)out_rawdata;
  size_t n = MIN(size, nsamples);
  drop_samples(n);
  out_size = n * sample_size;

  while (n--)
  {
    dst[0] = int2le24(s2i24(src[0][0])); src[0]++;

    dst += nch;
  }
}
void
Converter::linear_pcm32_1ch()
{
  const int nch = 1;
  const size_t sample_size = sizeof(int32_t) * 1;

  samples_t src = samples;
  int32_t *dst = (int32_t *)out_rawdata;
  size_t n = MIN(size, nsamples);
  drop_samples(n);
  out_size = n * sample_size;

  while (n--)
  {
    dst[0] = int2le32(s2i32(src[0][0])); src[0]++;

    dst += nch;
  }
}
void
Converter::linear_pcm16_be_1ch()
{
  const int nch = 1;
  const size_t sample_size = sizeof(int16_t) * 1;

  samples_t src = samples;
  int16_t *dst = (int16_t *)out_rawdata;
  size_t n = MIN(size, nsamples);
  drop_samples(n);
  out_size = n * sample_size;

  while (n--)
  {
    dst[0] = int2be16(s2i16(src[0][0])); src[0]++;

    dst += nch;
  }
}
void
Converter::linear_pcm24_be_1ch()
{
  const int nch = 1;
  const size_t sample_size = sizeof(int24_t) * 1;

  samples_t src = samples;
  int24_t *dst = (int24_t *)out_rawdata;
  size_t n = MIN(size, nsamples);
  drop_samples(n);
  out_size = n * sample_size;

  while (n--)
  {
    dst[0] = int2be24(s2i24(src[0][0])); src[0]++;

    dst += nch;
  }
}
void
Converter::linear_pcm32_be_1ch()
{
  const int nch = 1;
  const size_t sample_size = sizeof(int32_t) * 1;

  samples_t src = samples;
  int32_t *dst = (int32_t *)out_rawdata;
  size_t n = MIN(size, nsamples);
  drop_samples(n);
  out_size = n * sample_size;

  while (n--)
  {
    dst[0] = int2be32(s2i32(src[0][0])); src[0]++;

    dst += nch;
  }
}
void
Converter::linear_pcmfloat_1ch()
{
  const int nch = 1;
  const size_t sample_size = sizeof(float) * 1;

  samples_t src = samples;
  float *dst = (float *)out_rawdata;
  size_t n = MIN(size, nsamples);
  drop_samples(n);
  out_size = n * sample_size;

  while (n--)
  {
    dst[0] = ((float)(src[0][0])); src[0]++;

    dst += nch;
  }
}

void
Converter::linear_pcm16_2ch()
{
  const int nch = 2;
  const size_t sample_size = sizeof(int16_t) * 2;

  samples_t src = samples;
  int16_t *dst = (int16_t *)out_rawdata;
  size_t n = MIN(size, nsamples);
  drop_samples(n);
  out_size = n * sample_size;

  while (n--)
  {
    dst[0] = int2le16(s2i16(src[0][0])); src[0]++;
    dst[1] = int2le16(s2i16(src[1][0])); src[1]++;

    dst += nch;
  }
}
void
Converter::linear_pcm24_2ch()
{
  const int nch = 2;
  const size_t sample_size = sizeof(int24_t) * 2;

  samples_t src = samples;
  int24_t *dst = (int24_t *)out_rawdata;
  size_t n = MIN(size, nsamples);
  drop_samples(n);
  out_size = n * sample_size;

  while (n--)
  {
    dst[0] = int2le24(s2i24(src[0][0])); src[0]++;
    dst[1] = int2le24(s2i24(src[1][0])); src[1]++;

    dst += nch;
  }
}
void
Converter::linear_pcm32_2ch()
{
  const int nch = 2;
  const size_t sample_size = sizeof(int32_t) * 2;

  samples_t src = samples;
  int32_t *dst = (int32_t *)out_rawdata;
  size_t n = MIN(size, nsamples);
  drop_samples(n);
  out_size = n * sample_size;

  while (n--)
  {
    dst[0] = int2le32(s2i32(src[0][0])); src[0]++;
    dst[1] = int2le32(s2i32(src[1][0])); src[1]++;

    dst += nch;
  }
}
void
Converter::linear_pcm16_be_2ch()
{
  const int nch = 2;
  const size_t sample_size = sizeof(int16_t) * 2;

  samples_t src = samples;
  int16_t *dst = (int16_t *)out_rawdata;
  size_t n = MIN(size, nsamples);
  drop_samples(n);
  out_size = n * sample_size;

  while (n--)
  {
    dst[0] = int2be16(s2i16(src[0][0])); src[0]++;
    dst[1] = int2be16(s2i16(src[1][0])); src[1]++;

    dst += nch;
  }
}
void
Converter::linear_pcm24_be_2ch()
{
  const int nch = 2;
  const size_t sample_size = sizeof(int24_t) * 2;

  samples_t src = samples;
  int24_t *dst = (int24_t *)out_rawdata;
  size_t n = MIN(size, nsamples);
  drop_samples(n);
  out_size = n * sample_size;

  while (n--)
  {
    dst[0] = int2be24(s2i24(src[0][0])); src[0]++;
    dst[1] = int2be24(s2i24(src[1][0])); src[1]++;

    dst += nch;
  }
}
void
Converter::linear_pcm32_be_2ch()
{
  const int nch = 2;
  const size_t sample_size = sizeof(int32_t) * 2;

  samples_t src = samples;
  int32_t *dst = (int32_t *)out_rawdata;
  size_t n = MIN(size, nsamples);
  drop_samples(n);
  out_size = n * sample_size;

  while (n--)
  {
    dst[0] = int2be32(s2i32(src[0][0])); src[0]++;
    dst[1] = int2be32(s2i32(src[1][0])); src[1]++;

    dst += nch;
  }
}
void
Converter::linear_pcmfloat_2ch()
{
  const int nch = 2;
  const size_t sample_size = sizeof(float) * 2;

  samples_t src = samples;
  float *dst = (float *)out_rawdata;
  size_t n = MIN(size, nsamples);
  drop_samples(n);
  out_size = n * sample_size;

  while (n--)
  {
    dst[0] = ((float)(src[0][0])); src[0]++;
    dst[1] = ((float)(src[1][0])); src[1]++;

    dst += nch;
  }
}

void
Converter::linear_pcm16_3ch()
{
  const int nch = 3;
  const size_t sample_size = sizeof(int16_t) * 3;

  samples_t src = samples;
  int16_t *dst = (int16_t *)out_rawdata;
  size_t n = MIN(size, nsamples);
  drop_samples(n);
  out_size = n * sample_size;

  while (n--)
  {
    dst[0] = int2le16(s2i16(src[0][0])); src[0]++;
    dst[1] = int2le16(s2i16(src[1][0])); src[1]++;
    dst[2] = int2le16(s2i16(src[2][0])); src[2]++;

    dst += nch;
  }
}
void
Converter::linear_pcm24_3ch()
{
  const int nch = 3;
  const size_t sample_size = sizeof(int24_t) * 3;

  samples_t src = samples;
  int24_t *dst = (int24_t *)out_rawdata;
  size_t n = MIN(size, nsamples);
  drop_samples(n);
  out_size = n * sample_size;

  while (n--)
  {
    dst[0] = int2le24(s2i24(src[0][0])); src[0]++;
    dst[1] = int2le24(s2i24(src[1][0])); src[1]++;
    dst[2] = int2le24(s2i24(src[2][0])); src[2]++;

    dst += nch;
  }
}
void
Converter::linear_pcm32_3ch()
{
  const int nch = 3;
  const size_t sample_size = sizeof(int32_t) * 3;

  samples_t src = samples;
  int32_t *dst = (int32_t *)out_rawdata;
  size_t n = MIN(size, nsamples);
  drop_samples(n);
  out_size = n * sample_size;

  while (n--)
  {
    dst[0] = int2le32(s2i32(src[0][0])); src[0]++;
    dst[1] = int2le32(s2i32(src[1][0])); src[1]++;
    dst[2] = int2le32(s2i32(src[2][0])); src[2]++;

    dst += nch;
  }
}
void
Converter::linear_pcm16_be_3ch()
{
  const int nch = 3;
  const size_t sample_size = sizeof(int16_t) * 3;

  samples_t src = samples;
  int16_t *dst = (int16_t *)out_rawdata;
  size_t n = MIN(size, nsamples);
  drop_samples(n);
  out_size = n * sample_size;

  while (n--)
  {
    dst[0] = int2be16(s2i16(src[0][0])); src[0]++;
    dst[1] = int2be16(s2i16(src[1][0])); src[1]++;
    dst[2] = int2be16(s2i16(src[2][0])); src[2]++;

    dst += nch;
  }
}
void
Converter::linear_pcm24_be_3ch()
{
  const int nch = 3;
  const size_t sample_size = sizeof(int24_t) * 3;

  samples_t src = samples;
  int24_t *dst = (int24_t *)out_rawdata;
  size_t n = MIN(size, nsamples);
  drop_samples(n);
  out_size = n * sample_size;

  while (n--)
  {
    dst[0] = int2be24(s2i24(src[0][0])); src[0]++;
    dst[1] = int2be24(s2i24(src[1][0])); src[1]++;
    dst[2] = int2be24(s2i24(src[2][0])); src[2]++;

    dst += nch;
  }
}
void
Converter::linear_pcm32_be_3ch()
{
  const int nch = 3;
  const size_t sample_size = sizeof(int32_t) * 3;

  samples_t src = samples;
  int32_t *dst = (int32_t *)out_rawdata;
  size_t n = MIN(size, nsamples);
  drop_samples(n);
  out_size = n * sample_size;

  while (n--)
  {
    dst[0] = int2be32(s2i32(src[0][0])); src[0]++;
    dst[1] = int2be32(s2i32(src[1][0])); src[1]++;
    dst[2] = int2be32(s2i32(src[2][0])); src[2]++;

    dst += nch;
  }
}
void
Converter::linear_pcmfloat_3ch()
{
  const int nch = 3;
  const size_t sample_size = sizeof(float) * 3;

  samples_t src = samples;
  float *dst = (float *)out_rawdata;
  size_t n = MIN(size, nsamples);
  drop_samples(n);
  out_size = n * sample_size;

  while (n--)
  {
    dst[0] = ((float)(src[0][0])); src[0]++;
    dst[1] = ((float)(src[1][0])); src[1]++;
    dst[2] = ((float)(src[2][0])); src[2]++;

    dst += nch;
  }
}

void
Converter::linear_pcm16_4ch()
{
  const int nch = 4;
  const size_t sample_size = sizeof(int16_t) * 4;

  samples_t src = samples;
  int16_t *dst = (int16_t *)out_rawdata;
  size_t n = MIN(size, nsamples);
  drop_samples(n);
  out_size = n * sample_size;

  while (n--)
  {
    dst[0] = int2le16(s2i16(src[0][0])); src[0]++;
    dst[1] = int2le16(s2i16(src[1][0])); src[1]++;
    dst[2] = int2le16(s2i16(src[2][0])); src[2]++;
    dst[3] = int2le16(s2i16(src[3][0])); src[3]++;

    dst += nch;
  }
}
void
Converter::linear_pcm24_4ch()
{
  const int nch = 4;
  const size_t sample_size = sizeof(int24_t) * 4;

  samples_t src = samples;
  int24_t *dst = (int24_t *)out_rawdata;
  size_t n = MIN(size, nsamples);
  drop_samples(n);
  out_size = n * sample_size;

  while (n--)
  {
    dst[0] = int2le24(s2i24(src[0][0])); src[0]++;
    dst[1] = int2le24(s2i24(src[1][0])); src[1]++;
    dst[2] = int2le24(s2i24(src[2][0])); src[2]++;
    dst[3] = int2le24(s2i24(src[3][0])); src[3]++;

    dst += nch;
  }
}
void
Converter::linear_pcm32_4ch()
{
  const int nch = 4;
  const size_t sample_size = sizeof(int32_t) * 4;

  samples_t src = samples;
  int32_t *dst = (int32_t *)out_rawdata;
  size_t n = MIN(size, nsamples);
  drop_samples(n);
  out_size = n * sample_size;

  while (n--)
  {
    dst[0] = int2le32(s2i32(src[0][0])); src[0]++;
    dst[1] = int2le32(s2i32(src[1][0])); src[1]++;
    dst[2] = int2le32(s2i32(src[2][0])); src[2]++;
    dst[3] = int2le32(s2i32(src[3][0])); src[3]++;

    dst += nch;
  }
}
void
Converter::linear_pcm16_be_4ch()
{
  const int nch = 4;
  const size_t sample_size = sizeof(int16_t) * 4;

  samples_t src = samples;
  int16_t *dst = (int16_t *)out_rawdata;
  size_t n = MIN(size, nsamples);
  drop_samples(n);
  out_size = n * sample_size;

  while (n--)
  {
    dst[0] = int2be16(s2i16(src[0][0])); src[0]++;
    dst[1] = int2be16(s2i16(src[1][0])); src[1]++;
    dst[2] = int2be16(s2i16(src[2][0])); src[2]++;
    dst[3] = int2be16(s2i16(src[3][0])); src[3]++;

    dst += nch;
  }
}
void
Converter::linear_pcm24_be_4ch()
{
  const int nch = 4;
  const size_t sample_size = sizeof(int24_t) * 4;

  samples_t src = samples;
  int24_t *dst = (int24_t *)out_rawdata;
  size_t n = MIN(size, nsamples);
  drop_samples(n);
  out_size = n * sample_size;

  while (n--)
  {
    dst[0] = int2be24(s2i24(src[0][0])); src[0]++;
    dst[1] = int2be24(s2i24(src[1][0])); src[1]++;
    dst[2] = int2be24(s2i24(src[2][0])); src[2]++;
    dst[3] = int2be24(s2i24(src[3][0])); src[3]++;

    dst += nch;
  }
}
void
Converter::linear_pcm32_be_4ch()
{
  const int nch = 4;
  const size_t sample_size = sizeof(int32_t) * 4;

  samples_t src = samples;
  int32_t *dst = (int32_t *)out_rawdata;
  size_t n = MIN(size, nsamples);
  drop_samples(n);
  out_size = n * sample_size;

  while (n--)
  {
    dst[0] = int2be32(s2i32(src[0][0])); src[0]++;
    dst[1] = int2be32(s2i32(src[1][0])); src[1]++;
    dst[2] = int2be32(s2i32(src[2][0])); src[2]++;
    dst[3] = int2be32(s2i32(src[3][0])); src[3]++;

    dst += nch;
  }
}
void
Converter::linear_pcmfloat_4ch()
{
  const int nch = 4;
  const size_t sample_size = sizeof(float) * 4;

  samples_t src = samples;
  float *dst = (float *)out_rawdata;
  size_t n = MIN(size, nsamples);
  drop_samples(n);
  out_size = n * sample_size;

  while (n--)
  {
    dst[0] = ((float)(src[0][0])); src[0]++;
    dst[1] = ((float)(src[1][0])); src[1]++;
    dst[2] = ((float)(src[2][0])); src[2]++;
    dst[3] = ((float)(src[3][0])); src[3]++;

    dst += nch;
  }
}

void
Converter::linear_pcm16_5ch()
{
  const int nch = 5;
  const size_t sample_size = sizeof(int16_t) * 5;

  samples_t src = samples;
  int16_t *dst = (int16_t *)out_rawdata;
  size_t n = MIN(size, nsamples);
  drop_samples(n);
  out_size = n * sample_size;

  while (n--)
  {
    dst[0] = int2le16(s2i16(src[0][0])); src[0]++;
    dst[1] = int2le16(s2i16(src[1][0])); src[1]++;
    dst[2] = int2le16(s2i16(src[2][0])); src[2]++;
    dst[3] = int2le16(s2i16(src[3][0])); src[3]++;
    dst[4] = int2le16(s2i16(src[4][0])); src[4]++;

    dst += nch;
  }
}
void
Converter::linear_pcm24_5ch()
{
  const int nch = 5;
  const size_t sample_size = sizeof(int24_t) * 5;

  samples_t src = samples;
  int24_t *dst = (int24_t *)out_rawdata;
  size_t n = MIN(size, nsamples);
  drop_samples(n);
  out_size = n * sample_size;

  while (n--)
  {
    dst[0] = int2le24(s2i24(src[0][0])); src[0]++;
    dst[1] = int2le24(s2i24(src[1][0])); src[1]++;
    dst[2] = int2le24(s2i24(src[2][0])); src[2]++;
    dst[3] = int2le24(s2i24(src[3][0])); src[3]++;
    dst[4] = int2le24(s2i24(src[4][0])); src[4]++;

    dst += nch;
  }
}
void
Converter::linear_pcm32_5ch()
{
  const int nch = 5;
  const size_t sample_size = sizeof(int32_t) * 5;

  samples_t src = samples;
  int32_t *dst = (int32_t *)out_rawdata;
  size_t n = MIN(size, nsamples);
  drop_samples(n);
  out_size = n * sample_size;

  while (n--)
  {
    dst[0] = int2le32(s2i32(src[0][0])); src[0]++;
    dst[1] = int2le32(s2i32(src[1][0])); src[1]++;
    dst[2] = int2le32(s2i32(src[2][0])); src[2]++;
    dst[3] = int2le32(s2i32(src[3][0])); src[3]++;
    dst[4] = int2le32(s2i32(src[4][0])); src[4]++;

    dst += nch;
  }
}
void
Converter::linear_pcm16_be_5ch()
{
  const int nch = 5;
  const size_t sample_size = sizeof(int16_t) * 5;

  samples_t src = samples;
  int16_t *dst = (int16_t *)out_rawdata;
  size_t n = MIN(size, nsamples);
  drop_samples(n);
  out_size = n * sample_size;

  while (n--)
  {
    dst[0] = int2be16(s2i16(src[0][0])); src[0]++;
    dst[1] = int2be16(s2i16(src[1][0])); src[1]++;
    dst[2] = int2be16(s2i16(src[2][0])); src[2]++;
    dst[3] = int2be16(s2i16(src[3][0])); src[3]++;
    dst[4] = int2be16(s2i16(src[4][0])); src[4]++;

    dst += nch;
  }
}
void
Converter::linear_pcm24_be_5ch()
{
  const int nch = 5;
  const size_t sample_size = sizeof(int24_t) * 5;

  samples_t src = samples;
  int24_t *dst = (int24_t *)out_rawdata;
  size_t n = MIN(size, nsamples);
  drop_samples(n);
  out_size = n * sample_size;

  while (n--)
  {
    dst[0] = int2be24(s2i24(src[0][0])); src[0]++;
    dst[1] = int2be24(s2i24(src[1][0])); src[1]++;
    dst[2] = int2be24(s2i24(src[2][0])); src[2]++;
    dst[3] = int2be24(s2i24(src[3][0])); src[3]++;
    dst[4] = int2be24(s2i24(src[4][0])); src[4]++;

    dst += nch;
  }
}
void
Converter::linear_pcm32_be_5ch()
{
  const int nch = 5;
  const size_t sample_size = sizeof(int32_t) * 5;

  samples_t src = samples;
  int32_t *dst = (int32_t *)out_rawdata;
  size_t n = MIN(size, nsamples);
  drop_samples(n);
  out_size = n * sample_size;

  while (n--)
  {
    dst[0] = int2be32(s2i32(src[0][0])); src[0]++;
    dst[1] = int2be32(s2i32(src[1][0])); src[1]++;
    dst[2] = int2be32(s2i32(src[2][0])); src[2]++;
    dst[3] = int2be32(s2i32(src[3][0])); src[3]++;
    dst[4] = int2be32(s2i32(src[4][0])); src[4]++;

    dst += nch;
  }
}
void
Converter::linear_pcmfloat_5ch()
{
  const int nch = 5;
  const size_t sample_size = sizeof(float) * 5;

  samples_t src = samples;
  float *dst = (float *)out_rawdata;
  size_t n = MIN(size, nsamples);
  drop_samples(n);
  out_size = n * sample_size;

  while (n--)
  {
    dst[0] = ((float)(src[0][0])); src[0]++;
    dst[1] = ((float)(src[1][0])); src[1]++;
    dst[2] = ((float)(src[2][0])); src[2]++;
    dst[3] = ((float)(src[3][0])); src[3]++;
    dst[4] = ((float)(src[4][0])); src[4]++;

    dst += nch;
  }
}

void
Converter::linear_pcm16_6ch()
{
  const int nch = 6;
  const size_t sample_size = sizeof(int16_t) * 6;

  samples_t src = samples;
  int16_t *dst = (int16_t *)out_rawdata;
  size_t n = MIN(size, nsamples);
  drop_samples(n);
  out_size = n * sample_size;

  while (n--)
  {
    dst[0] = int2le16(s2i16(src[0][0])); src[0]++;
    dst[1] = int2le16(s2i16(src[1][0])); src[1]++;
    dst[2] = int2le16(s2i16(src[2][0])); src[2]++;
    dst[3] = int2le16(s2i16(src[3][0])); src[3]++;
    dst[4] = int2le16(s2i16(src[4][0])); src[4]++;
    dst[5] = int2le16(s2i16(src[5][0])); src[5]++;

    dst += nch;
  }
}
void
Converter::linear_pcm24_6ch()
{
  const int nch = 6;
  const size_t sample_size = sizeof(int24_t) * 6;

  samples_t src = samples;
  int24_t *dst = (int24_t *)out_rawdata;
  size_t n = MIN(size, nsamples);
  drop_samples(n);
  out_size = n * sample_size;

  while (n--)
  {
    dst[0] = int2le24(s2i24(src[0][0])); src[0]++;
    dst[1] = int2le24(s2i24(src[1][0])); src[1]++;
    dst[2] = int2le24(s2i24(src[2][0])); src[2]++;
    dst[3] = int2le24(s2i24(src[3][0])); src[3]++;
    dst[4] = int2le24(s2i24(src[4][0])); src[4]++;
    dst[5] = int2le24(s2i24(src[5][0])); src[5]++;

    dst += nch;
  }
}
void
Converter::linear_pcm32_6ch()
{
  const int nch = 6;
  const size_t sample_size = sizeof(int32_t) * 6;

  samples_t src = samples;
  int32_t *dst = (int32_t *)out_rawdata;
  size_t n = MIN(size, nsamples);
  drop_samples(n);
  out_size = n * sample_size;

  while (n--)
  {
    dst[0] = int2le32(s2i32(src[0][0])); src[0]++;
    dst[1] = int2le32(s2i32(src[1][0])); src[1]++;
    dst[2] = int2le32(s2i32(src[2][0])); src[2]++;
    dst[3] = int2le32(s2i32(src[3][0])); src[3]++;
    dst[4] = int2le32(s2i32(src[4][0])); src[4]++;
    dst[5] = int2le32(s2i32(src[5][0])); src[5]++;

    dst += nch;
  }
}
void
Converter::linear_pcm16_be_6ch()
{
  const int nch = 6;
  const size_t sample_size = sizeof(int16_t) * 6;

  samples_t src = samples;
  int16_t *dst = (int16_t *)out_rawdata;
  size_t n = MIN(size, nsamples);
  drop_samples(n);
  out_size = n * sample_size;

  while (n--)
  {
    dst[0] = int2be16(s2i16(src[0][0])); src[0]++;
    dst[1] = int2be16(s2i16(src[1][0])); src[1]++;
    dst[2] = int2be16(s2i16(src[2][0])); src[2]++;
    dst[3] = int2be16(s2i16(src[3][0])); src[3]++;
    dst[4] = int2be16(s2i16(src[4][0])); src[4]++;
    dst[5] = int2be16(s2i16(src[5][0])); src[5]++;

    dst += nch;
  }
}
void
Converter::linear_pcm24_be_6ch()
{
  const int nch = 6;
  const size_t sample_size = sizeof(int24_t) * 6;

  samples_t src = samples;
  int24_t *dst = (int24_t *)out_rawdata;
  size_t n = MIN(size, nsamples);
  drop_samples(n);
  out_size = n * sample_size;

  while (n--)
  {
    dst[0] = int2be24(s2i24(src[0][0])); src[0]++;
    dst[1] = int2be24(s2i24(src[1][0])); src[1]++;
    dst[2] = int2be24(s2i24(src[2][0])); src[2]++;
    dst[3] = int2be24(s2i24(src[3][0])); src[3]++;
    dst[4] = int2be24(s2i24(src[4][0])); src[4]++;
    dst[5] = int2be24(s2i24(src[5][0])); src[5]++;

    dst += nch;
  }
}
void
Converter::linear_pcm32_be_6ch()
{
  const int nch = 6;
  const size_t sample_size = sizeof(int32_t) * 6;

  samples_t src = samples;
  int32_t *dst = (int32_t *)out_rawdata;
  size_t n = MIN(size, nsamples);
  drop_samples(n);
  out_size = n * sample_size;

  while (n--)
  {
    dst[0] = int2be32(s2i32(src[0][0])); src[0]++;
    dst[1] = int2be32(s2i32(src[1][0])); src[1]++;
    dst[2] = int2be32(s2i32(src[2][0])); src[2]++;
    dst[3] = int2be32(s2i32(src[3][0])); src[3]++;
    dst[4] = int2be32(s2i32(src[4][0])); src[4]++;
    dst[5] = int2be32(s2i32(src[5][0])); src[5]++;

    dst += nch;
  }
}
void
Converter::linear_pcmfloat_6ch()
{
  const int nch = 6;
  const size_t sample_size = sizeof(float) * 6;

  samples_t src = samples;
  float *dst = (float *)out_rawdata;
  size_t n = MIN(size, nsamples);
  drop_samples(n);
  out_size = n * sample_size;

  while (n--)
  {
    dst[0] = ((float)(src[0][0])); src[0]++;
    dst[1] = ((float)(src[1][0])); src[1]++;
    dst[2] = ((float)(src[2][0])); src[2]++;
    dst[3] = ((float)(src[3][0])); src[3]++;
    dst[4] = ((float)(src[4][0])); src[4]++;
    dst[5] = ((float)(src[5][0])); src[5]++;

    dst += nch;
  }
}

