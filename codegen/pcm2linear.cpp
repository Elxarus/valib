void pcm16_linear_1ch(uint8_t *, samples_t, size_t);
void pcm24_linear_1ch(uint8_t *, samples_t, size_t);
void pcm32_linear_1ch(uint8_t *, samples_t, size_t);
void pcm16_be_linear_1ch(uint8_t *, samples_t, size_t);
void pcm24_be_linear_1ch(uint8_t *, samples_t, size_t);
void pcm32_be_linear_1ch(uint8_t *, samples_t, size_t);
void pcmfloat_linear_1ch(uint8_t *, samples_t, size_t);
void pcmdouble_linear_1ch(uint8_t *, samples_t, size_t);

void pcm16_linear_2ch(uint8_t *, samples_t, size_t);
void pcm24_linear_2ch(uint8_t *, samples_t, size_t);
void pcm32_linear_2ch(uint8_t *, samples_t, size_t);
void pcm16_be_linear_2ch(uint8_t *, samples_t, size_t);
void pcm24_be_linear_2ch(uint8_t *, samples_t, size_t);
void pcm32_be_linear_2ch(uint8_t *, samples_t, size_t);
void pcmfloat_linear_2ch(uint8_t *, samples_t, size_t);
void pcmdouble_linear_2ch(uint8_t *, samples_t, size_t);

void pcm16_linear_3ch(uint8_t *, samples_t, size_t);
void pcm24_linear_3ch(uint8_t *, samples_t, size_t);
void pcm32_linear_3ch(uint8_t *, samples_t, size_t);
void pcm16_be_linear_3ch(uint8_t *, samples_t, size_t);
void pcm24_be_linear_3ch(uint8_t *, samples_t, size_t);
void pcm32_be_linear_3ch(uint8_t *, samples_t, size_t);
void pcmfloat_linear_3ch(uint8_t *, samples_t, size_t);
void pcmdouble_linear_3ch(uint8_t *, samples_t, size_t);

void pcm16_linear_4ch(uint8_t *, samples_t, size_t);
void pcm24_linear_4ch(uint8_t *, samples_t, size_t);
void pcm32_linear_4ch(uint8_t *, samples_t, size_t);
void pcm16_be_linear_4ch(uint8_t *, samples_t, size_t);
void pcm24_be_linear_4ch(uint8_t *, samples_t, size_t);
void pcm32_be_linear_4ch(uint8_t *, samples_t, size_t);
void pcmfloat_linear_4ch(uint8_t *, samples_t, size_t);
void pcmdouble_linear_4ch(uint8_t *, samples_t, size_t);

void pcm16_linear_5ch(uint8_t *, samples_t, size_t);
void pcm24_linear_5ch(uint8_t *, samples_t, size_t);
void pcm32_linear_5ch(uint8_t *, samples_t, size_t);
void pcm16_be_linear_5ch(uint8_t *, samples_t, size_t);
void pcm24_be_linear_5ch(uint8_t *, samples_t, size_t);
void pcm32_be_linear_5ch(uint8_t *, samples_t, size_t);
void pcmfloat_linear_5ch(uint8_t *, samples_t, size_t);
void pcmdouble_linear_5ch(uint8_t *, samples_t, size_t);

void pcm16_linear_6ch(uint8_t *, samples_t, size_t);
void pcm24_linear_6ch(uint8_t *, samples_t, size_t);
void pcm32_linear_6ch(uint8_t *, samples_t, size_t);
void pcm16_be_linear_6ch(uint8_t *, samples_t, size_t);
void pcm24_be_linear_6ch(uint8_t *, samples_t, size_t);
void pcm32_be_linear_6ch(uint8_t *, samples_t, size_t);
void pcmfloat_linear_6ch(uint8_t *, samples_t, size_t);
void pcmdouble_linear_6ch(uint8_t *, samples_t, size_t);

