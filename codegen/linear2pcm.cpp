void linear_pcm16_1ch(uint8_t *, samples_t, size_t);
void linear_pcm24_1ch(uint8_t *, samples_t, size_t);
void linear_pcm32_1ch(uint8_t *, samples_t, size_t);
void linear_pcm16_be_1ch(uint8_t *, samples_t, size_t);
void linear_pcm24_be_1ch(uint8_t *, samples_t, size_t);
void linear_pcm32_be_1ch(uint8_t *, samples_t, size_t);
void linear_pcmfloat_1ch(uint8_t *, samples_t, size_t);

void linear_pcm16_2ch(uint8_t *, samples_t, size_t);
void linear_pcm24_2ch(uint8_t *, samples_t, size_t);
void linear_pcm32_2ch(uint8_t *, samples_t, size_t);
void linear_pcm16_be_2ch(uint8_t *, samples_t, size_t);
void linear_pcm24_be_2ch(uint8_t *, samples_t, size_t);
void linear_pcm32_be_2ch(uint8_t *, samples_t, size_t);
void linear_pcmfloat_2ch(uint8_t *, samples_t, size_t);

void linear_pcm16_3ch(uint8_t *, samples_t, size_t);
void linear_pcm24_3ch(uint8_t *, samples_t, size_t);
void linear_pcm32_3ch(uint8_t *, samples_t, size_t);
void linear_pcm16_be_3ch(uint8_t *, samples_t, size_t);
void linear_pcm24_be_3ch(uint8_t *, samples_t, size_t);
void linear_pcm32_be_3ch(uint8_t *, samples_t, size_t);
void linear_pcmfloat_3ch(uint8_t *, samples_t, size_t);

void linear_pcm16_4ch(uint8_t *, samples_t, size_t);
void linear_pcm24_4ch(uint8_t *, samples_t, size_t);
void linear_pcm32_4ch(uint8_t *, samples_t, size_t);
void linear_pcm16_be_4ch(uint8_t *, samples_t, size_t);
void linear_pcm24_be_4ch(uint8_t *, samples_t, size_t);
void linear_pcm32_be_4ch(uint8_t *, samples_t, size_t);
void linear_pcmfloat_4ch(uint8_t *, samples_t, size_t);

void linear_pcm16_5ch(uint8_t *, samples_t, size_t);
void linear_pcm24_5ch(uint8_t *, samples_t, size_t);
void linear_pcm32_5ch(uint8_t *, samples_t, size_t);
void linear_pcm16_be_5ch(uint8_t *, samples_t, size_t);
void linear_pcm24_be_5ch(uint8_t *, samples_t, size_t);
void linear_pcm32_be_5ch(uint8_t *, samples_t, size_t);
void linear_pcmfloat_5ch(uint8_t *, samples_t, size_t);

void linear_pcm16_6ch(uint8_t *, samples_t, size_t);
void linear_pcm24_6ch(uint8_t *, samples_t, size_t);
void linear_pcm32_6ch(uint8_t *, samples_t, size_t);
void linear_pcm16_be_6ch(uint8_t *, samples_t, size_t);
void linear_pcm24_be_6ch(uint8_t *, samples_t, size_t);
void linear_pcm32_be_6ch(uint8_t *, samples_t, size_t);
void linear_pcmfloat_6ch(uint8_t *, samples_t, size_t);

typedef void (*convert_t)(uint8_t *, samples_t, size_t);

static const int formats_tbl[] = { FORMAT_PCM16, FORMAT_PCM24, FORMAT_PCM32, FORMAT_PCM16_BE, FORMAT_PCM24_BE, FORMAT_PCM32_BE, FORMAT_PCMFLOAT };

static const int formats = FORMAT_PCM16 | FORMAT_PCM24 | FORMAT_PCM32 | FORMAT_PCM16_BE | FORMAT_PCM24_BE | FORMAT_PCM32_BE | FORMAT_PCMFLOAT;

