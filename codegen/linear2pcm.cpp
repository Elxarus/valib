  void linear_pcm16_1ch();
  void linear_pcm24_1ch();
  void linear_pcm32_1ch();
  void linear_pcmfloat_1ch();
  void linear_pcm16_le_1ch();
  void linear_pcm24_le_1ch();
  void linear_pcm32_le_1ch();
  void linear_pcmfloat_le_1ch();

  void linear_pcm16_2ch();
  void linear_pcm24_2ch();
  void linear_pcm32_2ch();
  void linear_pcmfloat_2ch();
  void linear_pcm16_le_2ch();
  void linear_pcm24_le_2ch();
  void linear_pcm32_le_2ch();
  void linear_pcmfloat_le_2ch();

  void linear_pcm16_3ch();
  void linear_pcm24_3ch();
  void linear_pcm32_3ch();
  void linear_pcmfloat_3ch();
  void linear_pcm16_le_3ch();
  void linear_pcm24_le_3ch();
  void linear_pcm32_le_3ch();
  void linear_pcmfloat_le_3ch();

  void linear_pcm16_4ch();
  void linear_pcm24_4ch();
  void linear_pcm32_4ch();
  void linear_pcmfloat_4ch();
  void linear_pcm16_le_4ch();
  void linear_pcm24_le_4ch();
  void linear_pcm32_le_4ch();
  void linear_pcmfloat_le_4ch();

  void linear_pcm16_5ch();
  void linear_pcm24_5ch();
  void linear_pcm32_5ch();
  void linear_pcmfloat_5ch();
  void linear_pcm16_le_5ch();
  void linear_pcm24_le_5ch();
  void linear_pcm32_le_5ch();
  void linear_pcmfloat_le_5ch();

  void linear_pcm16_6ch();
  void linear_pcm24_6ch();
  void linear_pcm32_6ch();
  void linear_pcmfloat_6ch();
  void linear_pcm16_le_6ch();
  void linear_pcm24_le_6ch();
  void linear_pcm32_le_6ch();
  void linear_pcmfloat_le_6ch();

typedef void (Converter::*convert_t)();

static const int formats_tbl[] = {FORMAT_PCM16, FORMAT_PCM24, FORMAT_PCM32, FORMAT_PCMFLOAT, FORMAT_PCM16_LE, FORMAT_PCM24_LE, FORMAT_PCM32_LE, FORMAT_PCMFLOAT_LE}

const int formats = FORMAT_PCM16 | FORMAT_PCM24 | FORMAT_PCM32 | FORMAT_PCMFLOAT | FORMAT_PCM16_LE | FORMAT_PCM24_LE | FORMAT_PCM32_LE | FORMAT_PCMFLOAT_LE;