typedef void (Converter::*convert_t)(uint8_t *rawdata, samples_t samples, size_t size);

static const int formats_tbl[] = { FORMAT_PCM16, FORMAT_PCM24, FORMAT_PCM32, FORMAT_PCM16_BE, FORMAT_PCM24_BE, FORMAT_PCM32_BE, FORMAT_PCMFLOAT, FORMAT_PCMDOUBLE };

static const int formats = FORMAT_PCM16 | FORMAT_PCM24 | FORMAT_PCM32 | FORMAT_PCM16_BE | FORMAT_PCM24_BE | FORMAT_PCM32_BE | FORMAT_PCMFLOAT | FORMAT_PCMDOUBLE;

static const convert_t pcm2linear_tbl[NCHANNELS][8] = {
 { pcm16_linear_1ch, pcm24_linear_1ch, pcm32_linear_1ch, pcm16_be_linear_1ch, pcm24_be_linear_1ch, pcm32_be_linear_1ch, pcmfloat_linear_1ch, pcmdouble_linear_1ch },
 { pcm16_linear_2ch, pcm24_linear_2ch, pcm32_linear_2ch, pcm16_be_linear_2ch, pcm24_be_linear_2ch, pcm32_be_linear_2ch, pcmfloat_linear_2ch, pcmdouble_linear_2ch },
 { pcm16_linear_3ch, pcm24_linear_3ch, pcm32_linear_3ch, pcm16_be_linear_3ch, pcm24_be_linear_3ch, pcm32_be_linear_3ch, pcmfloat_linear_3ch, pcmdouble_linear_3ch },
 { pcm16_linear_4ch, pcm24_linear_4ch, pcm32_linear_4ch, pcm16_be_linear_4ch, pcm24_be_linear_4ch, pcm32_be_linear_4ch, pcmfloat_linear_4ch, pcmdouble_linear_4ch },
 { pcm16_linear_5ch, pcm24_linear_5ch, pcm32_linear_5ch, pcm16_be_linear_5ch, pcm24_be_linear_5ch, pcm32_be_linear_5ch, pcmfloat_linear_5ch, pcmdouble_linear_5ch },
 { pcm16_linear_6ch, pcm24_linear_6ch, pcm32_linear_6ch, pcm16_be_linear_6ch, pcm24_be_linear_6ch, pcm32_be_linear_6ch, pcmfloat_linear_6ch, pcmdouble_linear_6ch },
};

void
pcm16_linear_1ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  const int nch = 1;
  samples_t dst = samples;
  int16_t *src = (int16_t *)rawdata;

  while (size--)
  {
    dst[0][0] = le2int16(src[0]); dst[0]++;

    src += nch;
  }
}
void
pcm24_linear_1ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  const int nch = 1;
  samples_t dst = samples;
  int24_t *src = (int24_t *)rawdata;

  while (size--)
  {
    dst[0][0] = le2int24(src[0]); dst[0]++;

    src += nch;
  }
}
void
pcm32_linear_1ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  const int nch = 1;
  samples_t dst = samples;
  int32_t *src = (int32_t *)rawdata;

  while (size--)
  {
    dst[0][0] = le2int32(src[0]); dst[0]++;

    src += nch;
  }
}
void
pcm16_be_linear_1ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  const int nch = 1;
  samples_t dst = samples;
  int16_t *src = (int16_t *)rawdata;

  while (size--)
  {
    dst[0][0] = be2int16(src[0]); dst[0]++;

    src += nch;
  }
}
void
pcm24_be_linear_1ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  const int nch = 1;
  samples_t dst = samples;
  int24_t *src = (int24_t *)rawdata;

  while (size--)
  {
    dst[0][0] = be2int24(src[0]); dst[0]++;

    src += nch;
  }
}
void
pcm32_be_linear_1ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  const int nch = 1;
  samples_t dst = samples;
  int32_t *src = (int32_t *)rawdata;

  while (size--)
  {
    dst[0][0] = be2int32(src[0]); dst[0]++;

    src += nch;
  }
}
void
pcmfloat_linear_1ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  const int nch = 1;
  samples_t dst = samples;
  float *src = (float *)rawdata;

  while (size--)
  {
    dst[0][0] = sample_t(src[0]); dst[0]++;

    src += nch;
  }
}
void
pcmdouble_linear_1ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  const int nch = 1;
  samples_t dst = samples;
  double *src = (double *)rawdata;

  while (size--)
  {
    dst[0][0] = sample_t(src[0]); dst[0]++;

    src += nch;
  }
}