static const convert_t linear2pcm_tbl[NCHANNELS][7] = {
 { linear_pcm16_1ch, linear_pcm24_1ch, linear_pcm32_1ch, linear_pcm16_be_1ch, linear_pcm24_be_1ch, linear_pcm32_be_1ch, linear_pcmfloat_1ch },
 { linear_pcm16_2ch, linear_pcm24_2ch, linear_pcm32_2ch, linear_pcm16_be_2ch, linear_pcm24_be_2ch, linear_pcm32_be_2ch, linear_pcmfloat_2ch },
 { linear_pcm16_3ch, linear_pcm24_3ch, linear_pcm32_3ch, linear_pcm16_be_3ch, linear_pcm24_be_3ch, linear_pcm32_be_3ch, linear_pcmfloat_3ch },
 { linear_pcm16_4ch, linear_pcm24_4ch, linear_pcm32_4ch, linear_pcm16_be_4ch, linear_pcm24_be_4ch, linear_pcm32_be_4ch, linear_pcmfloat_4ch },
 { linear_pcm16_5ch, linear_pcm24_5ch, linear_pcm32_5ch, linear_pcm16_be_5ch, linear_pcm24_be_5ch, linear_pcm32_be_5ch, linear_pcmfloat_5ch },
 { linear_pcm16_6ch, linear_pcm24_6ch, linear_pcm32_6ch, linear_pcm16_be_6ch, linear_pcm24_be_6ch, linear_pcm32_be_6ch, linear_pcmfloat_6ch },
};

void
linear_pcm16_1ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  const int nch = 1;
  samples_t src = samples;
  int16_t *dst = (int16_t *)rawdata;

  int r = set_rounding();  
  while (size--)
  {
    dst[0] = int2le16(s2i16(src[0][0])); src[0]++;

    dst += nch;
  }
  restore_rounding(r);
}
void
linear_pcm24_1ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  const int nch = 1;
  samples_t src = samples;
  int24_t *dst = (int24_t *)rawdata;

  int r = set_rounding();  
  while (size--)
  {
    dst[0] = int2le24(s2i24(src[0][0])); src[0]++;

    dst += nch;
  }
  restore_rounding(r);
}
void
linear_pcm32_1ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  const int nch = 1;
  samples_t src = samples;
  int32_t *dst = (int32_t *)rawdata;

  int r = set_rounding();  
  while (size--)
  {
    dst[0] = int2le32(s2i32(src[0][0])); src[0]++;

    dst += nch;
  }
  restore_rounding(r);
}
void
linear_pcm16_be_1ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  const int nch = 1;
  samples_t src = samples;
  int16_t *dst = (int16_t *)rawdata;

  int r = set_rounding();  
  while (size--)
  {
    dst[0] = int2be16(s2i16(src[0][0])); src[0]++;

    dst += nch;
  }
  restore_rounding(r);
}
void
linear_pcm24_be_1ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  const int nch = 1;
  samples_t src = samples;
  int24_t *dst = (int24_t *)rawdata;

  int r = set_rounding();  
  while (size--)
  {
    dst[0] = int2be24(s2i24(src[0][0])); src[0]++;

    dst += nch;
  }
  restore_rounding(r);
}
void
linear_pcm32_be_1ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  const int nch = 1;
  samples_t src = samples;
  int32_t *dst = (int32_t *)rawdata;

  int r = set_rounding();  
  while (size--)
  {
    dst[0] = int2be32(s2i32(src[0][0])); src[0]++;

    dst += nch;
  }
  restore_rounding(r);
}
void
linear_pcmfloat_1ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  const int nch = 1;
  samples_t src = samples;
  float *dst = (float *)rawdata;

  int r = set_rounding();  
  while (size--)
  {
    dst[0] = ((float)(src[0][0])); src[0]++;

    dst += nch;
  }
  restore_rounding(r);
}

