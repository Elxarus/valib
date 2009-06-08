#include <math.h>
#include "convert_func.h"

#if defined(_DEBUG)

static inline int set_rounding() { return 0; }
static inline void restore_rounding(int) {}

#define s2i32(s) ((int32_t)floor((s)+0.5))
#define s2i24(s) ((int24_t)(int32_t)floor((s)+0.5))
#define s2i16(s) ((int16_t)floor((s)+0.5))

#elif defined(_M_IX86)

static inline int set_rounding()
{
  // Set FPU rounding mode to the round to the nearest integer mode
  // Returns unchanged FPU control word to restore later

  uint16_t x87_ctrl;
  __asm fnstcw [x87_ctrl];

  uint16_t new_ctrl = x87_ctrl & 0xf3ff;
  __asm fldcw [new_ctrl];

  return x87_ctrl;
}

static inline void restore_rounding(int r)
{
  // Restores old FPU control word

  uint16_t x87_ctrl = r;
  __asm fldcw [x87_ctrl];
}

inline int32_t s2i32(sample_t s)
{
  register int32_t i;
  __asm fld [s]
  __asm fistp [i]
  return i;
}
inline int24_t s2i24(sample_t s)
{
  register int32_t i;
  __asm fld [s]
  __asm fistp [i]
  return i;
}
inline int16_t s2i16(sample_t s)
{
  register int16_t i;
  __asm fld [s]
  __asm fistp [i]
  return i;
}

#else

static inline int set_rounding() { return 0; }
static inline void restore_rounding(int) {}

inline int32_t s2i32(sample_t s) { return (int32_t)floor(s+0.5); }
inline int32_t s2i24(sample_t s) { return (int24_t)(int32_t)floor(s+0.5); }
inline int16_t s2i16(sample_t s) { return (int16_t)floor(s+0.5); }

#endif

///////////////////////////////////////////////////////////////////////////////