void
pcm16_linear_2ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  const int nch = 2;
  samples_t dst = samples;
  int16_t *src = (int16_t *)rawdata;

  while (size--)
  {
    dst[0][0] = le2int16(src[0]); dst[0]++;
    dst[1][0] = le2int16(src[1]); dst[1]++;

    src += nch;
  }
}
void
pcm24_linear_2ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  const int nch = 2;
  samples_t dst = samples;
  int24_t *src = (int24_t *)rawdata;

  while (size--)
  {
    dst[0][0] = le2int24(src[0]); dst[0]++;
    dst[1][0] = le2int24(src[1]); dst[1]++;

    src += nch;
  }
}
void
pcm32_linear_2ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  const int nch = 2;
  samples_t dst = samples;
  int32_t *src = (int32_t *)rawdata;

  while (size--)
  {
    dst[0][0] = le2int32(src[0]); dst[0]++;
    dst[1][0] = le2int32(src[1]); dst[1]++;

    src += nch;
  }
}
void
pcm16_be_linear_2ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  const int nch = 2;
  samples_t dst = samples;
  int16_t *src = (int16_t *)rawdata;

  while (size--)
  {
    dst[0][0] = be2int16(src[0]); dst[0]++;
    dst[1][0] = be2int16(src[1]); dst[1]++;

    src += nch;
  }
}
void
pcm24_be_linear_2ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  const int nch = 2;
  samples_t dst = samples;
  int24_t *src = (int24_t *)rawdata;

  while (size--)
  {
    dst[0][0] = be2int24(src[0]); dst[0]++;
    dst[1][0] = be2int24(src[1]); dst[1]++;

    src += nch;
  }
}
void
pcm32_be_linear_2ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  const int nch = 2;
  samples_t dst = samples;
  int32_t *src = (int32_t *)rawdata;

  while (size--)
  {
    dst[0][0] = be2int32(src[0]); dst[0]++;
    dst[1][0] = be2int32(src[1]); dst[1]++;

    src += nch;
  }
}
void
pcmfloat_linear_2ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  const int nch = 2;
  samples_t dst = samples;
  float *src = (float *)rawdata;

  while (size--)
  {
    dst[0][0] = sample_t(src[0]); dst[0]++;
    dst[1][0] = sample_t(src[1]); dst[1]++;

    src += nch;
  }
}
void
pcmdouble_linear_2ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  const int nch = 2;
  samples_t dst = samples;
  double *src = (double *)rawdata;

  while (size--)
  {
    dst[0][0] = sample_t(src[0]); dst[0]++;
    dst[1][0] = sample_t(src[1]); dst[1]++;

    src += nch;
  }
}

