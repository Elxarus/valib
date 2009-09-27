typedef void (Converter::*convert_t)(uint8_t *rawdata, samples_t samples, size_t size);
static const int linear2pcm_formats[] = { FORMAT_PCM16, FORMAT_PCM24, FORMAT_PCM32, FORMAT_PCM16_BE, FORMAT_PCM24_BE, FORMAT_PCM32_BE, FORMAT_PCMFLOAT, FORMAT_PCMDOUBLE };

static const convert_t linear2pcm_tbl[NCHANNELS][8] = {
 { linear_pcm16_1ch, linear_pcm24_1ch, linear_pcm32_1ch, linear_pcm16_be_1ch, linear_pcm24_be_1ch, linear_pcm32_be_1ch, linear_pcmfloat_1ch, linear_pcmdouble_1ch },
 { linear_pcm16_2ch, linear_pcm24_2ch, linear_pcm32_2ch, linear_pcm16_be_2ch, linear_pcm24_be_2ch, linear_pcm32_be_2ch, linear_pcmfloat_2ch, linear_pcmdouble_2ch },
 { linear_pcm16_3ch, linear_pcm24_3ch, linear_pcm32_3ch, linear_pcm16_be_3ch, linear_pcm24_be_3ch, linear_pcm32_be_3ch, linear_pcmfloat_3ch, linear_pcmdouble_3ch },
 { linear_pcm16_4ch, linear_pcm24_4ch, linear_pcm32_4ch, linear_pcm16_be_4ch, linear_pcm24_be_4ch, linear_pcm32_be_4ch, linear_pcmfloat_4ch, linear_pcmdouble_4ch },
 { linear_pcm16_5ch, linear_pcm24_5ch, linear_pcm32_5ch, linear_pcm16_be_5ch, linear_pcm24_be_5ch, linear_pcm32_be_5ch, linear_pcmfloat_5ch, linear_pcmdouble_5ch },
 { linear_pcm16_6ch, linear_pcm24_6ch, linear_pcm32_6ch, linear_pcm16_be_6ch, linear_pcm24_be_6ch, linear_pcm32_be_6ch, linear_pcmfloat_6ch, linear_pcmdouble_6ch },
 { linear_pcm16_7ch, linear_pcm24_7ch, linear_pcm32_7ch, linear_pcm16_be_7ch, linear_pcm24_be_7ch, linear_pcm32_be_7ch, linear_pcmfloat_7ch, linear_pcmdouble_7ch },
 { linear_pcm16_8ch, linear_pcm24_8ch, linear_pcm32_8ch, linear_pcm16_be_8ch, linear_pcm24_be_8ch, linear_pcm32_be_8ch, linear_pcmfloat_8ch, linear_pcmdouble_8ch },
};