void pcm16_linear_1ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  samples_t dst = samples;
  while (size--)
  {
    int16_t *src = (int16_t *)rawdata;
    *dst[0] = le2int16(src[0]); dst[0]++;
    rawdata += 2;
  }
}
void pcm24_linear_1ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  samples_t dst = samples;
  while (size--)
  {
    int24_t *src = (int24_t *)rawdata;
    *dst[0] = le2int24(src[0]); dst[0]++;
    rawdata += 3;
  }
}
void pcm32_linear_1ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  samples_t dst = samples;
  while (size--)
  {
    int32_t *src = (int32_t *)rawdata;
    *dst[0] = le2int32(src[0]); dst[0]++;
    rawdata += 4;
  }
}
void pcm16_be_linear_1ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  samples_t dst = samples;
  while (size--)
  {
    int16_t *src = (int16_t *)rawdata;
    *dst[0] = be2int16(src[0]); dst[0]++;
    rawdata += 2;
  }
}
void pcm24_be_linear_1ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  samples_t dst = samples;
  while (size--)
  {
    int24_t *src = (int24_t *)rawdata;
    *dst[0] = be2int24(src[0]); dst[0]++;
    rawdata += 3;
  }
}
void pcm32_be_linear_1ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  samples_t dst = samples;
  while (size--)
  {
    int32_t *src = (int32_t *)rawdata;
    *dst[0] = be2int32(src[0]); dst[0]++;
    rawdata += 4;
  }
}
void pcmfloat_linear_1ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  samples_t dst = samples;
  while (size--)
  {
    float *src = (float *)rawdata;
    *dst[0] = sample_t(src[0]); dst[0]++;
    rawdata += 4;
  }
}
void pcmdouble_linear_1ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  samples_t dst = samples;
  while (size--)
  {
    double *src = (double *)rawdata;
    *dst[0] = sample_t(src[0]); dst[0]++;
    rawdata += 8;
  }
}
void lpcm20_linear_1ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  samples_t dst = samples;
  while (size--)
  {
    int16_t *src = (int16_t *)rawdata;
    dst[0][0] = (be2int16(src[0+1*0]) << 4) | (rawdata[1*4+0] >> 4); dst[0][1] = (be2int16(src[0+1*1]) << 4) | (rawdata[1*4+0] & 0xf); dst[0]+=2;
    rawdata += 5;
  }
}
void lpcm24_linear_1ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  samples_t dst = samples;
  while (size--)
  {
    int16_t *src = (int16_t *)rawdata;
    dst[0][0] = (be2int16(src[0+1*0]) << 8) | rawdata[1*4+0*2+0]; dst[0][1] = (be2int16(src[0+1*1]) << 8) | rawdata[1*4+0*2+1]; dst[0]+=2;
    rawdata += 6;
  }
}
void pcm16_linear_2ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  samples_t dst = samples;
  while (size--)
  {
    int16_t *src = (int16_t *)rawdata;
    *dst[0] = le2int16(src[0]); dst[0]++;
    *dst[1] = le2int16(src[1]); dst[1]++;
    rawdata += 4;
  }
}
void pcm24_linear_2ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  samples_t dst = samples;
  while (size--)
  {
    int24_t *src = (int24_t *)rawdata;
    *dst[0] = le2int24(src[0]); dst[0]++;
    *dst[1] = le2int24(src[1]); dst[1]++;
    rawdata += 6;
  }
}
void pcm32_linear_2ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  samples_t dst = samples;
  while (size--)
  {
    int32_t *src = (int32_t *)rawdata;
    *dst[0] = le2int32(src[0]); dst[0]++;
    *dst[1] = le2int32(src[1]); dst[1]++;
    rawdata += 8;
  }
}
void pcm16_be_linear_2ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  samples_t dst = samples;
  while (size--)
  {
    int16_t *src = (int16_t *)rawdata;
    *dst[0] = be2int16(src[0]); dst[0]++;
    *dst[1] = be2int16(src[1]); dst[1]++;
    rawdata += 4;
  }
}
void pcm24_be_linear_2ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  samples_t dst = samples;
  while (size--)
  {
    int24_t *src = (int24_t *)rawdata;
    *dst[0] = be2int24(src[0]); dst[0]++;
    *dst[1] = be2int24(src[1]); dst[1]++;
    rawdata += 6;
  }
}
void pcm32_be_linear_2ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  samples_t dst = samples;
  while (size--)
  {
    int32_t *src = (int32_t *)rawdata;
    *dst[0] = be2int32(src[0]); dst[0]++;
    *dst[1] = be2int32(src[1]); dst[1]++;
    rawdata += 8;
  }
}
void pcmfloat_linear_2ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  samples_t dst = samples;
  while (size--)
  {
    float *src = (float *)rawdata;
    *dst[0] = sample_t(src[0]); dst[0]++;
    *dst[1] = sample_t(src[1]); dst[1]++;
    rawdata += 8;
  }
}
void pcmdouble_linear_2ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  samples_t dst = samples;
  while (size--)
  {
    double *src = (double *)rawdata;
    *dst[0] = sample_t(src[0]); dst[0]++;
    *dst[1] = sample_t(src[1]); dst[1]++;
    rawdata += 16;
  }
}
void lpcm20_linear_2ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  samples_t dst = samples;
  while (size--)
  {
    int16_t *src = (int16_t *)rawdata;
    dst[0][0] = (be2int16(src[0+2*0]) << 4) | (rawdata[2*4+0] >> 4); dst[0][1] = (be2int16(src[0+2*1]) << 4) | (rawdata[2*4+0] & 0xf); dst[0]+=2;
    dst[1][0] = (be2int16(src[1+2*0]) << 4) | (rawdata[2*4+1] >> 4); dst[1][1] = (be2int16(src[1+2*1]) << 4) | (rawdata[2*4+1] & 0xf); dst[1]+=2;
    rawdata += 10;
  }
}
void lpcm24_linear_2ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  samples_t dst = samples;
  while (size--)
  {
    int16_t *src = (int16_t *)rawdata;
    dst[0][0] = (be2int16(src[0+2*0]) << 8) | rawdata[2*4+0*2+0]; dst[0][1] = (be2int16(src[0+2*1]) << 8) | rawdata[2*4+0*2+1]; dst[0]+=2;
    dst[1][0] = (be2int16(src[1+2*0]) << 8) | rawdata[2*4+1*2+0]; dst[1][1] = (be2int16(src[1+2*1]) << 8) | rawdata[2*4+1*2+1]; dst[1]+=2;
    rawdata += 12;
  }
}
void pcm16_linear_3ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  samples_t dst = samples;
  while (size--)
  {
    int16_t *src = (int16_t *)rawdata;
    *dst[0] = le2int16(src[0]); dst[0]++;
    *dst[1] = le2int16(src[1]); dst[1]++;
    *dst[2] = le2int16(src[2]); dst[2]++;
    rawdata += 6;
  }
}
void pcm24_linear_3ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  samples_t dst = samples;
  while (size--)
  {
    int24_t *src = (int24_t *)rawdata;
    *dst[0] = le2int24(src[0]); dst[0]++;
    *dst[1] = le2int24(src[1]); dst[1]++;
    *dst[2] = le2int24(src[2]); dst[2]++;
    rawdata += 9;
  }
}
void pcm32_linear_3ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  samples_t dst = samples;
  while (size--)
  {
    int32_t *src = (int32_t *)rawdata;
    *dst[0] = le2int32(src[0]); dst[0]++;
    *dst[1] = le2int32(src[1]); dst[1]++;
    *dst[2] = le2int32(src[2]); dst[2]++;
    rawdata += 12;
  }
}
void pcm16_be_linear_3ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  samples_t dst = samples;
  while (size--)
  {
    int16_t *src = (int16_t *)rawdata;
    *dst[0] = be2int16(src[0]); dst[0]++;
    *dst[1] = be2int16(src[1]); dst[1]++;
    *dst[2] = be2int16(src[2]); dst[2]++;
    rawdata += 6;
  }
}
void pcm24_be_linear_3ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  samples_t dst = samples;
  while (size--)
  {
    int24_t *src = (int24_t *)rawdata;
    *dst[0] = be2int24(src[0]); dst[0]++;
    *dst[1] = be2int24(src[1]); dst[1]++;
    *dst[2] = be2int24(src[2]); dst[2]++;
    rawdata += 9;
  }
}
void pcm32_be_linear_3ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  samples_t dst = samples;
  while (size--)
  {
    int32_t *src = (int32_t *)rawdata;
    *dst[0] = be2int32(src[0]); dst[0]++;
    *dst[1] = be2int32(src[1]); dst[1]++;
    *dst[2] = be2int32(src[2]); dst[2]++;
    rawdata += 12;
  }
}
void pcmfloat_linear_3ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  samples_t dst = samples;
  while (size--)
  {
    float *src = (float *)rawdata;
    *dst[0] = sample_t(src[0]); dst[0]++;
    *dst[1] = sample_t(src[1]); dst[1]++;
    *dst[2] = sample_t(src[2]); dst[2]++;
    rawdata += 12;
  }
}
void pcmdouble_linear_3ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  samples_t dst = samples;
  while (size--)
  {
    double *src = (double *)rawdata;
    *dst[0] = sample_t(src[0]); dst[0]++;
    *dst[1] = sample_t(src[1]); dst[1]++;
    *dst[2] = sample_t(src[2]); dst[2]++;
    rawdata += 24;
  }
}
void lpcm20_linear_3ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  samples_t dst = samples;
  while (size--)
  {
    int16_t *src = (int16_t *)rawdata;
    dst[0][0] = (be2int16(src[0+3*0]) << 4) | (rawdata[3*4+0] >> 4); dst[0][1] = (be2int16(src[0+3*1]) << 4) | (rawdata[3*4+0] & 0xf); dst[0]+=2;
    dst[1][0] = (be2int16(src[1+3*0]) << 4) | (rawdata[3*4+1] >> 4); dst[1][1] = (be2int16(src[1+3*1]) << 4) | (rawdata[3*4+1] & 0xf); dst[1]+=2;
    dst[2][0] = (be2int16(src[2+3*0]) << 4) | (rawdata[3*4+2] >> 4); dst[2][1] = (be2int16(src[2+3*1]) << 4) | (rawdata[3*4+2] & 0xf); dst[2]+=2;
    rawdata += 15;
  }
}
void lpcm24_linear_3ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  samples_t dst = samples;
  while (size--)
  {
    int16_t *src = (int16_t *)rawdata;
    dst[0][0] = (be2int16(src[0+3*0]) << 8) | rawdata[3*4+0*2+0]; dst[0][1] = (be2int16(src[0+3*1]) << 8) | rawdata[3*4+0*2+1]; dst[0]+=2;
    dst[1][0] = (be2int16(src[1+3*0]) << 8) | rawdata[3*4+1*2+0]; dst[1][1] = (be2int16(src[1+3*1]) << 8) | rawdata[3*4+1*2+1]; dst[1]+=2;
    dst[2][0] = (be2int16(src[2+3*0]) << 8) | rawdata[3*4+2*2+0]; dst[2][1] = (be2int16(src[2+3*1]) << 8) | rawdata[3*4+2*2+1]; dst[2]+=2;
    rawdata += 18;
  }
}
void pcm16_linear_4ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  samples_t dst = samples;
  while (size--)
  {
    int16_t *src = (int16_t *)rawdata;
    *dst[0] = le2int16(src[0]); dst[0]++;
    *dst[1] = le2int16(src[1]); dst[1]++;
    *dst[2] = le2int16(src[2]); dst[2]++;
    *dst[3] = le2int16(src[3]); dst[3]++;
    rawdata += 8;
  }
}
void pcm24_linear_4ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  samples_t dst = samples;
  while (size--)
  {
    int24_t *src = (int24_t *)rawdata;
    *dst[0] = le2int24(src[0]); dst[0]++;
    *dst[1] = le2int24(src[1]); dst[1]++;
    *dst[2] = le2int24(src[2]); dst[2]++;
    *dst[3] = le2int24(src[3]); dst[3]++;
    rawdata += 12;
  }
}
void pcm32_linear_4ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  samples_t dst = samples;
  while (size--)
  {
    int32_t *src = (int32_t *)rawdata;
    *dst[0] = le2int32(src[0]); dst[0]++;
    *dst[1] = le2int32(src[1]); dst[1]++;
    *dst[2] = le2int32(src[2]); dst[2]++;
    *dst[3] = le2int32(src[3]); dst[3]++;
    rawdata += 16;
  }
}
void pcm16_be_linear_4ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  samples_t dst = samples;
  while (size--)
  {
    int16_t *src = (int16_t *)rawdata;
    *dst[0] = be2int16(src[0]); dst[0]++;
    *dst[1] = be2int16(src[1]); dst[1]++;
    *dst[2] = be2int16(src[2]); dst[2]++;
    *dst[3] = be2int16(src[3]); dst[3]++;
    rawdata += 8;
  }
}
void pcm24_be_linear_4ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  samples_t dst = samples;
  while (size--)
  {
    int24_t *src = (int24_t *)rawdata;
    *dst[0] = be2int24(src[0]); dst[0]++;
    *dst[1] = be2int24(src[1]); dst[1]++;
    *dst[2] = be2int24(src[2]); dst[2]++;
    *dst[3] = be2int24(src[3]); dst[3]++;
    rawdata += 12;
  }
}
void pcm32_be_linear_4ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  samples_t dst = samples;
  while (size--)
  {
    int32_t *src = (int32_t *)rawdata;
    *dst[0] = be2int32(src[0]); dst[0]++;
    *dst[1] = be2int32(src[1]); dst[1]++;
    *dst[2] = be2int32(src[2]); dst[2]++;
    *dst[3] = be2int32(src[3]); dst[3]++;
    rawdata += 16;
  }
}
void pcmfloat_linear_4ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  samples_t dst = samples;
  while (size--)
  {
    float *src = (float *)rawdata;
    *dst[0] = sample_t(src[0]); dst[0]++;
    *dst[1] = sample_t(src[1]); dst[1]++;
    *dst[2] = sample_t(src[2]); dst[2]++;
    *dst[3] = sample_t(src[3]); dst[3]++;
    rawdata += 16;
  }
}
void pcmdouble_linear_4ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  samples_t dst = samples;
  while (size--)
  {
    double *src = (double *)rawdata;
    *dst[0] = sample_t(src[0]); dst[0]++;
    *dst[1] = sample_t(src[1]); dst[1]++;
    *dst[2] = sample_t(src[2]); dst[2]++;
    *dst[3] = sample_t(src[3]); dst[3]++;
    rawdata += 32;
  }
}
void lpcm20_linear_4ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  samples_t dst = samples;
  while (size--)
  {
    int16_t *src = (int16_t *)rawdata;
    dst[0][0] = (be2int16(src[0+4*0]) << 4) | (rawdata[4*4+0] >> 4); dst[0][1] = (be2int16(src[0+4*1]) << 4) | (rawdata[4*4+0] & 0xf); dst[0]+=2;
    dst[1][0] = (be2int16(src[1+4*0]) << 4) | (rawdata[4*4+1] >> 4); dst[1][1] = (be2int16(src[1+4*1]) << 4) | (rawdata[4*4+1] & 0xf); dst[1]+=2;
    dst[2][0] = (be2int16(src[2+4*0]) << 4) | (rawdata[4*4+2] >> 4); dst[2][1] = (be2int16(src[2+4*1]) << 4) | (rawdata[4*4+2] & 0xf); dst[2]+=2;
    dst[3][0] = (be2int16(src[3+4*0]) << 4) | (rawdata[4*4+3] >> 4); dst[3][1] = (be2int16(src[3+4*1]) << 4) | (rawdata[4*4+3] & 0xf); dst[3]+=2;
    rawdata += 20;
  }
}
void lpcm24_linear_4ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  samples_t dst = samples;
  while (size--)
  {
    int16_t *src = (int16_t *)rawdata;
    dst[0][0] = (be2int16(src[0+4*0]) << 8) | rawdata[4*4+0*2+0]; dst[0][1] = (be2int16(src[0+4*1]) << 8) | rawdata[4*4+0*2+1]; dst[0]+=2;
    dst[1][0] = (be2int16(src[1+4*0]) << 8) | rawdata[4*4+1*2+0]; dst[1][1] = (be2int16(src[1+4*1]) << 8) | rawdata[4*4+1*2+1]; dst[1]+=2;
    dst[2][0] = (be2int16(src[2+4*0]) << 8) | rawdata[4*4+2*2+0]; dst[2][1] = (be2int16(src[2+4*1]) << 8) | rawdata[4*4+2*2+1]; dst[2]+=2;
    dst[3][0] = (be2int16(src[3+4*0]) << 8) | rawdata[4*4+3*2+0]; dst[3][1] = (be2int16(src[3+4*1]) << 8) | rawdata[4*4+3*2+1]; dst[3]+=2;
    rawdata += 24;
  }
}
void pcm16_linear_5ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  samples_t dst = samples;
  while (size--)
  {
    int16_t *src = (int16_t *)rawdata;
    *dst[0] = le2int16(src[0]); dst[0]++;
    *dst[1] = le2int16(src[1]); dst[1]++;
    *dst[2] = le2int16(src[2]); dst[2]++;
    *dst[3] = le2int16(src[3]); dst[3]++;
    *dst[4] = le2int16(src[4]); dst[4]++;
    rawdata += 10;
  }
}
void pcm24_linear_5ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  samples_t dst = samples;
  while (size--)
  {
    int24_t *src = (int24_t *)rawdata;
    *dst[0] = le2int24(src[0]); dst[0]++;
    *dst[1] = le2int24(src[1]); dst[1]++;
    *dst[2] = le2int24(src[2]); dst[2]++;
    *dst[3] = le2int24(src[3]); dst[3]++;
    *dst[4] = le2int24(src[4]); dst[4]++;
    rawdata += 15;
  }
}
void pcm32_linear_5ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  samples_t dst = samples;
  while (size--)
  {
    int32_t *src = (int32_t *)rawdata;
    *dst[0] = le2int32(src[0]); dst[0]++;
    *dst[1] = le2int32(src[1]); dst[1]++;
    *dst[2] = le2int32(src[2]); dst[2]++;
    *dst[3] = le2int32(src[3]); dst[3]++;
    *dst[4] = le2int32(src[4]); dst[4]++;
    rawdata += 20;
  }
}
void pcm16_be_linear_5ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  samples_t dst = samples;
  while (size--)
  {
    int16_t *src = (int16_t *)rawdata;
    *dst[0] = be2int16(src[0]); dst[0]++;
    *dst[1] = be2int16(src[1]); dst[1]++;
    *dst[2] = be2int16(src[2]); dst[2]++;
    *dst[3] = be2int16(src[3]); dst[3]++;
    *dst[4] = be2int16(src[4]); dst[4]++;
    rawdata += 10;
  }
}
void pcm24_be_linear_5ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  samples_t dst = samples;
  while (size--)
  {
    int24_t *src = (int24_t *)rawdata;
    *dst[0] = be2int24(src[0]); dst[0]++;
    *dst[1] = be2int24(src[1]); dst[1]++;
    *dst[2] = be2int24(src[2]); dst[2]++;
    *dst[3] = be2int24(src[3]); dst[3]++;
    *dst[4] = be2int24(src[4]); dst[4]++;
    rawdata += 15;
  }
}
void pcm32_be_linear_5ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  samples_t dst = samples;
  while (size--)
  {
    int32_t *src = (int32_t *)rawdata;
    *dst[0] = be2int32(src[0]); dst[0]++;
    *dst[1] = be2int32(src[1]); dst[1]++;
    *dst[2] = be2int32(src[2]); dst[2]++;
    *dst[3] = be2int32(src[3]); dst[3]++;
    *dst[4] = be2int32(src[4]); dst[4]++;
    rawdata += 20;
  }
}
void pcmfloat_linear_5ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  samples_t dst = samples;
  while (size--)
  {
    float *src = (float *)rawdata;
    *dst[0] = sample_t(src[0]); dst[0]++;
    *dst[1] = sample_t(src[1]); dst[1]++;
    *dst[2] = sample_t(src[2]); dst[2]++;
    *dst[3] = sample_t(src[3]); dst[3]++;
    *dst[4] = sample_t(src[4]); dst[4]++;
    rawdata += 20;
  }
}
void pcmdouble_linear_5ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  samples_t dst = samples;
  while (size--)
  {
    double *src = (double *)rawdata;
    *dst[0] = sample_t(src[0]); dst[0]++;
    *dst[1] = sample_t(src[1]); dst[1]++;
    *dst[2] = sample_t(src[2]); dst[2]++;
    *dst[3] = sample_t(src[3]); dst[3]++;
    *dst[4] = sample_t(src[4]); dst[4]++;
    rawdata += 40;
  }
}
void lpcm20_linear_5ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  samples_t dst = samples;
  while (size--)
  {
    int16_t *src = (int16_t *)rawdata;
    dst[0][0] = (be2int16(src[0+5*0]) << 4) | (rawdata[5*4+0] >> 4); dst[0][1] = (be2int16(src[0+5*1]) << 4) | (rawdata[5*4+0] & 0xf); dst[0]+=2;
    dst[1][0] = (be2int16(src[1+5*0]) << 4) | (rawdata[5*4+1] >> 4); dst[1][1] = (be2int16(src[1+5*1]) << 4) | (rawdata[5*4+1] & 0xf); dst[1]+=2;
    dst[2][0] = (be2int16(src[2+5*0]) << 4) | (rawdata[5*4+2] >> 4); dst[2][1] = (be2int16(src[2+5*1]) << 4) | (rawdata[5*4+2] & 0xf); dst[2]+=2;
    dst[3][0] = (be2int16(src[3+5*0]) << 4) | (rawdata[5*4+3] >> 4); dst[3][1] = (be2int16(src[3+5*1]) << 4) | (rawdata[5*4+3] & 0xf); dst[3]+=2;
    dst[4][0] = (be2int16(src[4+5*0]) << 4) | (rawdata[5*4+4] >> 4); dst[4][1] = (be2int16(src[4+5*1]) << 4) | (rawdata[5*4+4] & 0xf); dst[4]+=2;
    rawdata += 25;
  }
}
void lpcm24_linear_5ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  samples_t dst = samples;
  while (size--)
  {
    int16_t *src = (int16_t *)rawdata;
    dst[0][0] = (be2int16(src[0+5*0]) << 8) | rawdata[5*4+0*2+0]; dst[0][1] = (be2int16(src[0+5*1]) << 8) | rawdata[5*4+0*2+1]; dst[0]+=2;
    dst[1][0] = (be2int16(src[1+5*0]) << 8) | rawdata[5*4+1*2+0]; dst[1][1] = (be2int16(src[1+5*1]) << 8) | rawdata[5*4+1*2+1]; dst[1]+=2;
    dst[2][0] = (be2int16(src[2+5*0]) << 8) | rawdata[5*4+2*2+0]; dst[2][1] = (be2int16(src[2+5*1]) << 8) | rawdata[5*4+2*2+1]; dst[2]+=2;
    dst[3][0] = (be2int16(src[3+5*0]) << 8) | rawdata[5*4+3*2+0]; dst[3][1] = (be2int16(src[3+5*1]) << 8) | rawdata[5*4+3*2+1]; dst[3]+=2;
    dst[4][0] = (be2int16(src[4+5*0]) << 8) | rawdata[5*4+4*2+0]; dst[4][1] = (be2int16(src[4+5*1]) << 8) | rawdata[5*4+4*2+1]; dst[4]+=2;
    rawdata += 30;
  }
}
void pcm16_linear_6ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  samples_t dst = samples;
  while (size--)
  {
    int16_t *src = (int16_t *)rawdata;
    *dst[0] = le2int16(src[0]); dst[0]++;
    *dst[1] = le2int16(src[1]); dst[1]++;
    *dst[2] = le2int16(src[2]); dst[2]++;
    *dst[3] = le2int16(src[3]); dst[3]++;
    *dst[4] = le2int16(src[4]); dst[4]++;
    *dst[5] = le2int16(src[5]); dst[5]++;
    rawdata += 12;
  }
}
void pcm24_linear_6ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  samples_t dst = samples;
  while (size--)
  {
    int24_t *src = (int24_t *)rawdata;
    *dst[0] = le2int24(src[0]); dst[0]++;
    *dst[1] = le2int24(src[1]); dst[1]++;
    *dst[2] = le2int24(src[2]); dst[2]++;
    *dst[3] = le2int24(src[3]); dst[3]++;
    *dst[4] = le2int24(src[4]); dst[4]++;
    *dst[5] = le2int24(src[5]); dst[5]++;
    rawdata += 18;
  }
}
void pcm32_linear_6ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  samples_t dst = samples;
  while (size--)
  {
    int32_t *src = (int32_t *)rawdata;
    *dst[0] = le2int32(src[0]); dst[0]++;
    *dst[1] = le2int32(src[1]); dst[1]++;
    *dst[2] = le2int32(src[2]); dst[2]++;
    *dst[3] = le2int32(src[3]); dst[3]++;
    *dst[4] = le2int32(src[4]); dst[4]++;
    *dst[5] = le2int32(src[5]); dst[5]++;
    rawdata += 24;
  }
}
void pcm16_be_linear_6ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  samples_t dst = samples;
  while (size--)
  {
    int16_t *src = (int16_t *)rawdata;
    *dst[0] = be2int16(src[0]); dst[0]++;
    *dst[1] = be2int16(src[1]); dst[1]++;
    *dst[2] = be2int16(src[2]); dst[2]++;
    *dst[3] = be2int16(src[3]); dst[3]++;
    *dst[4] = be2int16(src[4]); dst[4]++;
    *dst[5] = be2int16(src[5]); dst[5]++;
    rawdata += 12;
  }
}
void pcm24_be_linear_6ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  samples_t dst = samples;
  while (size--)
  {
    int24_t *src = (int24_t *)rawdata;
    *dst[0] = be2int24(src[0]); dst[0]++;
    *dst[1] = be2int24(src[1]); dst[1]++;
    *dst[2] = be2int24(src[2]); dst[2]++;
    *dst[3] = be2int24(src[3]); dst[3]++;
    *dst[4] = be2int24(src[4]); dst[4]++;
    *dst[5] = be2int24(src[5]); dst[5]++;
    rawdata += 18;
  }
}
void pcm32_be_linear_6ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  samples_t dst = samples;
  while (size--)
  {
    int32_t *src = (int32_t *)rawdata;
    *dst[0] = be2int32(src[0]); dst[0]++;
    *dst[1] = be2int32(src[1]); dst[1]++;
    *dst[2] = be2int32(src[2]); dst[2]++;
    *dst[3] = be2int32(src[3]); dst[3]++;
    *dst[4] = be2int32(src[4]); dst[4]++;
    *dst[5] = be2int32(src[5]); dst[5]++;
    rawdata += 24;
  }
}
void pcmfloat_linear_6ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  samples_t dst = samples;
  while (size--)
  {
    float *src = (float *)rawdata;
    *dst[0] = sample_t(src[0]); dst[0]++;
    *dst[1] = sample_t(src[1]); dst[1]++;
    *dst[2] = sample_t(src[2]); dst[2]++;
    *dst[3] = sample_t(src[3]); dst[3]++;
    *dst[4] = sample_t(src[4]); dst[4]++;
    *dst[5] = sample_t(src[5]); dst[5]++;
    rawdata += 24;
  }
}
void pcmdouble_linear_6ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  samples_t dst = samples;
  while (size--)
  {
    double *src = (double *)rawdata;
    *dst[0] = sample_t(src[0]); dst[0]++;
    *dst[1] = sample_t(src[1]); dst[1]++;
    *dst[2] = sample_t(src[2]); dst[2]++;
    *dst[3] = sample_t(src[3]); dst[3]++;
    *dst[4] = sample_t(src[4]); dst[4]++;
    *dst[5] = sample_t(src[5]); dst[5]++;
    rawdata += 48;
  }
}
void lpcm20_linear_6ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  samples_t dst = samples;
  while (size--)
  {
    int16_t *src = (int16_t *)rawdata;
    dst[0][0] = (be2int16(src[0+6*0]) << 4) | (rawdata[6*4+0] >> 4); dst[0][1] = (be2int16(src[0+6*1]) << 4) | (rawdata[6*4+0] & 0xf); dst[0]+=2;
    dst[1][0] = (be2int16(src[1+6*0]) << 4) | (rawdata[6*4+1] >> 4); dst[1][1] = (be2int16(src[1+6*1]) << 4) | (rawdata[6*4+1] & 0xf); dst[1]+=2;
    dst[2][0] = (be2int16(src[2+6*0]) << 4) | (rawdata[6*4+2] >> 4); dst[2][1] = (be2int16(src[2+6*1]) << 4) | (rawdata[6*4+2] & 0xf); dst[2]+=2;
    dst[3][0] = (be2int16(src[3+6*0]) << 4) | (rawdata[6*4+3] >> 4); dst[3][1] = (be2int16(src[3+6*1]) << 4) | (rawdata[6*4+3] & 0xf); dst[3]+=2;
    dst[4][0] = (be2int16(src[4+6*0]) << 4) | (rawdata[6*4+4] >> 4); dst[4][1] = (be2int16(src[4+6*1]) << 4) | (rawdata[6*4+4] & 0xf); dst[4]+=2;
    dst[5][0] = (be2int16(src[5+6*0]) << 4) | (rawdata[6*4+5] >> 4); dst[5][1] = (be2int16(src[5+6*1]) << 4) | (rawdata[6*4+5] & 0xf); dst[5]+=2;
    rawdata += 30;
  }
}
void lpcm24_linear_6ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  samples_t dst = samples;
  while (size--)
  {
    int16_t *src = (int16_t *)rawdata;
    dst[0][0] = (be2int16(src[0+6*0]) << 8) | rawdata[6*4+0*2+0]; dst[0][1] = (be2int16(src[0+6*1]) << 8) | rawdata[6*4+0*2+1]; dst[0]+=2;
    dst[1][0] = (be2int16(src[1+6*0]) << 8) | rawdata[6*4+1*2+0]; dst[1][1] = (be2int16(src[1+6*1]) << 8) | rawdata[6*4+1*2+1]; dst[1]+=2;
    dst[2][0] = (be2int16(src[2+6*0]) << 8) | rawdata[6*4+2*2+0]; dst[2][1] = (be2int16(src[2+6*1]) << 8) | rawdata[6*4+2*2+1]; dst[2]+=2;
    dst[3][0] = (be2int16(src[3+6*0]) << 8) | rawdata[6*4+3*2+0]; dst[3][1] = (be2int16(src[3+6*1]) << 8) | rawdata[6*4+3*2+1]; dst[3]+=2;
    dst[4][0] = (be2int16(src[4+6*0]) << 8) | rawdata[6*4+4*2+0]; dst[4][1] = (be2int16(src[4+6*1]) << 8) | rawdata[6*4+4*2+1]; dst[4]+=2;
    dst[5][0] = (be2int16(src[5+6*0]) << 8) | rawdata[6*4+5*2+0]; dst[5][1] = (be2int16(src[5+6*1]) << 8) | rawdata[6*4+5*2+1]; dst[5]+=2;
    rawdata += 36;
  }
}