void
pcm16_linear_3ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  const int nch = 3;
  samples_t dst = samples;
  int16_t *src = (int16_t *)rawdata;

  while (size--)
  {
    dst[0][0] = le2int16(src[0]); dst[0]++;
    dst[1][0] = le2int16(src[1]); dst[1]++;
    dst[2][0] = le2int16(src[2]); dst[2]++;

    src += nch;
  }
}
void
pcm24_linear_3ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  const int nch = 3;
  samples_t dst = samples;
  int24_t *src = (int24_t *)rawdata;

  while (size--)
  {
    dst[0][0] = le2int24(src[0]); dst[0]++;
    dst[1][0] = le2int24(src[1]); dst[1]++;
    dst[2][0] = le2int24(src[2]); dst[2]++;

    src += nch;
  }
}
void
pcm32_linear_3ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  const int nch = 3;
  samples_t dst = samples;
  int32_t *src = (int32_t *)rawdata;

  while (size--)
  {
    dst[0][0] = le2int32(src[0]); dst[0]++;
    dst[1][0] = le2int32(src[1]); dst[1]++;
    dst[2][0] = le2int32(src[2]); dst[2]++;

    src += nch;
  }
}
void
pcm16_be_linear_3ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  const int nch = 3;
  samples_t dst = samples;
  int16_t *src = (int16_t *)rawdata;

  while (size--)
  {
    dst[0][0] = be2int16(src[0]); dst[0]++;
    dst[1][0] = be2int16(src[1]); dst[1]++;
    dst[2][0] = be2int16(src[2]); dst[2]++;

    src += nch;
  }
}
void
pcm24_be_linear_3ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  const int nch = 3;
  samples_t dst = samples;
  int24_t *src = (int24_t *)rawdata;

  while (size--)
  {
    dst[0][0] = be2int24(src[0]); dst[0]++;
    dst[1][0] = be2int24(src[1]); dst[1]++;
    dst[2][0] = be2int24(src[2]); dst[2]++;

    src += nch;
  }
}
void
pcm32_be_linear_3ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  const int nch = 3;
  samples_t dst = samples;
  int32_t *src = (int32_t *)rawdata;

  while (size--)
  {
    dst[0][0] = be2int32(src[0]); dst[0]++;
    dst[1][0] = be2int32(src[1]); dst[1]++;
    dst[2][0] = be2int32(src[2]); dst[2]++;

    src += nch;
  }
}
void
pcmfloat_linear_3ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  const int nch = 3;
  samples_t dst = samples;
  float *src = (float *)rawdata;

  while (size--)
  {
    dst[0][0] = sample_t(src[0]); dst[0]++;
    dst[1][0] = sample_t(src[1]); dst[1]++;
    dst[2][0] = sample_t(src[2]); dst[2]++;

    src += nch;
  }
}
void
pcmdouble_linear_3ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  const int nch = 3;
  samples_t dst = samples;
  double *src = (double *)rawdata;

  while (size--)
  {
    dst[0][0] = sample_t(src[0]); dst[0]++;
    dst[1][0] = sample_t(src[1]); dst[1]++;
    dst[2][0] = sample_t(src[2]); dst[2]++;

    src += nch;
  }
}