void
linear_pcm16_2ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  const int nch = 2;
  samples_t src = samples;
  int16_t *dst = (int16_t *)rawdata;

  int r = set_rounding();  
  while (size--)
  {
    dst[0] = int2le16(s2i16(src[0][0])); src[0]++;
    dst[1] = int2le16(s2i16(src[1][0])); src[1]++;

    dst += nch;
  }
  restore_rounding(r);
}
void
linear_pcm24_2ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  const int nch = 2;
  samples_t src = samples;
  int24_t *dst = (int24_t *)rawdata;

  int r = set_rounding();  
  while (size--)
  {
    dst[0] = int2le24(s2i24(src[0][0])); src[0]++;
    dst[1] = int2le24(s2i24(src[1][0])); src[1]++;

    dst += nch;
  }
  restore_rounding(r);
}
void
linear_pcm32_2ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  const int nch = 2;
  samples_t src = samples;
  int32_t *dst = (int32_t *)rawdata;

  int r = set_rounding();  
  while (size--)
  {
    dst[0] = int2le32(s2i32(src[0][0])); src[0]++;
    dst[1] = int2le32(s2i32(src[1][0])); src[1]++;

    dst += nch;
  }
  restore_rounding(r);
}
void
linear_pcm16_be_2ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  const int nch = 2;
  samples_t src = samples;
  int16_t *dst = (int16_t *)rawdata;

  int r = set_rounding();  
  while (size--)
  {
    dst[0] = int2be16(s2i16(src[0][0])); src[0]++;
    dst[1] = int2be16(s2i16(src[1][0])); src[1]++;

    dst += nch;
  }
  restore_rounding(r);
}
void
linear_pcm24_be_2ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  const int nch = 2;
  samples_t src = samples;
  int24_t *dst = (int24_t *)rawdata;

  int r = set_rounding();  
  while (size--)
  {
    dst[0] = int2be24(s2i24(src[0][0])); src[0]++;
    dst[1] = int2be24(s2i24(src[1][0])); src[1]++;

    dst += nch;
  }
  restore_rounding(r);
}
void
linear_pcm32_be_2ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  const int nch = 2;
  samples_t src = samples;
  int32_t *dst = (int32_t *)rawdata;

  int r = set_rounding();  
  while (size--)
  {
    dst[0] = int2be32(s2i32(src[0][0])); src[0]++;
    dst[1] = int2be32(s2i32(src[1][0])); src[1]++;

    dst += nch;
  }
  restore_rounding(r);
}
void
linear_pcmfloat_2ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  const int nch = 2;
  samples_t src = samples;
  float *dst = (float *)rawdata;

  int r = set_rounding();  
  while (size--)
  {
    dst[0] = ((float)(src[0][0])); src[0]++;
    dst[1] = ((float)(src[1][0])); src[1]++;

    dst += nch;
  }
  restore_rounding(r);
}