///////////////////////////////////////////////////////////////////////////////

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
linear_pcmdouble_1ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  const int nch = 1;
  samples_t src = samples;
  double *dst = (double *)rawdata;

  int r = set_rounding();  
  while (size--)
  {
    dst[0] = ((double)(src[0][0])); src[0]++;

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
linear_pcmdouble_2ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  const int nch = 2;
  samples_t src = samples;
  double *dst = (double *)rawdata;

  int r = set_rounding();  
  while (size--)
  {
    dst[0] = ((double)(src[0][0])); src[0]++;
    dst[1] = ((double)(src[1][0])); src[1]++;

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
linear_pcmdouble_3ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  const int nch = 3;
  samples_t src = samples;
  double *dst = (double *)rawdata;

  int r = set_rounding();  
  while (size--)
  {
    dst[0] = ((double)(src[0][0])); src[0]++;
    dst[1] = ((double)(src[1][0])); src[1]++;
    dst[2] = ((double)(src[2][0])); src[2]++;

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
linear_pcmdouble_4ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  const int nch = 4;
  samples_t src = samples;
  double *dst = (double *)rawdata;

  int r = set_rounding();  
  while (size--)
  {
    dst[0] = ((double)(src[0][0])); src[0]++;
    dst[1] = ((double)(src[1][0])); src[1]++;
    dst[2] = ((double)(src[2][0])); src[2]++;
    dst[3] = ((double)(src[3][0])); src[3]++;

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
linear_pcmdouble_5ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  const int nch = 5;
  samples_t src = samples;
  double *dst = (double *)rawdata;

  int r = set_rounding();  
  while (size--)
  {
    dst[0] = ((double)(src[0][0])); src[0]++;
    dst[1] = ((double)(src[1][0])); src[1]++;
    dst[2] = ((double)(src[2][0])); src[2]++;
    dst[3] = ((double)(src[3][0])); src[3]++;
    dst[4] = ((double)(src[4][0])); src[4]++;

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
void
linear_pcmdouble_6ch(uint8_t *rawdata, samples_t samples, size_t size)
{
  const int nch = 6;
  samples_t src = samples;
  double *dst = (double *)rawdata;

  int r = set_rounding();  
  while (size--)
  {
    dst[0] = ((double)(src[0][0])); src[0]++;
    dst[1] = ((double)(src[1][0])); src[1]++;
    dst[2] = ((double)(src[2][0])); src[2]++;
    dst[3] = ((double)(src[3][0])); src[3]++;
    dst[4] = ((double)(src[4][0])); src[4]++;
    dst[5] = ((double)(src[5][0])); src[5]++;

    dst += nch;
  }
  restore_rounding(r);
}

///////////////////////////////////////////////////////////////////////////////

static const int pcm2linear_formats[] = { FORMAT_PCM16, FORMAT_PCM24, FORMAT_PCM32, FORMAT_PCM16_BE, FORMAT_PCM24_BE, FORMAT_PCM32_BE, FORMAT_PCMFLOAT, FORMAT_PCMDOUBLE, FORMAT_LPCM20, FORMAT_LPCM24 };
static const convert_t pcm2linear_tbl[NCHANNELS][10] = {
 { pcm16_linear_1ch, pcm24_linear_1ch, pcm32_linear_1ch, pcm16_be_linear_1ch, pcm24_be_linear_1ch, pcm32_be_linear_1ch, pcmfloat_linear_1ch, pcmdouble_linear_1ch, lpcm20_linear_1ch, lpcm24_linear_1ch },
 { pcm16_linear_2ch, pcm24_linear_2ch, pcm32_linear_2ch, pcm16_be_linear_2ch, pcm24_be_linear_2ch, pcm32_be_linear_2ch, pcmfloat_linear_2ch, pcmdouble_linear_2ch, lpcm20_linear_2ch, lpcm24_linear_2ch },
 { pcm16_linear_3ch, pcm24_linear_3ch, pcm32_linear_3ch, pcm16_be_linear_3ch, pcm24_be_linear_3ch, pcm32_be_linear_3ch, pcmfloat_linear_3ch, pcmdouble_linear_3ch, lpcm20_linear_3ch, lpcm24_linear_3ch },
 { pcm16_linear_4ch, pcm24_linear_4ch, pcm32_linear_4ch, pcm16_be_linear_4ch, pcm24_be_linear_4ch, pcm32_be_linear_4ch, pcmfloat_linear_4ch, pcmdouble_linear_4ch, lpcm20_linear_4ch, lpcm24_linear_4ch },
 { pcm16_linear_5ch, pcm24_linear_5ch, pcm32_linear_5ch, pcm16_be_linear_5ch, pcm24_be_linear_5ch, pcm32_be_linear_5ch, pcmfloat_linear_5ch, pcmdouble_linear_5ch, lpcm20_linear_5ch, lpcm24_linear_5ch },
 { pcm16_linear_6ch, pcm24_linear_6ch, pcm32_linear_6ch, pcm16_be_linear_6ch, pcm24_be_linear_6ch, pcm32_be_linear_6ch, pcmfloat_linear_6ch, pcmdouble_linear_6ch, lpcm20_linear_6ch, lpcm24_linear_6ch },
};

static const int linear2pcm_formats[] = { FORMAT_PCM16, FORMAT_PCM24, FORMAT_PCM32, FORMAT_PCM16_BE, FORMAT_PCM24_BE, FORMAT_PCM32_BE, FORMAT_PCMFLOAT, FORMAT_PCMDOUBLE };
static const convert_t linear2pcm_tbl[NCHANNELS][8] = {
 { linear_pcm16_1ch, linear_pcm24_1ch, linear_pcm32_1ch, linear_pcm16_be_1ch, linear_pcm24_be_1ch, linear_pcm32_be_1ch, linear_pcmfloat_1ch, linear_pcmdouble_1ch },
 { linear_pcm16_2ch, linear_pcm24_2ch, linear_pcm32_2ch, linear_pcm16_be_2ch, linear_pcm24_be_2ch, linear_pcm32_be_2ch, linear_pcmfloat_2ch, linear_pcmdouble_2ch },
 { linear_pcm16_3ch, linear_pcm24_3ch, linear_pcm32_3ch, linear_pcm16_be_3ch, linear_pcm24_be_3ch, linear_pcm32_be_3ch, linear_pcmfloat_3ch, linear_pcmdouble_3ch },
 { linear_pcm16_4ch, linear_pcm24_4ch, linear_pcm32_4ch, linear_pcm16_be_4ch, linear_pcm24_be_4ch, linear_pcm32_be_4ch, linear_pcmfloat_4ch, linear_pcmdouble_4ch },
 { linear_pcm16_5ch, linear_pcm24_5ch, linear_pcm32_5ch, linear_pcm16_be_5ch, linear_pcm24_be_5ch, linear_pcm32_be_5ch, linear_pcmfloat_5ch, linear_pcmdouble_5ch },
 { linear_pcm16_6ch, linear_pcm24_6ch, linear_pcm32_6ch, linear_pcm16_be_6ch, linear_pcm24_be_6ch, linear_pcm32_be_6ch, linear_pcmfloat_6ch, linear_pcmdouble_6ch },
};

convert_t find_pcm2linear(int pcm_format, int nch)
{
  if (nch < 1 || nch > NCHANNELS)
    return 0;

  for (int i = 0; i < array_size(pcm2linear_formats); i++)
    if (pcm_format == pcm2linear_formats[i])
      return pcm2linear_tbl[nch-1][i];

  return 0;
}

convert_t find_linear2pcm(int pcm_format, int nch)
{
  if (nch < 1 || nch > NCHANNELS)
    return 0;

  for (int i = 0; i < array_size(linear2pcm_formats); i++)
    if (pcm_format == linear2pcm_formats[i])
      return linear2pcm_tbl[nch-1][i];

  return 0;
}