void
pcm16_linear_4ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  const int nch = 4;
  samples_t dst = samples;
  int16_t *src = (int16_t *)rawdata;

  while (size--)
  {
    dst[0][0] = le2int16(src[0]); dst[0]++;
    dst[1][0] = le2int16(src[1]); dst[1]++;
    dst[2][0] = le2int16(src[2]); dst[2]++;
    dst[3][0] = le2int16(src[3]); dst[3]++;

    src += nch;
  }
}
void
pcm24_linear_4ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  const int nch = 4;
  samples_t dst = samples;
  int24_t *src = (int24_t *)rawdata;

  while (size--)
  {
    dst[0][0] = le2int24(src[0]); dst[0]++;
    dst[1][0] = le2int24(src[1]); dst[1]++;
    dst[2][0] = le2int24(src[2]); dst[2]++;
    dst[3][0] = le2int24(src[3]); dst[3]++;

    src += nch;
  }
}
void
pcm32_linear_4ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  const int nch = 4;
  samples_t dst = samples;
  int32_t *src = (int32_t *)rawdata;

  while (size--)
  {
    dst[0][0] = le2int32(src[0]); dst[0]++;
    dst[1][0] = le2int32(src[1]); dst[1]++;
    dst[2][0] = le2int32(src[2]); dst[2]++;
    dst[3][0] = le2int32(src[3]); dst[3]++;

    src += nch;
  }
}
void
pcm16_be_linear_4ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  const int nch = 4;
  samples_t dst = samples;
  int16_t *src = (int16_t *)rawdata;

  while (size--)
  {
    dst[0][0] = be2int16(src[0]); dst[0]++;
    dst[1][0] = be2int16(src[1]); dst[1]++;
    dst[2][0] = be2int16(src[2]); dst[2]++;
    dst[3][0] = be2int16(src[3]); dst[3]++;

    src += nch;
  }
}
void
pcm24_be_linear_4ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  const int nch = 4;
  samples_t dst = samples;
  int24_t *src = (int24_t *)rawdata;

  while (size--)
  {
    dst[0][0] = be2int24(src[0]); dst[0]++;
    dst[1][0] = be2int24(src[1]); dst[1]++;
    dst[2][0] = be2int24(src[2]); dst[2]++;
    dst[3][0] = be2int24(src[3]); dst[3]++;

    src += nch;
  }
}
void
pcm32_be_linear_4ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  const int nch = 4;
  samples_t dst = samples;
  int32_t *src = (int32_t *)rawdata;

  while (size--)
  {
    dst[0][0] = be2int32(src[0]); dst[0]++;
    dst[1][0] = be2int32(src[1]); dst[1]++;
    dst[2][0] = be2int32(src[2]); dst[2]++;
    dst[3][0] = be2int32(src[3]); dst[3]++;

    src += nch;
  }
}
void
pcmfloat_linear_4ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  const int nch = 4;
  samples_t dst = samples;
  float *src = (float *)rawdata;

  while (size--)
  {
    dst[0][0] = sample_t(src[0]); dst[0]++;
    dst[1][0] = sample_t(src[1]); dst[1]++;
    dst[2][0] = sample_t(src[2]); dst[2]++;
    dst[3][0] = sample_t(src[3]); dst[3]++;

    src += nch;
  }
}
void
pcmdouble_linear_4ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  const int nch = 4;
  samples_t dst = samples;
  double *src = (double *)rawdata;

  while (size--)
  {
    dst[0][0] = sample_t(src[0]); dst[0]++;
    dst[1][0] = sample_t(src[1]); dst[1]++;
    dst[2][0] = sample_t(src[2]); dst[2]++;
    dst[3][0] = sample_t(src[3]); dst[3]++;

    src += nch;
  }
}