void linear_pcm16_1ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  samples_t src = samples;
  int16_t *dst = (int16_t *)rawdata;

  int r = set_rounding();
  while (size--)
  {
    dst[0] = int2le16(s2i(*src[0])); src[0]++;
    dst += 1;
  }
  restore_rounding(r);
}
void linear_pcm24_1ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  samples_t src = samples;
  int24_t *dst = (int24_t *)rawdata;

  int r = set_rounding();
  while (size--)
  {
    dst[0] = int2le24(s2i(*src[0])); src[0]++;
    dst += 1;
  }
  restore_rounding(r);
}
void linear_pcm32_1ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  samples_t src = samples;
  int32_t *dst = (int32_t *)rawdata;

  int r = set_rounding();
  while (size--)
  {
    dst[0] = int2le32(s2i(*src[0])); src[0]++;
    dst += 1;
  }
  restore_rounding(r);
}
void linear_pcm16_be_1ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  samples_t src = samples;
  int16_t *dst = (int16_t *)rawdata;

  int r = set_rounding();
  while (size--)
  {
    dst[0] = int2be16(s2i(*src[0])); src[0]++;
    dst += 1;
  }
  restore_rounding(r);
}
void linear_pcm24_be_1ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  samples_t src = samples;
  int24_t *dst = (int24_t *)rawdata;

  int r = set_rounding();
  while (size--)
  {
    dst[0] = int2be24(s2i(*src[0])); src[0]++;
    dst += 1;
  }
  restore_rounding(r);
}
void linear_pcm32_be_1ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  samples_t src = samples;
  int32_t *dst = (int32_t *)rawdata;

  int r = set_rounding();
  while (size--)
  {
    dst[0] = int2be32(s2i(*src[0])); src[0]++;
    dst += 1;
  }
  restore_rounding(r);
}
void linear_pcmfloat_1ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  samples_t src = samples;
  float *dst = (float *)rawdata;

  int r = set_rounding();
  while (size--)
  {
    dst[0] = float(*src[0]); src[0]++;
    dst += 1;
  }
  restore_rounding(r);
}
void linear_pcmdouble_1ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  samples_t src = samples;
  double *dst = (double *)rawdata;

  int r = set_rounding();
  while (size--)
  {
    dst[0] = double(*src[0]); src[0]++;
    dst += 1;
  }
  restore_rounding(r);
}
void linear_pcm16_2ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  samples_t src = samples;
  int16_t *dst = (int16_t *)rawdata;

  int r = set_rounding();
  while (size--)
  {
    dst[0] = int2le16(s2i(*src[0])); src[0]++;
    dst[1] = int2le16(s2i(*src[1])); src[1]++;
    dst += 2;
  }
  restore_rounding(r);
}
void linear_pcm24_2ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  samples_t src = samples;
  int24_t *dst = (int24_t *)rawdata;

  int r = set_rounding();
  while (size--)
  {
    dst[0] = int2le24(s2i(*src[0])); src[0]++;
    dst[1] = int2le24(s2i(*src[1])); src[1]++;
    dst += 2;
  }
  restore_rounding(r);
}
void linear_pcm32_2ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  samples_t src = samples;
  int32_t *dst = (int32_t *)rawdata;

  int r = set_rounding();
  while (size--)
  {
    dst[0] = int2le32(s2i(*src[0])); src[0]++;
    dst[1] = int2le32(s2i(*src[1])); src[1]++;
    dst += 2;
  }
  restore_rounding(r);
}
void linear_pcm16_be_2ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  samples_t src = samples;
  int16_t *dst = (int16_t *)rawdata;

  int r = set_rounding();
  while (size--)
  {
    dst[0] = int2be16(s2i(*src[0])); src[0]++;
    dst[1] = int2be16(s2i(*src[1])); src[1]++;
    dst += 2;
  }
  restore_rounding(r);
}
void linear_pcm24_be_2ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  samples_t src = samples;
  int24_t *dst = (int24_t *)rawdata;

  int r = set_rounding();
  while (size--)
  {
    dst[0] = int2be24(s2i(*src[0])); src[0]++;
    dst[1] = int2be24(s2i(*src[1])); src[1]++;
    dst += 2;
  }
  restore_rounding(r);
}
void linear_pcm32_be_2ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  samples_t src = samples;
  int32_t *dst = (int32_t *)rawdata;

  int r = set_rounding();
  while (size--)
  {
    dst[0] = int2be32(s2i(*src[0])); src[0]++;
    dst[1] = int2be32(s2i(*src[1])); src[1]++;
    dst += 2;
  }
  restore_rounding(r);
}
void linear_pcmfloat_2ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  samples_t src = samples;
  float *dst = (float *)rawdata;

  int r = set_rounding();
  while (size--)
  {
    dst[0] = float(*src[0]); src[0]++;
    dst[1] = float(*src[1]); src[1]++;
    dst += 2;
  }
  restore_rounding(r);
}
void linear_pcmdouble_2ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  samples_t src = samples;
  double *dst = (double *)rawdata;

  int r = set_rounding();
  while (size--)
  {
    dst[0] = double(*src[0]); src[0]++;
    dst[1] = double(*src[1]); src[1]++;
    dst += 2;
  }
  restore_rounding(r);
}
void linear_pcm16_3ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  samples_t src = samples;
  int16_t *dst = (int16_t *)rawdata;

  int r = set_rounding();
  while (size--)
  {
    dst[0] = int2le16(s2i(*src[0])); src[0]++;
    dst[1] = int2le16(s2i(*src[1])); src[1]++;
    dst[2] = int2le16(s2i(*src[2])); src[2]++;
    dst += 3;
  }
  restore_rounding(r);
}
void linear_pcm24_3ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  samples_t src = samples;
  int24_t *dst = (int24_t *)rawdata;

  int r = set_rounding();
  while (size--)
  {
    dst[0] = int2le24(s2i(*src[0])); src[0]++;
    dst[1] = int2le24(s2i(*src[1])); src[1]++;
    dst[2] = int2le24(s2i(*src[2])); src[2]++;
    dst += 3;
  }
  restore_rounding(r);
}
void linear_pcm32_3ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  samples_t src = samples;
  int32_t *dst = (int32_t *)rawdata;

  int r = set_rounding();
  while (size--)
  {
    dst[0] = int2le32(s2i(*src[0])); src[0]++;
    dst[1] = int2le32(s2i(*src[1])); src[1]++;
    dst[2] = int2le32(s2i(*src[2])); src[2]++;
    dst += 3;
  }
  restore_rounding(r);
}
void linear_pcm16_be_3ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  samples_t src = samples;
  int16_t *dst = (int16_t *)rawdata;

  int r = set_rounding();
  while (size--)
  {
    dst[0] = int2be16(s2i(*src[0])); src[0]++;
    dst[1] = int2be16(s2i(*src[1])); src[1]++;
    dst[2] = int2be16(s2i(*src[2])); src[2]++;
    dst += 3;
  }
  restore_rounding(r);
}
void linear_pcm24_be_3ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  samples_t src = samples;
  int24_t *dst = (int24_t *)rawdata;

  int r = set_rounding();
  while (size--)
  {
    dst[0] = int2be24(s2i(*src[0])); src[0]++;
    dst[1] = int2be24(s2i(*src[1])); src[1]++;
    dst[2] = int2be24(s2i(*src[2])); src[2]++;
    dst += 3;
  }
  restore_rounding(r);
}
void linear_pcm32_be_3ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  samples_t src = samples;
  int32_t *dst = (int32_t *)rawdata;

  int r = set_rounding();
  while (size--)
  {
    dst[0] = int2be32(s2i(*src[0])); src[0]++;
    dst[1] = int2be32(s2i(*src[1])); src[1]++;
    dst[2] = int2be32(s2i(*src[2])); src[2]++;
    dst += 3;
  }
  restore_rounding(r);
}
void linear_pcmfloat_3ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  samples_t src = samples;
  float *dst = (float *)rawdata;

  int r = set_rounding();
  while (size--)
  {
    dst[0] = float(*src[0]); src[0]++;
    dst[1] = float(*src[1]); src[1]++;
    dst[2] = float(*src[2]); src[2]++;
    dst += 3;
  }
  restore_rounding(r);
}
void linear_pcmdouble_3ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  samples_t src = samples;
  double *dst = (double *)rawdata;

  int r = set_rounding();
  while (size--)
  {
    dst[0] = double(*src[0]); src[0]++;
    dst[1] = double(*src[1]); src[1]++;
    dst[2] = double(*src[2]); src[2]++;
    dst += 3;
  }
  restore_rounding(r);
}
void linear_pcm16_4ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  samples_t src = samples;
  int16_t *dst = (int16_t *)rawdata;

  int r = set_rounding();
  while (size--)
  {
    dst[0] = int2le16(s2i(*src[0])); src[0]++;
    dst[1] = int2le16(s2i(*src[1])); src[1]++;
    dst[2] = int2le16(s2i(*src[2])); src[2]++;
    dst[3] = int2le16(s2i(*src[3])); src[3]++;
    dst += 4;
  }
  restore_rounding(r);
}
void linear_pcm24_4ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  samples_t src = samples;
  int24_t *dst = (int24_t *)rawdata;

  int r = set_rounding();
  while (size--)
  {
    dst[0] = int2le24(s2i(*src[0])); src[0]++;
    dst[1] = int2le24(s2i(*src[1])); src[1]++;
    dst[2] = int2le24(s2i(*src[2])); src[2]++;
    dst[3] = int2le24(s2i(*src[3])); src[3]++;
    dst += 4;
  }
  restore_rounding(r);
}
void linear_pcm32_4ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  samples_t src = samples;
  int32_t *dst = (int32_t *)rawdata;

  int r = set_rounding();
  while (size--)
  {
    dst[0] = int2le32(s2i(*src[0])); src[0]++;
    dst[1] = int2le32(s2i(*src[1])); src[1]++;
    dst[2] = int2le32(s2i(*src[2])); src[2]++;
    dst[3] = int2le32(s2i(*src[3])); src[3]++;
    dst += 4;
  }
  restore_rounding(r);
}
void linear_pcm16_be_4ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  samples_t src = samples;
  int16_t *dst = (int16_t *)rawdata;

  int r = set_rounding();
  while (size--)
  {
    dst[0] = int2be16(s2i(*src[0])); src[0]++;
    dst[1] = int2be16(s2i(*src[1])); src[1]++;
    dst[2] = int2be16(s2i(*src[2])); src[2]++;
    dst[3] = int2be16(s2i(*src[3])); src[3]++;
    dst += 4;
  }
  restore_rounding(r);
}
void linear_pcm24_be_4ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  samples_t src = samples;
  int24_t *dst = (int24_t *)rawdata;

  int r = set_rounding();
  while (size--)
  {
    dst[0] = int2be24(s2i(*src[0])); src[0]++;
    dst[1] = int2be24(s2i(*src[1])); src[1]++;
    dst[2] = int2be24(s2i(*src[2])); src[2]++;
    dst[3] = int2be24(s2i(*src[3])); src[3]++;
    dst += 4;
  }
  restore_rounding(r);
}
void linear_pcm32_be_4ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  samples_t src = samples;
  int32_t *dst = (int32_t *)rawdata;

  int r = set_rounding();
  while (size--)
  {
    dst[0] = int2be32(s2i(*src[0])); src[0]++;
    dst[1] = int2be32(s2i(*src[1])); src[1]++;
    dst[2] = int2be32(s2i(*src[2])); src[2]++;
    dst[3] = int2be32(s2i(*src[3])); src[3]++;
    dst += 4;
  }
  restore_rounding(r);
}
void linear_pcmfloat_4ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  samples_t src = samples;
  float *dst = (float *)rawdata;

  int r = set_rounding();
  while (size--)
  {
    dst[0] = float(*src[0]); src[0]++;
    dst[1] = float(*src[1]); src[1]++;
    dst[2] = float(*src[2]); src[2]++;
    dst[3] = float(*src[3]); src[3]++;
    dst += 4;
  }
  restore_rounding(r);
}
void linear_pcmdouble_4ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  samples_t src = samples;
  double *dst = (double *)rawdata;

  int r = set_rounding();
  while (size--)
  {
    dst[0] = double(*src[0]); src[0]++;
    dst[1] = double(*src[1]); src[1]++;
    dst[2] = double(*src[2]); src[2]++;
    dst[3] = double(*src[3]); src[3]++;
    dst += 4;
  }
  restore_rounding(r);
}
void linear_pcm16_5ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  samples_t src = samples;
  int16_t *dst = (int16_t *)rawdata;

  int r = set_rounding();
  while (size--)
  {
    dst[0] = int2le16(s2i(*src[0])); src[0]++;
    dst[1] = int2le16(s2i(*src[1])); src[1]++;
    dst[2] = int2le16(s2i(*src[2])); src[2]++;
    dst[3] = int2le16(s2i(*src[3])); src[3]++;
    dst[4] = int2le16(s2i(*src[4])); src[4]++;
    dst += 5;
  }
  restore_rounding(r);
}
void linear_pcm24_5ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  samples_t src = samples;
  int24_t *dst = (int24_t *)rawdata;

  int r = set_rounding();
  while (size--)
  {
    dst[0] = int2le24(s2i(*src[0])); src[0]++;
    dst[1] = int2le24(s2i(*src[1])); src[1]++;
    dst[2] = int2le24(s2i(*src[2])); src[2]++;
    dst[3] = int2le24(s2i(*src[3])); src[3]++;
    dst[4] = int2le24(s2i(*src[4])); src[4]++;
    dst += 5;
  }
  restore_rounding(r);
}
void linear_pcm32_5ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  samples_t src = samples;
  int32_t *dst = (int32_t *)rawdata;

  int r = set_rounding();
  while (size--)
  {
    dst[0] = int2le32(s2i(*src[0])); src[0]++;
    dst[1] = int2le32(s2i(*src[1])); src[1]++;
    dst[2] = int2le32(s2i(*src[2])); src[2]++;
    dst[3] = int2le32(s2i(*src[3])); src[3]++;
    dst[4] = int2le32(s2i(*src[4])); src[4]++;
    dst += 5;
  }
  restore_rounding(r);
}
void linear_pcm16_be_5ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  samples_t src = samples;
  int16_t *dst = (int16_t *)rawdata;

  int r = set_rounding();
  while (size--)
  {
    dst[0] = int2be16(s2i(*src[0])); src[0]++;
    dst[1] = int2be16(s2i(*src[1])); src[1]++;
    dst[2] = int2be16(s2i(*src[2])); src[2]++;
    dst[3] = int2be16(s2i(*src[3])); src[3]++;
    dst[4] = int2be16(s2i(*src[4])); src[4]++;
    dst += 5;
  }
  restore_rounding(r);
}
void linear_pcm24_be_5ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  samples_t src = samples;
  int24_t *dst = (int24_t *)rawdata;

  int r = set_rounding();
  while (size--)
  {
    dst[0] = int2be24(s2i(*src[0])); src[0]++;
    dst[1] = int2be24(s2i(*src[1])); src[1]++;
    dst[2] = int2be24(s2i(*src[2])); src[2]++;
    dst[3] = int2be24(s2i(*src[3])); src[3]++;
    dst[4] = int2be24(s2i(*src[4])); src[4]++;
    dst += 5;
  }
  restore_rounding(r);
}
void linear_pcm32_be_5ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  samples_t src = samples;
  int32_t *dst = (int32_t *)rawdata;

  int r = set_rounding();
  while (size--)
  {
    dst[0] = int2be32(s2i(*src[0])); src[0]++;
    dst[1] = int2be32(s2i(*src[1])); src[1]++;
    dst[2] = int2be32(s2i(*src[2])); src[2]++;
    dst[3] = int2be32(s2i(*src[3])); src[3]++;
    dst[4] = int2be32(s2i(*src[4])); src[4]++;
    dst += 5;
  }
  restore_rounding(r);
}
void linear_pcmfloat_5ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  samples_t src = samples;
  float *dst = (float *)rawdata;

  int r = set_rounding();
  while (size--)
  {
    dst[0] = float(*src[0]); src[0]++;
    dst[1] = float(*src[1]); src[1]++;
    dst[2] = float(*src[2]); src[2]++;
    dst[3] = float(*src[3]); src[3]++;
    dst[4] = float(*src[4]); src[4]++;
    dst += 5;
  }
  restore_rounding(r);
}
void linear_pcmdouble_5ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  samples_t src = samples;
  double *dst = (double *)rawdata;

  int r = set_rounding();
  while (size--)
  {
    dst[0] = double(*src[0]); src[0]++;
    dst[1] = double(*src[1]); src[1]++;
    dst[2] = double(*src[2]); src[2]++;
    dst[3] = double(*src[3]); src[3]++;
    dst[4] = double(*src[4]); src[4]++;
    dst += 5;
  }
  restore_rounding(r);
}
void linear_pcm16_6ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  samples_t src = samples;
  int16_t *dst = (int16_t *)rawdata;

  int r = set_rounding();
  while (size--)
  {
    dst[0] = int2le16(s2i(*src[0])); src[0]++;
    dst[1] = int2le16(s2i(*src[1])); src[1]++;
    dst[2] = int2le16(s2i(*src[2])); src[2]++;
    dst[3] = int2le16(s2i(*src[3])); src[3]++;
    dst[4] = int2le16(s2i(*src[4])); src[4]++;
    dst[5] = int2le16(s2i(*src[5])); src[5]++;
    dst += 6;
  }
  restore_rounding(r);
}
void linear_pcm24_6ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  samples_t src = samples;
  int24_t *dst = (int24_t *)rawdata;

  int r = set_rounding();
  while (size--)
  {
    dst[0] = int2le24(s2i(*src[0])); src[0]++;
    dst[1] = int2le24(s2i(*src[1])); src[1]++;
    dst[2] = int2le24(s2i(*src[2])); src[2]++;
    dst[3] = int2le24(s2i(*src[3])); src[3]++;
    dst[4] = int2le24(s2i(*src[4])); src[4]++;
    dst[5] = int2le24(s2i(*src[5])); src[5]++;
    dst += 6;
  }
  restore_rounding(r);
}
void linear_pcm32_6ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  samples_t src = samples;
  int32_t *dst = (int32_t *)rawdata;

  int r = set_rounding();
  while (size--)
  {
    dst[0] = int2le32(s2i(*src[0])); src[0]++;
    dst[1] = int2le32(s2i(*src[1])); src[1]++;
    dst[2] = int2le32(s2i(*src[2])); src[2]++;
    dst[3] = int2le32(s2i(*src[3])); src[3]++;
    dst[4] = int2le32(s2i(*src[4])); src[4]++;
    dst[5] = int2le32(s2i(*src[5])); src[5]++;
    dst += 6;
  }
  restore_rounding(r);
}
void linear_pcm16_be_6ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  samples_t src = samples;
  int16_t *dst = (int16_t *)rawdata;

  int r = set_rounding();
  while (size--)
  {
    dst[0] = int2be16(s2i(*src[0])); src[0]++;
    dst[1] = int2be16(s2i(*src[1])); src[1]++;
    dst[2] = int2be16(s2i(*src[2])); src[2]++;
    dst[3] = int2be16(s2i(*src[3])); src[3]++;
    dst[4] = int2be16(s2i(*src[4])); src[4]++;
    dst[5] = int2be16(s2i(*src[5])); src[5]++;
    dst += 6;
  }
  restore_rounding(r);
}
void linear_pcm24_be_6ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  samples_t src = samples;
  int24_t *dst = (int24_t *)rawdata;

  int r = set_rounding();
  while (size--)
  {
    dst[0] = int2be24(s2i(*src[0])); src[0]++;
    dst[1] = int2be24(s2i(*src[1])); src[1]++;
    dst[2] = int2be24(s2i(*src[2])); src[2]++;
    dst[3] = int2be24(s2i(*src[3])); src[3]++;
    dst[4] = int2be24(s2i(*src[4])); src[4]++;
    dst[5] = int2be24(s2i(*src[5])); src[5]++;
    dst += 6;
  }
  restore_rounding(r);
}
void linear_pcm32_be_6ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  samples_t src = samples;
  int32_t *dst = (int32_t *)rawdata;

  int r = set_rounding();
  while (size--)
  {
    dst[0] = int2be32(s2i(*src[0])); src[0]++;
    dst[1] = int2be32(s2i(*src[1])); src[1]++;
    dst[2] = int2be32(s2i(*src[2])); src[2]++;
    dst[3] = int2be32(s2i(*src[3])); src[3]++;
    dst[4] = int2be32(s2i(*src[4])); src[4]++;
    dst[5] = int2be32(s2i(*src[5])); src[5]++;
    dst += 6;
  }
  restore_rounding(r);
}
void linear_pcmfloat_6ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  samples_t src = samples;
  float *dst = (float *)rawdata;

  int r = set_rounding();
  while (size--)
  {
    dst[0] = float(*src[0]); src[0]++;
    dst[1] = float(*src[1]); src[1]++;
    dst[2] = float(*src[2]); src[2]++;
    dst[3] = float(*src[3]); src[3]++;
    dst[4] = float(*src[4]); src[4]++;
    dst[5] = float(*src[5]); src[5]++;
    dst += 6;
  }
  restore_rounding(r);
}
void linear_pcmdouble_6ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  samples_t src = samples;
  double *dst = (double *)rawdata;

  int r = set_rounding();
  while (size--)
  {
    dst[0] = double(*src[0]); src[0]++;
    dst[1] = double(*src[1]); src[1]++;
    dst[2] = double(*src[2]); src[2]++;
    dst[3] = double(*src[3]); src[3]++;
    dst[4] = double(*src[4]); src[4]++;
    dst[5] = double(*src[5]); src[5]++;
    dst += 6;
  }
  restore_rounding(r);
}
void linear_pcm16_7ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  samples_t src = samples;
  int16_t *dst = (int16_t *)rawdata;

  int r = set_rounding();
  while (size--)
  {
    dst[0] = int2le16(s2i(*src[0])); src[0]++;
    dst[1] = int2le16(s2i(*src[1])); src[1]++;
    dst[2] = int2le16(s2i(*src[2])); src[2]++;
    dst[3] = int2le16(s2i(*src[3])); src[3]++;
    dst[4] = int2le16(s2i(*src[4])); src[4]++;
    dst[5] = int2le16(s2i(*src[5])); src[5]++;
    dst[6] = int2le16(s2i(*src[6])); src[6]++;
    dst += 7;
  }
  restore_rounding(r);
}
void linear_pcm24_7ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  samples_t src = samples;
  int24_t *dst = (int24_t *)rawdata;

  int r = set_rounding();
  while (size--)
  {
    dst[0] = int2le24(s2i(*src[0])); src[0]++;
    dst[1] = int2le24(s2i(*src[1])); src[1]++;
    dst[2] = int2le24(s2i(*src[2])); src[2]++;
    dst[3] = int2le24(s2i(*src[3])); src[3]++;
    dst[4] = int2le24(s2i(*src[4])); src[4]++;
    dst[5] = int2le24(s2i(*src[5])); src[5]++;
    dst[6] = int2le24(s2i(*src[6])); src[6]++;
    dst += 7;
  }
  restore_rounding(r);
}
void linear_pcm32_7ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  samples_t src = samples;
  int32_t *dst = (int32_t *)rawdata;

  int r = set_rounding();
  while (size--)
  {
    dst[0] = int2le32(s2i(*src[0])); src[0]++;
    dst[1] = int2le32(s2i(*src[1])); src[1]++;
    dst[2] = int2le32(s2i(*src[2])); src[2]++;
    dst[3] = int2le32(s2i(*src[3])); src[3]++;
    dst[4] = int2le32(s2i(*src[4])); src[4]++;
    dst[5] = int2le32(s2i(*src[5])); src[5]++;
    dst[6] = int2le32(s2i(*src[6])); src[6]++;
    dst += 7;
  }
  restore_rounding(r);
}
void linear_pcm16_be_7ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  samples_t src = samples;
  int16_t *dst = (int16_t *)rawdata;

  int r = set_rounding();
  while (size--)
  {
    dst[0] = int2be16(s2i(*src[0])); src[0]++;
    dst[1] = int2be16(s2i(*src[1])); src[1]++;
    dst[2] = int2be16(s2i(*src[2])); src[2]++;
    dst[3] = int2be16(s2i(*src[3])); src[3]++;
    dst[4] = int2be16(s2i(*src[4])); src[4]++;
    dst[5] = int2be16(s2i(*src[5])); src[5]++;
    dst[6] = int2be16(s2i(*src[6])); src[6]++;
    dst += 7;
  }
  restore_rounding(r);
}
void linear_pcm24_be_7ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  samples_t src = samples;
  int24_t *dst = (int24_t *)rawdata;

  int r = set_rounding();
  while (size--)
  {
    dst[0] = int2be24(s2i(*src[0])); src[0]++;
    dst[1] = int2be24(s2i(*src[1])); src[1]++;
    dst[2] = int2be24(s2i(*src[2])); src[2]++;
    dst[3] = int2be24(s2i(*src[3])); src[3]++;
    dst[4] = int2be24(s2i(*src[4])); src[4]++;
    dst[5] = int2be24(s2i(*src[5])); src[5]++;
    dst[6] = int2be24(s2i(*src[6])); src[6]++;
    dst += 7;
  }
  restore_rounding(r);
}
void linear_pcm32_be_7ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  samples_t src = samples;
  int32_t *dst = (int32_t *)rawdata;

  int r = set_rounding();
  while (size--)
  {
    dst[0] = int2be32(s2i(*src[0])); src[0]++;
    dst[1] = int2be32(s2i(*src[1])); src[1]++;
    dst[2] = int2be32(s2i(*src[2])); src[2]++;
    dst[3] = int2be32(s2i(*src[3])); src[3]++;
    dst[4] = int2be32(s2i(*src[4])); src[4]++;
    dst[5] = int2be32(s2i(*src[5])); src[5]++;
    dst[6] = int2be32(s2i(*src[6])); src[6]++;
    dst += 7;
  }
  restore_rounding(r);
}
void linear_pcmfloat_7ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  samples_t src = samples;
  float *dst = (float *)rawdata;

  int r = set_rounding();
  while (size--)
  {
    dst[0] = float(*src[0]); src[0]++;
    dst[1] = float(*src[1]); src[1]++;
    dst[2] = float(*src[2]); src[2]++;
    dst[3] = float(*src[3]); src[3]++;
    dst[4] = float(*src[4]); src[4]++;
    dst[5] = float(*src[5]); src[5]++;
    dst[6] = float(*src[6]); src[6]++;
    dst += 7;
  }
  restore_rounding(r);
}
void linear_pcmdouble_7ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  samples_t src = samples;
  double *dst = (double *)rawdata;

  int r = set_rounding();
  while (size--)
  {
    dst[0] = double(*src[0]); src[0]++;
    dst[1] = double(*src[1]); src[1]++;
    dst[2] = double(*src[2]); src[2]++;
    dst[3] = double(*src[3]); src[3]++;
    dst[4] = double(*src[4]); src[4]++;
    dst[5] = double(*src[5]); src[5]++;
    dst[6] = double(*src[6]); src[6]++;
    dst += 7;
  }
  restore_rounding(r);
}
void linear_pcm16_8ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  samples_t src = samples;
  int16_t *dst = (int16_t *)rawdata;

  int r = set_rounding();
  while (size--)
  {
    dst[0] = int2le16(s2i(*src[0])); src[0]++;
    dst[1] = int2le16(s2i(*src[1])); src[1]++;
    dst[2] = int2le16(s2i(*src[2])); src[2]++;
    dst[3] = int2le16(s2i(*src[3])); src[3]++;
    dst[4] = int2le16(s2i(*src[4])); src[4]++;
    dst[5] = int2le16(s2i(*src[5])); src[5]++;
    dst[6] = int2le16(s2i(*src[6])); src[6]++;
    dst[7] = int2le16(s2i(*src[7])); src[7]++;
    dst += 8;
  }
  restore_rounding(r);
}
void linear_pcm24_8ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  samples_t src = samples;
  int24_t *dst = (int24_t *)rawdata;

  int r = set_rounding();
  while (size--)
  {
    dst[0] = int2le24(s2i(*src[0])); src[0]++;
    dst[1] = int2le24(s2i(*src[1])); src[1]++;
    dst[2] = int2le24(s2i(*src[2])); src[2]++;
    dst[3] = int2le24(s2i(*src[3])); src[3]++;
    dst[4] = int2le24(s2i(*src[4])); src[4]++;
    dst[5] = int2le24(s2i(*src[5])); src[5]++;
    dst[6] = int2le24(s2i(*src[6])); src[6]++;
    dst[7] = int2le24(s2i(*src[7])); src[7]++;
    dst += 8;
  }
  restore_rounding(r);
}
void linear_pcm32_8ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  samples_t src = samples;
  int32_t *dst = (int32_t *)rawdata;

  int r = set_rounding();
  while (size--)
  {
    dst[0] = int2le32(s2i(*src[0])); src[0]++;
    dst[1] = int2le32(s2i(*src[1])); src[1]++;
    dst[2] = int2le32(s2i(*src[2])); src[2]++;
    dst[3] = int2le32(s2i(*src[3])); src[3]++;
    dst[4] = int2le32(s2i(*src[4])); src[4]++;
    dst[5] = int2le32(s2i(*src[5])); src[5]++;
    dst[6] = int2le32(s2i(*src[6])); src[6]++;
    dst[7] = int2le32(s2i(*src[7])); src[7]++;
    dst += 8;
  }
  restore_rounding(r);
}
void linear_pcm16_be_8ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  samples_t src = samples;
  int16_t *dst = (int16_t *)rawdata;

  int r = set_rounding();
  while (size--)
  {
    dst[0] = int2be16(s2i(*src[0])); src[0]++;
    dst[1] = int2be16(s2i(*src[1])); src[1]++;
    dst[2] = int2be16(s2i(*src[2])); src[2]++;
    dst[3] = int2be16(s2i(*src[3])); src[3]++;
    dst[4] = int2be16(s2i(*src[4])); src[4]++;
    dst[5] = int2be16(s2i(*src[5])); src[5]++;
    dst[6] = int2be16(s2i(*src[6])); src[6]++;
    dst[7] = int2be16(s2i(*src[7])); src[7]++;
    dst += 8;
  }
  restore_rounding(r);
}
void linear_pcm24_be_8ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  samples_t src = samples;
  int24_t *dst = (int24_t *)rawdata;

  int r = set_rounding();
  while (size--)
  {
    dst[0] = int2be24(s2i(*src[0])); src[0]++;
    dst[1] = int2be24(s2i(*src[1])); src[1]++;
    dst[2] = int2be24(s2i(*src[2])); src[2]++;
    dst[3] = int2be24(s2i(*src[3])); src[3]++;
    dst[4] = int2be24(s2i(*src[4])); src[4]++;
    dst[5] = int2be24(s2i(*src[5])); src[5]++;
    dst[6] = int2be24(s2i(*src[6])); src[6]++;
    dst[7] = int2be24(s2i(*src[7])); src[7]++;
    dst += 8;
  }
  restore_rounding(r);
}
void linear_pcm32_be_8ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  samples_t src = samples;
  int32_t *dst = (int32_t *)rawdata;

  int r = set_rounding();
  while (size--)
  {
    dst[0] = int2be32(s2i(*src[0])); src[0]++;
    dst[1] = int2be32(s2i(*src[1])); src[1]++;
    dst[2] = int2be32(s2i(*src[2])); src[2]++;
    dst[3] = int2be32(s2i(*src[3])); src[3]++;
    dst[4] = int2be32(s2i(*src[4])); src[4]++;
    dst[5] = int2be32(s2i(*src[5])); src[5]++;
    dst[6] = int2be32(s2i(*src[6])); src[6]++;
    dst[7] = int2be32(s2i(*src[7])); src[7]++;
    dst += 8;
  }
  restore_rounding(r);
}
void linear_pcmfloat_8ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  samples_t src = samples;
  float *dst = (float *)rawdata;

  int r = set_rounding();
  while (size--)
  {
    dst[0] = float(*src[0]); src[0]++;
    dst[1] = float(*src[1]); src[1]++;
    dst[2] = float(*src[2]); src[2]++;
    dst[3] = float(*src[3]); src[3]++;
    dst[4] = float(*src[4]); src[4]++;
    dst[5] = float(*src[5]); src[5]++;
    dst[6] = float(*src[6]); src[6]++;
    dst[7] = float(*src[7]); src[7]++;
    dst += 8;
  }
  restore_rounding(r);
}
void linear_pcmdouble_8ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  samples_t src = samples;
  double *dst = (double *)rawdata;

  int r = set_rounding();
  while (size--)
  {
    dst[0] = double(*src[0]); src[0]++;
    dst[1] = double(*src[1]); src[1]++;
    dst[2] = double(*src[2]); src[2]++;
    dst[3] = double(*src[3]); src[3]++;
    dst[4] = double(*src[4]); src[4]++;
    dst[5] = double(*src[5]); src[5]++;
    dst[6] = double(*src[6]); src[6]++;
    dst[7] = double(*src[7]); src[7]++;
    dst += 8;
  }
  restore_rounding(r);
}
