/*
  Random numbers generator test
  Test correctness of generation:
  z(10001) = 1043618065
*/

#include "suite.h"
#include "rng.h"

TEST(rng, "Random numbers generator test")
  const int seed = 9873485;
  // Seed constructor test
  RNG rng1(seed);
  RNG rng2(seed);
  CHECKT(rng1.get_int() == rng2.get_int(), ("Same seed must generate the same sequence"));

  // RNG::set() test
  rng2.set(seed);
  rng2.get_int(); // one got at the first test
  CHECKT(rng1.get_int() == rng2.get_int(), ("RNG::set() cannot set the generator into correct state"));

  int z;
  RNG rng(1);
  for (int i = 0; i < 10001; i++)
    z = rng.get_int();
  CHECKT(z == 1043618065, ("RNG fail!"));
TEST_END(rng);
