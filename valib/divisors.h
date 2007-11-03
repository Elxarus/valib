/*
  Divisors enumerator

  Enumerates all divisors of a number, prime divisors, calculates number of
  divisors.

  Optimized to work with sample rate conversion, therefore it factorizes
  numbers up to 1 000 000 fast using a primes table, but for larger numbers
  stupid direct divisor enumeration may be used. Threrefore in some cases
  it may be good to extend the primes table...

  Enumeration including trivial divisors:
  ---------------------------------------

  DivEnum d(m);
  for (int i = 0; i < d.divisors(); i++)
  {
    x = d.next();
    ....
  }

  Enumeration without trivial divisors:
  -------------------------------------

  DivEnum d(m); d.next();
  for (int i = 0; i < d.divisors() - 2; i++)
  {
    x = d.next();
    ....
  }

  Enumerate divisors twice:
  (without having to factorize the number again)
  ---------------------------------------

  DivEnum d(m);
  for (int i = 0; i < d.divisors(); i++)
  {
    x = d.next();
    ....
  }
  for (int i = 0; i < d.divisors(); i++)
  {
    x = d.next();
    ....
  }
*/

#ifndef DIVISORS_H
#define DIVISORS_H

#include "defs.h"

class DivEnum
{
protected:
  // 10 different primes is enough to factorize any 32bit number:
  // 2 * 3 * 5 * ... * 29 = 6 469 693 230 > 2^32
  int p[10];     // prime divisor
  int n[10];     // prime divisor power
  int c[10];     // current power of the prime
  int nprimes;   // number of prime divisors
  int ndivisors; // number of divisors (excluding trivial)

public:
  DivEnum();
  DivEnum(uint32_t num);

  int init(uint32_t num);
  uint32_t next();

  inline int divisors() const { return ndivisors; }
  inline int prime_divisors() const { return nprimes; }
  inline int prime_divisor(int i) const { return i < nprimes? p[i]: 0; }
};

#endif