void
linear_pcm16_3ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  const int nch = 3;
  samples_t src = samples;
  int16_t *dst = (int16_t *)rawdata;

  int r = set_rounding();  
  while (size--)
  {
    dst[0] = int2le16(s2i16(src[0][0])); src[0]++;
    dst[1] = int2le16(s2i16(src[1][0])); src[1]++;
    dst[2] = int2le16(s2i16(src[2][0])); src[2]++;

    dst += nch;
  }
  restore_rounding(r);
}
void
linear_pcm24_3ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  const int nch = 3;
  samples_t src = samples;
  int24_t *dst = (int24_t *)rawdata;

  int r = set_rounding();  
  while (size--)
  {
    dst[0] = int2le24(s2i24(src[0][0])); src[0]++;
    dst[1] = int2le24(s2i24(src[1][0])); src[1]++;
    dst[2] = int2le24(s2i24(src[2][0])); src[2]++;

    dst += nch;
  }
  restore_rounding(r);
}
void
linear_pcm32_3ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  const int nch = 3;
  samples_t src = samples;
  int32_t *dst = (int32_t *)rawdata;

  int r = set_rounding();  
  while (size--)
  {
    dst[0] = int2le32(s2i32(src[0][0])); src[0]++;
    dst[1] = int2le32(s2i32(src[1][0])); src[1]++;
    dst[2] = int2le32(s2i32(src[2][0])); src[2]++;

    dst += nch;
  }
  restore_rounding(r);
}
void
linear_pcm16_be_3ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  const int nch = 3;
  samples_t src = samples;
  int16_t *dst = (int16_t *)rawdata;

  int r = set_rounding();  
  while (size--)
  {
    dst[0] = int2be16(s2i16(src[0][0])); src[0]++;
    dst[1] = int2be16(s2i16(src[1][0])); src[1]++;
    dst[2] = int2be16(s2i16(src[2][0])); src[2]++;

    dst += nch;
  }
  restore_rounding(r);
}
void
linear_pcm24_be_3ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  const int nch = 3;
  samples_t src = samples;
  int24_t *dst = (int24_t *)rawdata;

  int r = set_rounding();  
  while (size--)
  {
    dst[0] = int2be24(s2i24(src[0][0])); src[0]++;
    dst[1] = int2be24(s2i24(src[1][0])); src[1]++;
    dst[2] = int2be24(s2i24(src[2][0])); src[2]++;

    dst += nch;
  }
  restore_rounding(r);
}
void
linear_pcm32_be_3ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  const int nch = 3;
  samples_t src = samples;
  int32_t *dst = (int32_t *)rawdata;

  int r = set_rounding();  
  while (size--)
  {
    dst[0] = int2be32(s2i32(src[0][0])); src[0]++;
    dst[1] = int2be32(s2i32(src[1][0])); src[1]++;
    dst[2] = int2be32(s2i32(src[2][0])); src[2]++;

    dst += nch;
  }
  restore_rounding(r);
}
void
linear_pcmfloat_3ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  const int nch = 3;
  samples_t src = samples;
  float *dst = (float *)rawdata;

  int r = set_rounding();  
  while (size--)
  {
    dst[0] = ((float)(src[0][0])); src[0]++;
    dst[1] = ((float)(src[1][0])); src[1]++;
    dst[2] = ((float)(src[2][0])); src[2]++;

    dst += nch;
  }
  restore_rounding(r);
}

