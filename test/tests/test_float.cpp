/*
  Test sample_mant() and sample_exp() functions
*/

#include <stdio.h>
#include "defs.h"

///////////////////////////////////////////////////////////////////////////////
// sample_t utils
///////////////////////////////////////////////////////////////////////////////

#ifndef FLOAT_SAMPLE

union double_t 
{ 
  double d;
  uint8_t raw[8];
  struct 
  {
    unsigned mant_low  : 32;
    unsigned mant_high : 20;
    unsigned exp       : 11;
    unsigned sign      : 1;
  };

  double_t(double _d) { d = _d;   }
  operator double &() { return d; }
};

inline int32_t sample_mant(sample_t s)
{
  uint32_t mant;
  double_t d = s;

  mant = 0x40000000 | (d.mant_high << 10) | (d.mant_low >> 22);
  mant = (mant ^ (unsigned)(-(int)d.sign)) + d.sign;
  return (int32_t)mant;
}

inline int16_t sample_exp(sample_t s)
{
  double_t d = s;
  return int16_t(d.exp) - 1023 + 1;
}

#else

union float_t
{
  float f;
  struct 
  {
    unsigned mantissa:23;
    unsigned exponent:8;
    unsigned sign:1;
  };
};


inline int32_t sample_mant(sample_t s);
{
  int32_t i32;
  i32 = float_t(s).mant_high;
  (uint32_t)i32 |= float_t(s).sign << 31;
  return i32;
}

inline int sample_exp(sample_t s)
{
  return float_t(s).exp;
}

#endif


int test_one_float(sample_t s, int32_t mant, int16_t exp);


int test_float()
{
  int err = 0;
  printf("\n* Floating-point consistency test\n");

  sample_t s;
  s = 1.0;

  err += test_one_float(+2.00, +0x40000000, +2);
  err += test_one_float(-2.00, -0x40000000, +2);

  err += test_one_float(+1.00, +0x40000000, +1);
  err += test_one_float(-1.00, -0x40000000, +1);

  err += test_one_float(+0.50, +0x40000000, 0);
  err += test_one_float(-0.50, -0x40000000, 0);

  err += test_one_float(+0.25, +0x40000000, -1);
  err += test_one_float(-0.25, -0x40000000, -1);

  err += test_one_float(+0.125, +0x40000000, -2);
  err += test_one_float(-0.125, -0x40000000, -2);

  err += test_one_float(+0.75, +0x60000000, 0);
  err += test_one_float(-0.75, -0x60000000, 0);

  return err;
}


int test_one_float(sample_t s, int32_t mant, int16_t exp)
{
  if (sample_mant(s) != mant || sample_exp(s) != exp)
  {
    printf("!!!Error: s = %0.4f; mant = %.4f (%.4f), exp = %i (%i)\n", 
      s, 
      double(sample_mant(s))/2147483648, 
      double(mant)/2147483648, 
      sample_exp(s),
      exp);
    return 1;
  }
  return 0;
}