#include <math.h>
#include "divisors.h"

static const int prime[] = {
     1,    2,    3,    5,    7,   11,   13,   17,   19,   23, 
    29,   31,   37,   41,   43,   47,   53,   59,   61,   67, 
    71,   73,   79,   83,   89,   97,  101,  103,  107,  109, 
   113,  127,  131,  137,  139,  149,  151,  157,  163,  167, 
   173,  179,  181,  191,  193,  197,  199,  211,  223,  227, 
   229,  233,  239,  241,  251,  257,  263,  269,  271,  277, 
   281,  283,  293,  307,  311,  313,  317,  331,  337,  347, 
   349,  353,  359,  367,  373,  379,  383,  389,  397,  401, 
   409,  419,  421,  431,  433,  439,  443,  449,  457,  461, 
   463,  467,  479,  487,  491,  499,  503,  509,  521,  523, 
   541,  547,  557,  563,  569,  571,  577,  587,  593,  599, 
   601,  607,  613,  617,  619,  631,  641,  643,  647,  653, 
   659,  661,  673,  677,  683,  691,  701,  709,  719,  727, 
   733,  739,  743,  751,  757,  761,  769,  773,  787,  797, 
   809,  811,  821,  823,  827,  829,  839,  853,  857,  859, 
   863,  877,  881,  883,  887,  907,  911,  919,  929,  937, 
   941,  947,  953,  967,  971,  977,  983,  991,  997, 1009, 
};

DivEnum::DivEnum()
{ 
  nprimes = 0;
  ndivisors = 0;
};

DivEnum::DivEnum(uint32_t _num)
{
  init(_num);
};

int
DivEnum::init(uint32_t _num)
{
  nprimes = 0;
  ndivisors = 0;

  /////////////////////////////////////////////////////////
  // Test prime divisors using a table

  int i = 1;
  int t = _num;
  while ((prime[i] * prime[i] <= t) && (i < array_size(prime)))
  {
    if (t % prime[i] == 0)
    {
      p[nprimes] = prime[i];
      n[nprimes] = 0;
      c[nprimes] = 0;
      do {
        n[nprimes]++;
        t /= prime[i];
      } while (t % prime[i] == 0);
      nprimes++;
    }
    i++;
  }

  if (i >= array_size(prime))
  {
    ///////////////////////////////////////////////////////
    // Table of prime divisors is too small
    // Enumerate divisors directly

    int max = (int)sqrt(t);
    int d = prime[array_size(prime)-1];
    while (d < max)
    {
      if (t % d == 0)
      {
        p[nprimes] = d;
        n[nprimes] = 0;
        c[nprimes] = 0;
        do {
          n[nprimes]++;
          t /= d;
        } while (t % d == 0);
        nprimes++;
      }
      d += 2;
    }
  }

  if (t != 1)
  {
    ///////////////////////////////////////////////////////
    // t is prime, a divisor too

    p[nprimes] = t;
    n[nprimes] = 1;
    c[nprimes] = 0;
    nprimes++;
  }

  /////////////////////////////////////////////////////////
  // Number of divisors

  ndivisors = 1;
  for (i = 0; i < nprimes; i++)
    ndivisors *= n[i]+1;

  return ndivisors;
}

uint32_t
DivEnum::next()
{
  int i, j;

  int d = 1;
  for (i = 0; i < nprimes; i++)
    for (j = 0; j < c[i]; j++)
      d *= p[i];

  i = 0;
  while (n[i] == c[i])
  {
    c[i++] = 0;
    if (i >= nprimes)
      return d;
  }
  c[i]++;
  return d;
};