void
linear_pcm16_4ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  const int nch = 4;
  samples_t src = samples;
  int16_t *dst = (int16_t *)rawdata;

  int r = set_rounding();  
  while (size--)
  {
    dst[0] = int2le16(s2i16(src[0][0])); src[0]++;
    dst[1] = int2le16(s2i16(src[1][0])); src[1]++;
    dst[2] = int2le16(s2i16(src[2][0])); src[2]++;
    dst[3] = int2le16(s2i16(src[3][0])); src[3]++;

    dst += nch;
  }
  restore_rounding(r);
}
void
linear_pcm24_4ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  const int nch = 4;
  samples_t src = samples;
  int24_t *dst = (int24_t *)rawdata;

  int r = set_rounding();  
  while (size--)
  {
    dst[0] = int2le24(s2i24(src[0][0])); src[0]++;
    dst[1] = int2le24(s2i24(src[1][0])); src[1]++;
    dst[2] = int2le24(s2i24(src[2][0])); src[2]++;
    dst[3] = int2le24(s2i24(src[3][0])); src[3]++;

    dst += nch;
  }
  restore_rounding(r);
}
void
linear_pcm32_4ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  const int nch = 4;
  samples_t src = samples;
  int32_t *dst = (int32_t *)rawdata;

  int r = set_rounding();  
  while (size--)
  {
    dst[0] = int2le32(s2i32(src[0][0])); src[0]++;
    dst[1] = int2le32(s2i32(src[1][0])); src[1]++;
    dst[2] = int2le32(s2i32(src[2][0])); src[2]++;
    dst[3] = int2le32(s2i32(src[3][0])); src[3]++;

    dst += nch;
  }
  restore_rounding(r);
}
void
linear_pcm16_be_4ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  const int nch = 4;
  samples_t src = samples;
  int16_t *dst = (int16_t *)rawdata;

  int r = set_rounding();  
  while (size--)
  {
    dst[0] = int2be16(s2i16(src[0][0])); src[0]++;
    dst[1] = int2be16(s2i16(src[1][0])); src[1]++;
    dst[2] = int2be16(s2i16(src[2][0])); src[2]++;
    dst[3] = int2be16(s2i16(src[3][0])); src[3]++;

    dst += nch;
  }
  restore_rounding(r);
}
void
linear_pcm24_be_4ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  const int nch = 4;
  samples_t src = samples;
  int24_t *dst = (int24_t *)rawdata;

  int r = set_rounding();  
  while (size--)
  {
    dst[0] = int2be24(s2i24(src[0][0])); src[0]++;
    dst[1] = int2be24(s2i24(src[1][0])); src[1]++;
    dst[2] = int2be24(s2i24(src[2][0])); src[2]++;
    dst[3] = int2be24(s2i24(src[3][0])); src[3]++;

    dst += nch;
  }
  restore_rounding(r);
}
void
linear_pcm32_be_4ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  const int nch = 4;
  samples_t src = samples;
  int32_t *dst = (int32_t *)rawdata;

  int r = set_rounding();  
  while (size--)
  {
    dst[0] = int2be32(s2i32(src[0][0])); src[0]++;
    dst[1] = int2be32(s2i32(src[1][0])); src[1]++;
    dst[2] = int2be32(s2i32(src[2][0])); src[2]++;
    dst[3] = int2be32(s2i32(src[3][0])); src[3]++;

    dst += nch;
  }
  restore_rounding(r);
}
void
linear_pcmfloat_4ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  const int nch = 4;
  samples_t src = samples;
  float *dst = (float *)rawdata;

  int r = set_rounding();  
  while (size--)
  {
    dst[0] = ((float)(src[0][0])); src[0]++;
    dst[1] = ((float)(src[1][0])); src[1]++;
    dst[2] = ((float)(src[2][0])); src[2]++;
    dst[3] = ((float)(src[3][0])); src[3]++;

    dst += nch;
  }
  restore_rounding(r);
}

