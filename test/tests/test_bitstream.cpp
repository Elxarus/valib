/*
  Bitstream classes
*/

#include <math.h>
#include "bitstream.h"
#include "rng.h"
#include "../suite.h"

static const int seed = 4796;
static const int noise_size = 64 * 1024;

///////////////////////////////////////////////////////////////////////////////
// ReadBS test
///////////////////////////////////////////////////////////////////////////////

TEST(bitstream_read, "ReadBS")
  ReadBS2 bs;
  uint32_t result;

  const size_t max_align = 64;
  const size_t max_start_bit = 64;

  const size_t buf_size = max_align + max_start_bit / 8 + 4;
  uint8_t buf[buf_size];
  RNG rng(seed);
  rng.fill_raw(buf, buf_size);

  /////////////////////////////////////////////////////////
  // Test different start positions
  // Start position consists of the buffer pointer and
  // start_bit. Note that ReadBS aligns the pointer, so we
  // have to test different pointer alignments.

  {
    for (uint8_t *pos = buf; pos < buf + max_align; pos++)
    {
      uint32_t test = be2uint32(*(uint32_t *)pos);
      uint8_t next = 0;

      for (size_t start_bit = 0; start_bit < max_start_bit; start_bit++)
      {
        bs.set(pos, start_bit, 32);
        result = bs.get(32);
        CHECKT(result == test, ("bs.set(pos=%i, start_bit=%i) fails", pos-buf, start_bit));

        // shift the test word
        if ((start_bit & 7) == 0)
          next = *(pos + 4 + start_bit / 8);
        test = (test << 1) | (next >> 7);
        next = (next << 1);
      }
    }
  }

  /////////////////////////////////////////////////////////
  // ReadBS::get()
  // Test all number of bits

  {
    bs.set(buf, 0, 32);
    result = bs.get(0);
    CHECKT(result == 0, ("bs.get(%i) fails", 0));

    uint32_t test = be2uint32(*(uint32_t*)buf);
    for (unsigned num_bits = 1; num_bits <= 32; num_bits++)
    {
      bs.set(buf, 0, 32);
      result = bs.get(num_bits);
      CHECKT(result == (test >> (32 - num_bits)), ("bs.get(%i) fails", num_bits));
    }
  }

  /////////////////////////////////////////////////////////
  // ReadBS::get()
  // Test all combinations of num_bits and bits_left

  {
    uint32_t test = be2uint32(*(uint32_t*)(buf + 0));
    uint32_t next = be2uint32(*(uint32_t*)(buf + 4));
    for (unsigned i = 0; i <= 32; i++)
    {
      for (unsigned j = 0; j <= 32; j++)
      {
        bs.set(buf, 0, 64);
        result = bs.get(i);
        result = bs.get(j);
        if (j == 0)
          CHECKT(result == 0, ("bs.get(%i); bs.get(%i); sequence fails", i, j))
        else
          CHECKT(result == (test >> (32 - j)), ("bs.get(%i); bs.get(%i); sequence fails", i, j));
      }
      test = (test << 1) | (next >> 31);
      next = (next << 1);
    }
  }

  /////////////////////////////////////////////////////////
  // ReadBS::get_signed()
  // Test all number of bits
  // Note that we have to test both sings

  {
    bs.set(buf, 0, 64);
    result = bs.get_signed(0);
    CHECKT(result == 0, ("bs.get_signed(%i) fails", 0));

    for (int sign = 0; sign <= 1; sign++)
    {
      *(uint32_t *)buf = ~*(uint32_t *)buf;
      int32_t test = be2uint32(*(uint32_t*)buf);
      for (unsigned num_bits = 1; num_bits <= 32; num_bits++)
      {
        bs.set(buf, 0, 32);
        result = bs.get_signed(num_bits);
        CHECKT(result == (test >> (32 - num_bits)), ("bs.get_signed(%i) fails", num_bits));
      }
    }
  }

  /////////////////////////////////////////////////////////
  // ReadBS::get_signed()
  // Test all combinations of num_bits and bits_left

  {
    for (int sign = 0; sign <= 1; sign++)
    {
      *(uint32_t *)buf = ~*(uint32_t *)buf;
      int32_t test = be2int32(*(int32_t*)(buf + 0));
      uint32_t next = be2uint32(*(uint32_t*)(buf + 4));
      for (unsigned i = 0; i <= 32; i++)
      {
        for (unsigned j = 0; j <= 32; j++)
        {
          bs.set(buf, 0, 64);
          result = bs.get_signed(i);
          result = bs.get_signed(j);
          if (j == 0)
            CHECKT(result == 0, ("bs.get(%i); bs.get(%i); sequence fails", i, j))
          else
            CHECKT(result == (test >> (32 - j)), ("bs.get(%i); bs.get(%i); sequence fails", i, j));
        }
        test = (test << 1) | (next >> 31);
        next = (next << 1);
      }
    }
  }

TEST_END(bitstream_read);

TEST(bitstream_write, "WriteBS")
  WriteBS2 bs;
  uint32_t result;

  CHECKT(false, ("Not implemented yet"));
TEST_END(bitstream_write);


///////////////////////////////////////////////////////////////////////////////
// Test suite
///////////////////////////////////////////////////////////////////////////////

SUITE(bitstream, "Bitstream")
  TEST_FACTORY(bitstream_read),
  TEST_FACTORY(bitstream_write),
SUITE_END;
