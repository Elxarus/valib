#ifndef VALIB_NOISE_H
#define VALIB_NOISE_H

#include "filter.h"

/*
  Random number generator is based on rg_rand.c by Ray Gardner:

  longrand() -- generate 2**31-2 random numbers
  based on "Random Number Generators: Good Ones Are Hard to Find",
  S.K. Park and K.W. Miller, Communications of the ACM 31:10 (Oct 1988),
  and "Two Fast Implementations of the 'Minimal Standard' Random
  Number Generator", David G. Carta, Comm. ACM 33, 1 (Jan 1990), p. 87-88
  linear congruential generator f(z) = 16807 z mod (2 ** 31 - 1)
  uses L. Schrage's method to avoid overflow problems
*/


class RNG
{
protected:
  long seed;

public:
  RNG(long seed = 1);

  void set(long seed);
  long next();
  long get();
};



class Noise : public Source
{
protected:
  RNG rng;

  Speakers  spk;
  SampleBuf buf;
  size_t    buf_size;
  size_t    data_size;

public:
  Noise(Speakers spk = def_spk, size_t data_size = 65536, size_t buf_size = 4096);

  bool set_output(Speakers spk, size_t data_size = 65536, size_t buf_size = 4096);

  void   set_seed(long seed);
  size_t get_data_size();
  void   set_data_size(size_t _data_size);

  // Source interface
  virtual Speakers get_output() const;
  virtual bool is_empty() const;
  virtual bool get_chunk(Chunk *chunk);
};

#endif