void
linear_pcm16_5ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  const int nch = 5;
  samples_t src = samples;
  int16_t *dst = (int16_t *)rawdata;

  int r = set_rounding();  
  while (size--)
  {
    dst[0] = int2le16(s2i16(src[0][0])); src[0]++;
    dst[1] = int2le16(s2i16(src[1][0])); src[1]++;
    dst[2] = int2le16(s2i16(src[2][0])); src[2]++;
    dst[3] = int2le16(s2i16(src[3][0])); src[3]++;
    dst[4] = int2le16(s2i16(src[4][0])); src[4]++;

    dst += nch;
  }
  restore_rounding(r);
}
void
linear_pcm24_5ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  const int nch = 5;
  samples_t src = samples;
  int24_t *dst = (int24_t *)rawdata;

  int r = set_rounding();  
  while (size--)
  {
    dst[0] = int2le24(s2i24(src[0][0])); src[0]++;
    dst[1] = int2le24(s2i24(src[1][0])); src[1]++;
    dst[2] = int2le24(s2i24(src[2][0])); src[2]++;
    dst[3] = int2le24(s2i24(src[3][0])); src[3]++;
    dst[4] = int2le24(s2i24(src[4][0])); src[4]++;

    dst += nch;
  }
  restore_rounding(r);
}
void
linear_pcm32_5ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  const int nch = 5;
  samples_t src = samples;
  int32_t *dst = (int32_t *)rawdata;

  int r = set_rounding();  
  while (size--)
  {
    dst[0] = int2le32(s2i32(src[0][0])); src[0]++;
    dst[1] = int2le32(s2i32(src[1][0])); src[1]++;
    dst[2] = int2le32(s2i32(src[2][0])); src[2]++;
    dst[3] = int2le32(s2i32(src[3][0])); src[3]++;
    dst[4] = int2le32(s2i32(src[4][0])); src[4]++;

    dst += nch;
  }
  restore_rounding(r);
}
void
linear_pcm16_be_5ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  const int nch = 5;
  samples_t src = samples;
  int16_t *dst = (int16_t *)rawdata;

  int r = set_rounding();  
  while (size--)
  {
    dst[0] = int2be16(s2i16(src[0][0])); src[0]++;
    dst[1] = int2be16(s2i16(src[1][0])); src[1]++;
    dst[2] = int2be16(s2i16(src[2][0])); src[2]++;
    dst[3] = int2be16(s2i16(src[3][0])); src[3]++;
    dst[4] = int2be16(s2i16(src[4][0])); src[4]++;

    dst += nch;
  }
  restore_rounding(r);
}
void
linear_pcm24_be_5ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  const int nch = 5;
  samples_t src = samples;
  int24_t *dst = (int24_t *)rawdata;

  int r = set_rounding();  
  while (size--)
  {
    dst[0] = int2be24(s2i24(src[0][0])); src[0]++;
    dst[1] = int2be24(s2i24(src[1][0])); src[1]++;
    dst[2] = int2be24(s2i24(src[2][0])); src[2]++;
    dst[3] = int2be24(s2i24(src[3][0])); src[3]++;
    dst[4] = int2be24(s2i24(src[4][0])); src[4]++;

    dst += nch;
  }
  restore_rounding(r);
}
void
linear_pcm32_be_5ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  const int nch = 5;
  samples_t src = samples;
  int32_t *dst = (int32_t *)rawdata;

  int r = set_rounding();  
  while (size--)
  {
    dst[0] = int2be32(s2i32(src[0][0])); src[0]++;
    dst[1] = int2be32(s2i32(src[1][0])); src[1]++;
    dst[2] = int2be32(s2i32(src[2][0])); src[2]++;
    dst[3] = int2be32(s2i32(src[3][0])); src[3]++;
    dst[4] = int2be32(s2i32(src[4][0])); src[4]++;

    dst += nch;
  }
  restore_rounding(r);
}
void
linear_pcmfloat_5ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  const int nch = 5;
  samples_t src = samples;
  float *dst = (float *)rawdata;

  int r = set_rounding();  
  while (size--)
  {
    dst[0] = ((float)(src[0][0])); src[0]++;
    dst[1] = ((float)(src[1][0])); src[1]++;
    dst[2] = ((float)(src[2][0])); src[2]++;
    dst[3] = ((float)(src[3][0])); src[3]++;
    dst[4] = ((float)(src[4][0])); src[4]++;

    dst += nch;
  }
  restore_rounding(r);
}

