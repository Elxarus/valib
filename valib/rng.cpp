#include "rng.h"

#define a 16807         /* multiplier */
#define m 2147483647L   /* 2**31 - 1 */
#define q 127773L       /* m div a */
#define r 2836          /* m mod a */


RNG::RNG(int32_t _seed)
{
  set(_seed); 
};

void 
RNG::set(int32_t _seed)
{
  seed = _seed;
}

int32_t 
RNG::next()
{
  uint32_t lo, hi;

  lo = a * (int32_t)(seed & 0xFFFF);
  hi = a * (int32_t)((uint32_t)seed >> 16);
  lo += (hi & 0x7FFF) << 16;
  if (lo > m)
  {
    lo &= m;
    ++lo;
  }
  lo += hi >> 15;
  if (lo > m)
  {
    lo &= m;
    ++lo;
  }
  return (int32_t)lo;
}

int32_t
RNG::next_int(int32_t range) const
{
  int32_t  hi1 = seed;
  uint32_t lo1 = uint32_t(hi1) & 0xffff;
  hi1 >>= 16;

  int32_t  hi2 = range;
  uint32_t lo2 = uint32_t(hi2) & 0xffff;
  hi2 >>= 16;

  return hi1*hi2 + ((hi1*lo2) >> 16) + ((hi2*lo1) >> 16);
}

uint32_t
RNG::next_uint(uint32_t range) const
{
  uint32_t hi1 = seed;
  uint32_t lo1 = hi1 & 0xffff;
  hi1 >>= 16;

  uint32_t hi2 = range;
  uint32_t lo2 =hi2 & 0xffff;
  hi2 >>= 16;

  return hi1*hi2 + ((hi1*lo2) >> 16) + ((hi2*lo1) >> 16);
}

float
RNG::next_float(float range) const
{
  return float(seed) * range / m;
}
