/*
  Simple test for IEEE floating point format compatibility
*/

#include <stdio.h>
#include "defs.h"
#include "..\log.h"

///////////////////////////////////////////////////////////////////////////////
// Main test
///////////////////////////////////////////////////////////////////////////////

int test_one_float(Log * log, sample_t s, int32_t mant, int16_t exp);
int test_float(Log *log)
{
  int err = 0;
  log->open_group("Floating-point consistency test");

  test_one_float(log, +2.00, +0x40000000, +2);
  test_one_float(log, -2.00, -0x40000000, +2);

  test_one_float(log, +1.00, +0x40000000, +1);
  test_one_float(log, -1.00, -0x40000000, +1);

  test_one_float(log, +0.50, +0x40000000, 0);
  test_one_float(log, -0.50, -0x40000000, 0);

  test_one_float(log, +0.25, +0x40000000, -1);
  test_one_float(log, -0.25, -0x40000000, -1);

  test_one_float(log, +0.125, +0x40000000, -2);
  test_one_float(log, -0.125, -0x40000000, -2);

  test_one_float(log, +0.75, +0x60000000, 0);
  test_one_float(log, -0.75, -0x60000000, 0);

  return log->close_group();
}


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

#else // #ifndef FLOAT_SAMPLE

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

#endif // #ifndef FLOAT_SAMPLE ... #else ... 

int test_one_float(Log * log, sample_t s, int32_t mant, int16_t exp)
{
  if (sample_mant(s) != mant || sample_exp(s) != exp)
  {
    log->err("s = %0.4f; mant = %.4f (%.4f), exp = %i (%i)", 
      s, 
      double(sample_mant(s))/2147483648, 
      double(mant)/2147483648, 
      sample_exp(s),
      exp);
    return 1;
  }
  return 0;
}