void
linear_pcm16_6ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  const int nch = 6;
  samples_t src = samples;
  int16_t *dst = (int16_t *)rawdata;

  int r = set_rounding();  
  while (size--)
  {
    dst[0] = int2le16(s2i16(src[0][0])); src[0]++;
    dst[1] = int2le16(s2i16(src[1][0])); src[1]++;
    dst[2] = int2le16(s2i16(src[2][0])); src[2]++;
    dst[3] = int2le16(s2i16(src[3][0])); src[3]++;
    dst[4] = int2le16(s2i16(src[4][0])); src[4]++;
    dst[5] = int2le16(s2i16(src[5][0])); src[5]++;

    dst += nch;
  }
  restore_rounding(r);
}
void
linear_pcm24_6ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  const int nch = 6;
  samples_t src = samples;
  int24_t *dst = (int24_t *)rawdata;

  int r = set_rounding();  
  while (size--)
  {
    dst[0] = int2le24(s2i24(src[0][0])); src[0]++;
    dst[1] = int2le24(s2i24(src[1][0])); src[1]++;
    dst[2] = int2le24(s2i24(src[2][0])); src[2]++;
    dst[3] = int2le24(s2i24(src[3][0])); src[3]++;
    dst[4] = int2le24(s2i24(src[4][0])); src[4]++;
    dst[5] = int2le24(s2i24(src[5][0])); src[5]++;

    dst += nch;
  }
  restore_rounding(r);
}
void
linear_pcm32_6ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  const int nch = 6;
  samples_t src = samples;
  int32_t *dst = (int32_t *)rawdata;

  int r = set_rounding();  
  while (size--)
  {
    dst[0] = int2le32(s2i32(src[0][0])); src[0]++;
    dst[1] = int2le32(s2i32(src[1][0])); src[1]++;
    dst[2] = int2le32(s2i32(src[2][0])); src[2]++;
    dst[3] = int2le32(s2i32(src[3][0])); src[3]++;
    dst[4] = int2le32(s2i32(src[4][0])); src[4]++;
    dst[5] = int2le32(s2i32(src[5][0])); src[5]++;

    dst += nch;
  }
  restore_rounding(r);
}
void
linear_pcm16_be_6ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  const int nch = 6;
  samples_t src = samples;
  int16_t *dst = (int16_t *)rawdata;

  int r = set_rounding();  
  while (size--)
  {
    dst[0] = int2be16(s2i16(src[0][0])); src[0]++;
    dst[1] = int2be16(s2i16(src[1][0])); src[1]++;
    dst[2] = int2be16(s2i16(src[2][0])); src[2]++;
    dst[3] = int2be16(s2i16(src[3][0])); src[3]++;
    dst[4] = int2be16(s2i16(src[4][0])); src[4]++;
    dst[5] = int2be16(s2i16(src[5][0])); src[5]++;

    dst += nch;
  }
  restore_rounding(r);
}
void
linear_pcm24_be_6ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  const int nch = 6;
  samples_t src = samples;
  int24_t *dst = (int24_t *)rawdata;

  int r = set_rounding();  
  while (size--)
  {
    dst[0] = int2be24(s2i24(src[0][0])); src[0]++;
    dst[1] = int2be24(s2i24(src[1][0])); src[1]++;
    dst[2] = int2be24(s2i24(src[2][0])); src[2]++;
    dst[3] = int2be24(s2i24(src[3][0])); src[3]++;
    dst[4] = int2be24(s2i24(src[4][0])); src[4]++;
    dst[5] = int2be24(s2i24(src[5][0])); src[5]++;

    dst += nch;
  }
  restore_rounding(r);
}
void
linear_pcm32_be_6ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  const int nch = 6;
  samples_t src = samples;
  int32_t *dst = (int32_t *)rawdata;

  int r = set_rounding();  
  while (size--)
  {
    dst[0] = int2be32(s2i32(src[0][0])); src[0]++;
    dst[1] = int2be32(s2i32(src[1][0])); src[1]++;
    dst[2] = int2be32(s2i32(src[2][0])); src[2]++;
    dst[3] = int2be32(s2i32(src[3][0])); src[3]++;
    dst[4] = int2be32(s2i32(src[4][0])); src[4]++;
    dst[5] = int2be32(s2i32(src[5][0])); src[5]++;

    dst += nch;
  }
  restore_rounding(r);
}
void
linear_pcmfloat_6ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  const int nch = 6;
  samples_t src = samples;
  float *dst = (float *)rawdata;

  int r = set_rounding();  
  while (size--)
  {
    dst[0] = ((float)(src[0][0])); src[0]++;
    dst[1] = ((float)(src[1][0])); src[1]++;
    dst[2] = ((float)(src[2][0])); src[2]++;
    dst[3] = ((float)(src[3][0])); src[3]++;
    dst[4] = ((float)(src[4][0])); src[4]++;
    dst[5] = ((float)(src[5][0])); src[5]++;

    dst += nch;
  }
  restore_rounding(r);
}