void
pcm16_linear_5ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  const int nch = 5;
  samples_t dst = samples;
  int16_t *src = (int16_t *)rawdata;

  while (size--)
  {
    dst[0][0] = le2int16(src[0]); dst[0]++;
    dst[1][0] = le2int16(src[1]); dst[1]++;
    dst[2][0] = le2int16(src[2]); dst[2]++;
    dst[3][0] = le2int16(src[3]); dst[3]++;
    dst[4][0] = le2int16(src[4]); dst[4]++;

    src += nch;
  }
}
void
pcm24_linear_5ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  const int nch = 5;
  samples_t dst = samples;
  int24_t *src = (int24_t *)rawdata;

  while (size--)
  {
    dst[0][0] = le2int24(src[0]); dst[0]++;
    dst[1][0] = le2int24(src[1]); dst[1]++;
    dst[2][0] = le2int24(src[2]); dst[2]++;
    dst[3][0] = le2int24(src[3]); dst[3]++;
    dst[4][0] = le2int24(src[4]); dst[4]++;

    src += nch;
  }
}
void
pcm32_linear_5ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  const int nch = 5;
  samples_t dst = samples;
  int32_t *src = (int32_t *)rawdata;

  while (size--)
  {
    dst[0][0] = le2int32(src[0]); dst[0]++;
    dst[1][0] = le2int32(src[1]); dst[1]++;
    dst[2][0] = le2int32(src[2]); dst[2]++;
    dst[3][0] = le2int32(src[3]); dst[3]++;
    dst[4][0] = le2int32(src[4]); dst[4]++;

    src += nch;
  }
}
void
pcm16_be_linear_5ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  const int nch = 5;
  samples_t dst = samples;
  int16_t *src = (int16_t *)rawdata;

  while (size--)
  {
    dst[0][0] = be2int16(src[0]); dst[0]++;
    dst[1][0] = be2int16(src[1]); dst[1]++;
    dst[2][0] = be2int16(src[2]); dst[2]++;
    dst[3][0] = be2int16(src[3]); dst[3]++;
    dst[4][0] = be2int16(src[4]); dst[4]++;

    src += nch;
  }
}
void
pcm24_be_linear_5ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  const int nch = 5;
  samples_t dst = samples;
  int24_t *src = (int24_t *)rawdata;

  while (size--)
  {
    dst[0][0] = be2int24(src[0]); dst[0]++;
    dst[1][0] = be2int24(src[1]); dst[1]++;
    dst[2][0] = be2int24(src[2]); dst[2]++;
    dst[3][0] = be2int24(src[3]); dst[3]++;
    dst[4][0] = be2int24(src[4]); dst[4]++;

    src += nch;
  }
}
void
pcm32_be_linear_5ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  const int nch = 5;
  samples_t dst = samples;
  int32_t *src = (int32_t *)rawdata;

  while (size--)
  {
    dst[0][0] = be2int32(src[0]); dst[0]++;
    dst[1][0] = be2int32(src[1]); dst[1]++;
    dst[2][0] = be2int32(src[2]); dst[2]++;
    dst[3][0] = be2int32(src[3]); dst[3]++;
    dst[4][0] = be2int32(src[4]); dst[4]++;

    src += nch;
  }
}
void
pcmfloat_linear_5ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  const int nch = 5;
  samples_t dst = samples;
  float *src = (float *)rawdata;

  while (size--)
  {
    dst[0][0] = sample_t(src[0]); dst[0]++;
    dst[1][0] = sample_t(src[1]); dst[1]++;
    dst[2][0] = sample_t(src[2]); dst[2]++;
    dst[3][0] = sample_t(src[3]); dst[3]++;
    dst[4][0] = sample_t(src[4]); dst[4]++;

    src += nch;
  }
}
void
pcmdouble_linear_5ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  const int nch = 5;
  samples_t dst = samples;
  double *src = (double *)rawdata;

  while (size--)
  {
    dst[0][0] = sample_t(src[0]); dst[0]++;
    dst[1][0] = sample_t(src[1]); dst[1]++;
    dst[2][0] = sample_t(src[2]); dst[2]++;
    dst[3][0] = sample_t(src[3]); dst[3]++;
    dst[4][0] = sample_t(src[4]); dst[4]++;

    src += nch;
  }
}