static const convert_t linear2pcm_tbl[NCHANNELS][8] = {
 { Converter::linear_pcm16_1ch, Converter::linear_pcm24_1ch, Converter::linear_pcm32_1ch, Converter::linear_pcmfloat_1ch, Converter::linear_pcm16_le_1ch, Converter::linear_pcm24_le_1ch, Converter::linear_pcm32_le_1ch, Converter::linear_pcmfloat_le_1ch },
 { Converter::linear_pcm16_2ch, Converter::linear_pcm24_2ch, Converter::linear_pcm32_2ch, Converter::linear_pcmfloat_2ch, Converter::linear_pcm16_le_2ch, Converter::linear_pcm24_le_2ch, Converter::linear_pcm32_le_2ch, Converter::linear_pcmfloat_le_2ch },
 { Converter::linear_pcm16_3ch, Converter::linear_pcm24_3ch, Converter::linear_pcm32_3ch, Converter::linear_pcmfloat_3ch, Converter::linear_pcm16_le_3ch, Converter::linear_pcm24_le_3ch, Converter::linear_pcm32_le_3ch, Converter::linear_pcmfloat_le_3ch },
 { Converter::linear_pcm16_4ch, Converter::linear_pcm24_4ch, Converter::linear_pcm32_4ch, Converter::linear_pcmfloat_4ch, Converter::linear_pcm16_le_4ch, Converter::linear_pcm24_le_4ch, Converter::linear_pcm32_le_4ch, Converter::linear_pcmfloat_le_4ch },
 { Converter::linear_pcm16_5ch, Converter::linear_pcm24_5ch, Converter::linear_pcm32_5ch, Converter::linear_pcmfloat_5ch, Converter::linear_pcm16_le_5ch, Converter::linear_pcm24_le_5ch, Converter::linear_pcm32_le_5ch, Converter::linear_pcmfloat_le_5ch },
 { Converter::linear_pcm16_6ch, Converter::linear_pcm24_6ch, Converter::linear_pcm32_6ch, Converter::linear_pcmfloat_6ch, Converter::linear_pcm16_le_6ch, Converter::linear_pcm24_le_6ch, Converter::linear_pcm32_le_6ch, Converter::linear_pcmfloat_le_6ch },
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
    dst[0] = (s2i16(src[0][0])); src[0]++;

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
    dst[0] = (s2i32(src[0][0])); src[0]++;

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
    dst[0] = (s2i32(src[0][0])); src[0]++;

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
Converter::linear_pcm16_le_1ch()
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
    dst[0] = swab_s16(s2i16(src[0][0])); src[0]++;

    dst += nch;
  }
}
void
Converter::linear_pcm24_le_1ch()
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
    dst[0] = swab_s24(s2i32(src[0][0])); src[0]++;

    dst += nch;
  }
}
void
Converter::linear_pcm32_le_1ch()
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
    dst[0] = swab_s32(s2i32(src[0][0])); src[0]++;

    dst += nch;
  }
}
void
Converter::linear_pcmfloat_le_1ch()
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
    dst[0] = swab_float((float)(src[0][0])); src[0]++;

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
    dst[0] = (s2i16(src[0][0])); src[0]++;
    dst[1] = (s2i16(src[1][0])); src[1]++;

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
    dst[0] = (s2i32(src[0][0])); src[0]++;
    dst[1] = (s2i32(src[1][0])); src[1]++;

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
    dst[0] = (s2i32(src[0][0])); src[0]++;
    dst[1] = (s2i32(src[1][0])); src[1]++;

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
Converter::linear_pcm16_le_2ch()
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
    dst[0] = swab_s16(s2i16(src[0][0])); src[0]++;
    dst[1] = swab_s16(s2i16(src[1][0])); src[1]++;

    dst += nch;
  }
}
void
Converter::linear_pcm24_le_2ch()
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
    dst[0] = swab_s24(s2i32(src[0][0])); src[0]++;
    dst[1] = swab_s24(s2i32(src[1][0])); src[1]++;

    dst += nch;
  }
}
void
Converter::linear_pcm32_le_2ch()
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
    dst[0] = swab_s32(s2i32(src[0][0])); src[0]++;
    dst[1] = swab_s32(s2i32(src[1][0])); src[1]++;

    dst += nch;
  }
}
void
Converter::linear_pcmfloat_le_2ch()
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
    dst[0] = swab_float((float)(src[0][0])); src[0]++;
    dst[1] = swab_float((float)(src[1][0])); src[1]++;

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
    dst[0] = (s2i16(src[0][0])); src[0]++;
    dst[1] = (s2i16(src[1][0])); src[1]++;
    dst[2] = (s2i16(src[2][0])); src[2]++;

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
    dst[0] = (s2i32(src[0][0])); src[0]++;
    dst[1] = (s2i32(src[1][0])); src[1]++;
    dst[2] = (s2i32(src[2][0])); src[2]++;

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
    dst[0] = (s2i32(src[0][0])); src[0]++;
    dst[1] = (s2i32(src[1][0])); src[1]++;
    dst[2] = (s2i32(src[2][0])); src[2]++;

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
Converter::linear_pcm16_le_3ch()
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
    dst[0] = swab_s16(s2i16(src[0][0])); src[0]++;
    dst[1] = swab_s16(s2i16(src[1][0])); src[1]++;
    dst[2] = swab_s16(s2i16(src[2][0])); src[2]++;

    dst += nch;
  }
}
void
Converter::linear_pcm24_le_3ch()
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
    dst[0] = swab_s24(s2i32(src[0][0])); src[0]++;
    dst[1] = swab_s24(s2i32(src[1][0])); src[1]++;
    dst[2] = swab_s24(s2i32(src[2][0])); src[2]++;

    dst += nch;
  }
}
void
Converter::linear_pcm32_le_3ch()
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
    dst[0] = swab_s32(s2i32(src[0][0])); src[0]++;
    dst[1] = swab_s32(s2i32(src[1][0])); src[1]++;
    dst[2] = swab_s32(s2i32(src[2][0])); src[2]++;

    dst += nch;
  }
}
void
Converter::linear_pcmfloat_le_3ch()
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
    dst[0] = swab_float((float)(src[0][0])); src[0]++;
    dst[1] = swab_float((float)(src[1][0])); src[1]++;
    dst[2] = swab_float((float)(src[2][0])); src[2]++;

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
    dst[0] = (s2i16(src[0][0])); src[0]++;
    dst[1] = (s2i16(src[1][0])); src[1]++;
    dst[2] = (s2i16(src[2][0])); src[2]++;
    dst[3] = (s2i16(src[3][0])); src[3]++;

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
    dst[0] = (s2i32(src[0][0])); src[0]++;
    dst[1] = (s2i32(src[1][0])); src[1]++;
    dst[2] = (s2i32(src[2][0])); src[2]++;
    dst[3] = (s2i32(src[3][0])); src[3]++;

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
    dst[0] = (s2i32(src[0][0])); src[0]++;
    dst[1] = (s2i32(src[1][0])); src[1]++;
    dst[2] = (s2i32(src[2][0])); src[2]++;
    dst[3] = (s2i32(src[3][0])); src[3]++;

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
Converter::linear_pcm16_le_4ch()
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
    dst[0] = swab_s16(s2i16(src[0][0])); src[0]++;
    dst[1] = swab_s16(s2i16(src[1][0])); src[1]++;
    dst[2] = swab_s16(s2i16(src[2][0])); src[2]++;
    dst[3] = swab_s16(s2i16(src[3][0])); src[3]++;

    dst += nch;
  }
}
void
Converter::linear_pcm24_le_4ch()
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
    dst[0] = swab_s24(s2i32(src[0][0])); src[0]++;
    dst[1] = swab_s24(s2i32(src[1][0])); src[1]++;
    dst[2] = swab_s24(s2i32(src[2][0])); src[2]++;
    dst[3] = swab_s24(s2i32(src[3][0])); src[3]++;

    dst += nch;
  }
}
void
Converter::linear_pcm32_le_4ch()
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
    dst[0] = swab_s32(s2i32(src[0][0])); src[0]++;
    dst[1] = swab_s32(s2i32(src[1][0])); src[1]++;
    dst[2] = swab_s32(s2i32(src[2][0])); src[2]++;
    dst[3] = swab_s32(s2i32(src[3][0])); src[3]++;

    dst += nch;
  }
}
void
Converter::linear_pcmfloat_le_4ch()
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
    dst[0] = swab_float((float)(src[0][0])); src[0]++;
    dst[1] = swab_float((float)(src[1][0])); src[1]++;
    dst[2] = swab_float((float)(src[2][0])); src[2]++;
    dst[3] = swab_float((float)(src[3][0])); src[3]++;

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
    dst[0] = (s2i16(src[0][0])); src[0]++;
    dst[1] = (s2i16(src[1][0])); src[1]++;
    dst[2] = (s2i16(src[2][0])); src[2]++;
    dst[3] = (s2i16(src[3][0])); src[3]++;
    dst[4] = (s2i16(src[4][0])); src[4]++;

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
    dst[0] = (s2i32(src[0][0])); src[0]++;
    dst[1] = (s2i32(src[1][0])); src[1]++;
    dst[2] = (s2i32(src[2][0])); src[2]++;
    dst[3] = (s2i32(src[3][0])); src[3]++;
    dst[4] = (s2i32(src[4][0])); src[4]++;

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
    dst[0] = (s2i32(src[0][0])); src[0]++;
    dst[1] = (s2i32(src[1][0])); src[1]++;
    dst[2] = (s2i32(src[2][0])); src[2]++;
    dst[3] = (s2i32(src[3][0])); src[3]++;
    dst[4] = (s2i32(src[4][0])); src[4]++;

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
Converter::linear_pcm16_le_5ch()
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
    dst[0] = swab_s16(s2i16(src[0][0])); src[0]++;
    dst[1] = swab_s16(s2i16(src[1][0])); src[1]++;
    dst[2] = swab_s16(s2i16(src[2][0])); src[2]++;
    dst[3] = swab_s16(s2i16(src[3][0])); src[3]++;
    dst[4] = swab_s16(s2i16(src[4][0])); src[4]++;

    dst += nch;
  }
}
void
Converter::linear_pcm24_le_5ch()
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
    dst[0] = swab_s24(s2i32(src[0][0])); src[0]++;
    dst[1] = swab_s24(s2i32(src[1][0])); src[1]++;
    dst[2] = swab_s24(s2i32(src[2][0])); src[2]++;
    dst[3] = swab_s24(s2i32(src[3][0])); src[3]++;
    dst[4] = swab_s24(s2i32(src[4][0])); src[4]++;

    dst += nch;
  }
}
void
Converter::linear_pcm32_le_5ch()
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
    dst[0] = swab_s32(s2i32(src[0][0])); src[0]++;
    dst[1] = swab_s32(s2i32(src[1][0])); src[1]++;
    dst[2] = swab_s32(s2i32(src[2][0])); src[2]++;
    dst[3] = swab_s32(s2i32(src[3][0])); src[3]++;
    dst[4] = swab_s32(s2i32(src[4][0])); src[4]++;

    dst += nch;
  }
}
void
Converter::linear_pcmfloat_le_5ch()
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
    dst[0] = swab_float((float)(src[0][0])); src[0]++;
    dst[1] = swab_float((float)(src[1][0])); src[1]++;
    dst[2] = swab_float((float)(src[2][0])); src[2]++;
    dst[3] = swab_float((float)(src[3][0])); src[3]++;
    dst[4] = swab_float((float)(src[4][0])); src[4]++;

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
    dst[0] = (s2i16(src[0][0])); src[0]++;
    dst[1] = (s2i16(src[1][0])); src[1]++;
    dst[2] = (s2i16(src[2][0])); src[2]++;
    dst[3] = (s2i16(src[3][0])); src[3]++;
    dst[4] = (s2i16(src[4][0])); src[4]++;
    dst[5] = (s2i16(src[5][0])); src[5]++;

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
    dst[0] = (s2i32(src[0][0])); src[0]++;
    dst[1] = (s2i32(src[1][0])); src[1]++;
    dst[2] = (s2i32(src[2][0])); src[2]++;
    dst[3] = (s2i32(src[3][0])); src[3]++;
    dst[4] = (s2i32(src[4][0])); src[4]++;
    dst[5] = (s2i32(src[5][0])); src[5]++;

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
    dst[0] = (s2i32(src[0][0])); src[0]++;
    dst[1] = (s2i32(src[1][0])); src[1]++;
    dst[2] = (s2i32(src[2][0])); src[2]++;
    dst[3] = (s2i32(src[3][0])); src[3]++;
    dst[4] = (s2i32(src[4][0])); src[4]++;
    dst[5] = (s2i32(src[5][0])); src[5]++;

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
void
Converter::linear_pcm16_le_6ch()
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
    dst[0] = swab_s16(s2i16(src[0][0])); src[0]++;
    dst[1] = swab_s16(s2i16(src[1][0])); src[1]++;
    dst[2] = swab_s16(s2i16(src[2][0])); src[2]++;
    dst[3] = swab_s16(s2i16(src[3][0])); src[3]++;
    dst[4] = swab_s16(s2i16(src[4][0])); src[4]++;
    dst[5] = swab_s16(s2i16(src[5][0])); src[5]++;

    dst += nch;
  }
}
void
Converter::linear_pcm24_le_6ch()
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
    dst[0] = swab_s24(s2i32(src[0][0])); src[0]++;
    dst[1] = swab_s24(s2i32(src[1][0])); src[1]++;
    dst[2] = swab_s24(s2i32(src[2][0])); src[2]++;
    dst[3] = swab_s24(s2i32(src[3][0])); src[3]++;
    dst[4] = swab_s24(s2i32(src[4][0])); src[4]++;
    dst[5] = swab_s24(s2i32(src[5][0])); src[5]++;

    dst += nch;
  }
}
void
Converter::linear_pcm32_le_6ch()
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
    dst[0] = swab_s32(s2i32(src[0][0])); src[0]++;
    dst[1] = swab_s32(s2i32(src[1][0])); src[1]++;
    dst[2] = swab_s32(s2i32(src[2][0])); src[2]++;
    dst[3] = swab_s32(s2i32(src[3][0])); src[3]++;
    dst[4] = swab_s32(s2i32(src[4][0])); src[4]++;
    dst[5] = swab_s32(s2i32(src[5][0])); src[5]++;

    dst += nch;
  }
}
void
Converter::linear_pcmfloat_le_6ch()
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
    dst[0] = swab_float((float)(src[0][0])); src[0]++;
    dst[1] = swab_float((float)(src[1][0])); src[1]++;
    dst[2] = swab_float((float)(src[2][0])); src[2]++;
    dst[3] = swab_float((float)(src[3][0])); src[3]++;
    dst[4] = swab_float((float)(src[4][0])); src[4]++;
    dst[5] = swab_float((float)(src[5][0])); src[5]++;

    dst += nch;
  }
}

