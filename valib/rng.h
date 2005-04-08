#ifndef RNG_H
#define RNG_H

#include "defs.h"

/*
  Random number generator is based on rg_rand.c by Ray Gardner:

  longrand() -- generate 2**31-2 random numbers
  based on "Random Number Generators: Good Ones Are Hard to Find",
  S.K. Park and K.W. Miller, Communications of the ACM 31:10 (Oct 1988),
  and "Two Fast Implementations of the 'Minimal Standard' Random
  Number Generator", David G. Carta, Comm. ACM 33, 1 (Jan 1990), p. 87-88
  linear congruential generator f(z) = 16807 z mod (2 ** 31 - 1)
  uses L. Schrage's method to avoid overflow problems

  -----------------------------------------------------------------------
  RNG class is random numbers generator.

  xxxx_int()   functions return number in range [-2^31-1...+2^31-1] or [-range...+range]
  xxxx_uint()  functions return number in range [0...2^32] or [0...range]
  xxxx_float() functions return number in range [-1...+1] or [-range...+range]

  void set(long seed) initialize generator

  following functions return next number in sequence but do not change
  generator's state. So successive calls will return the same number.

  int32_t  next_int() const;
  uint32_t next_uint() const;
  float    next_float() const;

  int32_t  next_int(int32_t range) const;
  uint32_t next_uint(uint32_t range) const;
  float    next_float(float range) const;

  following functions return next number in sequence and update
  generator's state. So successive calls will return successive
  numbers in sequence.

  int32_t  get_int()                number in range 
  uint32_t get_uint()               number in range [0...+2^32-2]
  float    get_float()              number in range [-1...1]

  int32_t  get_int(int32_t range)   number in range [-range...+range]
  uint32_t get_uint(uint32_t range) number in range [-range...+range]
  float    get_float(float range)   number in range [-range...+range]
*/


class RNG
{
protected:
  int32_t seed;
  int32_t next();

public:
  RNG(int32_t seed = 562343462);

  void set(int32_t seed);

  int32_t  next_int()   const { return seed; }
  uint32_t next_uint()  const { return uint32_t(seed); }
  float    next_float() const { return float(seed/2147483647.0); }

  int32_t  next_int(int32_t range) const;
  uint32_t next_uint(uint32_t range) const;
  float    next_float(float range) const;

  int32_t  get_int()   { int32_t  r = next_int();   seed = next(); return r; }
  uint32_t get_uint()  { uint32_t r = next_uint();  seed = next(); return r; }
  float    get_float() { float    r = next_float(); seed = next(); return r; }

  int32_t  get_int(int32_t range)   { int32_t  r = next_int(range);   seed = next(); return r; }
  uint32_t get_uint(uint32_t range) { uint32_t r = next_uint(range);  seed = next(); return r; }
  float    get_float(float range)   { float    r = next_float(range); seed = next(); return r; }
};

#endif