void
pcm16_linear_6ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  const int nch = 6;
  samples_t dst = samples;
  int16_t *src = (int16_t *)rawdata;

  while (size--)
  {
    dst[0][0] = le2int16(src[0]); dst[0]++;
    dst[1][0] = le2int16(src[1]); dst[1]++;
    dst[2][0] = le2int16(src[2]); dst[2]++;
    dst[3][0] = le2int16(src[3]); dst[3]++;
    dst[4][0] = le2int16(src[4]); dst[4]++;
    dst[5][0] = le2int16(src[5]); dst[5]++;

    src += nch;
  }
}
void
pcm24_linear_6ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  const int nch = 6;
  samples_t dst = samples;
  int24_t *src = (int24_t *)rawdata;

  while (size--)
  {
    dst[0][0] = le2int24(src[0]); dst[0]++;
    dst[1][0] = le2int24(src[1]); dst[1]++;
    dst[2][0] = le2int24(src[2]); dst[2]++;
    dst[3][0] = le2int24(src[3]); dst[3]++;
    dst[4][0] = le2int24(src[4]); dst[4]++;
    dst[5][0] = le2int24(src[5]); dst[5]++;

    src += nch;
  }
}
void
pcm32_linear_6ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  const int nch = 6;
  samples_t dst = samples;
  int32_t *src = (int32_t *)rawdata;

  while (size--)
  {
    dst[0][0] = le2int32(src[0]); dst[0]++;
    dst[1][0] = le2int32(src[1]); dst[1]++;
    dst[2][0] = le2int32(src[2]); dst[2]++;
    dst[3][0] = le2int32(src[3]); dst[3]++;
    dst[4][0] = le2int32(src[4]); dst[4]++;
    dst[5][0] = le2int32(src[5]); dst[5]++;

    src += nch;
  }
}
void
pcm16_be_linear_6ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  const int nch = 6;
  samples_t dst = samples;
  int16_t *src = (int16_t *)rawdata;

  while (size--)
  {
    dst[0][0] = be2int16(src[0]); dst[0]++;
    dst[1][0] = be2int16(src[1]); dst[1]++;
    dst[2][0] = be2int16(src[2]); dst[2]++;
    dst[3][0] = be2int16(src[3]); dst[3]++;
    dst[4][0] = be2int16(src[4]); dst[4]++;
    dst[5][0] = be2int16(src[5]); dst[5]++;

    src += nch;
  }
}
void
pcm24_be_linear_6ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  const int nch = 6;
  samples_t dst = samples;
  int24_t *src = (int24_t *)rawdata;

  while (size--)
  {
    dst[0][0] = be2int24(src[0]); dst[0]++;
    dst[1][0] = be2int24(src[1]); dst[1]++;
    dst[2][0] = be2int24(src[2]); dst[2]++;
    dst[3][0] = be2int24(src[3]); dst[3]++;
    dst[4][0] = be2int24(src[4]); dst[4]++;
    dst[5][0] = be2int24(src[5]); dst[5]++;

    src += nch;
  }
}
void
pcm32_be_linear_6ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  const int nch = 6;
  samples_t dst = samples;
  int32_t *src = (int32_t *)rawdata;

  while (size--)
  {
    dst[0][0] = be2int32(src[0]); dst[0]++;
    dst[1][0] = be2int32(src[1]); dst[1]++;
    dst[2][0] = be2int32(src[2]); dst[2]++;
    dst[3][0] = be2int32(src[3]); dst[3]++;
    dst[4][0] = be2int32(src[4]); dst[4]++;
    dst[5][0] = be2int32(src[5]); dst[5]++;

    src += nch;
  }
}
void
pcmfloat_linear_6ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  const int nch = 6;
  samples_t dst = samples;
  float *src = (float *)rawdata;

  while (size--)
  {
    dst[0][0] = sample_t(src[0]); dst[0]++;
    dst[1][0] = sample_t(src[1]); dst[1]++;
    dst[2][0] = sample_t(src[2]); dst[2]++;
    dst[3][0] = sample_t(src[3]); dst[3]++;
    dst[4][0] = sample_t(src[4]); dst[4]++;
    dst[5][0] = sample_t(src[5]); dst[5]++;

    src += nch;
  }
}
void
pcmdouble_linear_6ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  const int nch = 6;
  samples_t dst = samples;
  double *src = (double *)rawdata;

  while (size--)
  {
    dst[0][0] = sample_t(src[0]); dst[0]++;
    dst[1][0] = sample_t(src[1]); dst[1]++;
    dst[2][0] = sample_t(src[2]); dst[2]++;
    dst[3][0] = sample_t(src[3]); dst[3]++;
    dst[4][0] = sample_t(src[4]); dst[4]++;
    dst[5][0] = sample_t(src[5]); dst[5]++;

    src += nch;
  }
}

