/*
  ReadBS test
  WriteBS test
  stream conversion functions test
*/

#include "rng.h"
#include "buffer.h"
#include "bitstream.h"
#include <boost/test/unit_test.hpp>

static const int seed = 4967205;
static const size_t max_align = 64;
static const size_t max_start_bit = 64;

static uint32_t get_uint32(uint8_t *buf, size_t start_bit)
{
  unsigned shift = start_bit & 7;
  uint32_t dword = be2uint32(*(uint32_t *)(buf + start_bit / 8));
  uint8_t next_byte = *(buf + start_bit / 8 + 4);
  return (dword << shift) | (next_byte >> (8 - shift));
}

///////////////////////////////////////////////////////////////////////////////
// ReadBS
///////////////////////////////////////////////////////////////////////////////

BOOST_AUTO_TEST_SUITE(read_bs)

///////////////////////////////////////////////////////////
// ReadBS::set()
// Test different start positions
//
// Start position consists of the buffer pointer and
// start_bit. Note that ReadBS aligns the pointer, so we
// have to test different pointer alignments.

BOOST_AUTO_TEST_CASE(set)
{
  RNG rng(seed);
  Rawdata buf(max_align + max_start_bit / 8 + 4);
  rng.fill_raw(buf, buf.size());

  ReadBS bs;
  for (uint8_t *pos = buf; pos < buf + max_align; pos++)
    for (size_t start_bit = 0; start_bit < max_start_bit; start_bit++)
    {
      bs.set(pos, start_bit, 32);
      uint32_t result = bs.get(32);
      uint32_t test = get_uint32(pos, start_bit);
      BOOST_CHECK_EQUAL(result, test);
    }
}

///////////////////////////////////////////////////////////
// ReadBS::set_bit_pos()

BOOST_AUTO_TEST_CASE(set_bit_pos)
{
  RNG rng(seed);
  Rawdata buf(max_align + max_start_bit / 8 + 4);
  rng.fill_raw(buf, buf.size());

  ReadBS bs;
  for (size_t pos_bits = 0; pos_bits < max_start_bit; pos_bits++)
  {
    bs.set(buf, 0, max_start_bit + 32);
    bs.set_pos_bits(pos_bits);
    uint32_t result = bs.get(32);
    uint32_t test = get_uint32(buf, pos_bits);
    BOOST_CHECK_EQUAL(result, test);
  }
}

///////////////////////////////////////////////////////////
// ReadBS::get_bit_pos()
// Bit position is calculated from all of the following:
// start pointer, current pointer, start_bit and bits_left.
// Therefore we have to check it all...

BOOST_AUTO_TEST_CASE(get_bit_pos)
{
  RNG rng(seed);
  Rawdata buf(max_align + max_start_bit / 8 + 4);
  rng.fill_raw(buf, buf.size());

  ReadBS bs;
  for (uint8_t *pos = buf; pos < buf + max_align; pos++)
    for (size_t start_bit = 0; start_bit < max_start_bit; start_bit++)
      for (size_t pos_bits = 0; pos_bits <= 32; pos_bits++)
      {
        bs.set(pos, start_bit, 32);
        bs.set_pos_bits(pos_bits);
        size_t test_pos_bits = bs.get_pos_bits();

        // ~100K variants enumerated.
        // BOOST_CHECK_EQUAL's logging is too slow for this.
        if (test_pos_bits != pos_bits)
          BOOST_CHECK_EQUAL(test_pos_bits, pos_bits);
      }
}

///////////////////////////////////////////////////////////
// ReadBS::get()

BOOST_AUTO_TEST_CASE(get)
{
  RNG rng(seed);
  Rawdata buf(max_align + max_start_bit / 8 + 4);
  rng.fill_raw(buf, buf.size());

  // Read zero bits, should return zero
  ReadBS bs(buf, 0, 32);
  uint32_t result = bs.get(0);
  BOOST_CHECK(result == 0);

  // Test all number of bits
  uint32_t test = be2uint32(*(uint32_t*)buf.begin());
  for (unsigned num_bits = 1; num_bits <= 32; num_bits++)
  {
    bs.set(buf, 0, 32);
    result = bs.get(num_bits);
    BOOST_CHECK_EQUAL(result, test >> (32 - num_bits));
  }

  // Test all combinations of num_bits and bits_left
  for (unsigned i = 0; i <= 32; i++)
  {
    test = get_uint32(buf, i);
    for (unsigned j = 0; j <= 32; j++)
    {
      bs.set(buf, 0, 64);
      result = bs.get(i);
      result = bs.get(j);
      if (j == 0)
        BOOST_CHECK(result == 0);
      else
        BOOST_CHECK_EQUAL(result, test >> (32 - j));
    }
  }
}

///////////////////////////////////////////////////////////
// ReadBS::get_signed()

BOOST_AUTO_TEST_CASE(get_signed)
{
  RNG rng(seed);
  Rawdata buf(max_align + max_start_bit / 8 + 4);
  rng.fill_raw(buf, buf.size());

  // Read zero bits, should return zero
  ReadBS bs(buf, 0, 64);
  uint32_t result = bs.get_signed(0);
  BOOST_CHECK(result == 0);

  // Test all number of bits
  // Note that we have to test both sings
  for (int sign = 0; sign <= 1; sign++)
  {
    *(uint32_t *)buf.begin() = ~*(uint32_t *)buf.begin();
    int32_t test = be2uint32(*(uint32_t*)buf.begin());
    for (unsigned num_bits = 1; num_bits <= 32; num_bits++)
    {
      bs.set(buf, 0, 32);
      result = bs.get_signed(num_bits);
      BOOST_CHECK_EQUAL(result, test >> (32 - num_bits));
    }
  }

  // Test all combinations of num_bits and bits_left
  for (int sign = 0; sign <= 1; sign++)
  {
    *(uint32_t *)buf.begin() = ~*(uint32_t *)buf.begin();
    for (unsigned i = 0; i <= 32; i++)
    {
      int32_t test_signed = get_uint32(buf, i);
      for (unsigned j = 0; j <= 32; j++)
      {
        bs.set(buf, 0, 64);
        result = bs.get_signed(i);
        result = bs.get_signed(j);
        if (j == 0)
          BOOST_CHECK(result == 0);
        else
          BOOST_CHECK_EQUAL(result, test_signed >> (32 - j));
      }
    }
  }
}

BOOST_AUTO_TEST_SUITE_END()


///////////////////////////////////////////////////////////////////////////////
// WriteBS test
///////////////////////////////////////////////////////////////////////////////

BOOST_AUTO_TEST_SUITE(write_bs)

///////////////////////////////////////////////////////////
// WriteBS::set()
// Test different start positions
// Start position consists of the buffer pointer and
// start_bit. Note that WriteBS aligns the pointer, so we
// have to test different pointer alignments.

BOOST_AUTO_TEST_CASE(set)
{
  RNG rng(seed);
  Rawdata buf(max_align + max_start_bit / 8 + 4);

  WriteBS bs;
  for (uint8_t *pos = buf; pos < buf + max_align; pos++)
  {
    rng.fill_raw(buf, buf.size());
    for (size_t start_bit = 0; start_bit < max_start_bit; start_bit++)
    {
      uint32_t test = rng.next();

      bs.set(pos, start_bit, 32);
      bs.put(32, test);
      bs.flush();

      uint32_t result = get_uint32(pos, start_bit);
      BOOST_CHECK_EQUAL(result, test);
    }
  }
}

///////////////////////////////////////////////////////////
// WriteBS::set_bit_pos()

BOOST_AUTO_TEST_CASE(set_bit_pos)
{
  RNG rng(seed);
  Rawdata buf(max_align + max_start_bit / 8 + 4);
  rng.fill_raw(buf, buf.size());

  WriteBS bs;
  for (size_t pos_bits = 0; pos_bits < max_start_bit; pos_bits++)
  {
    uint32_t test = rng.next();

    bs.set(buf, 0, max_start_bit + 32);
    bs.set_pos_bits(pos_bits);
    bs.put(32, test);
    bs.flush();

    uint32_t result = get_uint32(buf, pos_bits);
    BOOST_CHECK_EQUAL(result, test);
  }
}

///////////////////////////////////////////////////////////
// ReadBS::get_bit_pos()
// Bit position is calculated from all of the following:
// start pointer, current pointer, start_bit and bits_left.
// Therefore we have to check it all...

BOOST_AUTO_TEST_CASE(get_bit_pos)
{
  RNG rng(seed);
  Rawdata buf(max_align + max_start_bit / 8 + 4);
  rng.fill_raw(buf, buf.size());

  WriteBS bs;
  for (uint8_t *pos = buf; pos < buf + max_align; pos++)
    for (size_t start_bit = 0; start_bit < max_start_bit; start_bit++)
      for (unsigned pos_bits = 0; pos_bits <= 32; pos_bits++)
      {
        bs.set(pos, start_bit, 32);
        bs.set_pos_bits(pos_bits);
        size_t test_pos_bits = bs.get_pos_bits();

        // ~100K variants enumerated.
        // BOOST_CHECK_EQUAL's logging is too slow for this.
        if (test_pos_bits != pos_bits)
          BOOST_CHECK_EQUAL(test_pos_bits, pos_bits);
      }
}

///////////////////////////////////////////////////////////
// WriteBS::put()

BOOST_AUTO_TEST_CASE(put)
{
  WriteBS bs;
  RNG rng(seed);
  Rawdata buf(max_align + max_start_bit / 8 + 4);
  rng.fill_raw(buf, buf.size());

  // Test all number of bits
  for (unsigned num_bits = 1; num_bits <= 32; num_bits++)
  {
    *(uint32_t *)buf.begin() = rng.next();
    uint32_t test = rng.next() >> (32 - num_bits);

    bs.set(buf, 0, 32);
    bs.put(num_bits, test);
    bs.flush();

    uint32_t result = get_uint32(buf, 0) >> (32 - num_bits);
    BOOST_CHECK_EQUAL(result, test);
  }

  // Test all combinations of num_bits and bits_left
  for (unsigned i = 1; i <= 32; i++)
    for (unsigned j = 1; j <= 32; j++)
    {
      *(uint32_t *)buf.begin() = rng.next();
      uint32_t test1 = rng.next() >> (32 - i);
      uint32_t test2 = rng.next() >> (32 - j);

      bs.set(buf, 0, 64);
      bs.put(i, test1);
      bs.put(j, test2);
      bs.flush();

      uint32_t result = get_uint32(buf, i) >> (32 - j);
      BOOST_CHECK_EQUAL(result, test2);
    }
}

///////////////////////////////////////////////////////////
// WriteBS::flush()

BOOST_AUTO_TEST_CASE(flush)
{
  WriteBS bs;
  RNG rng(seed);
  Rawdata buf(max_align + max_start_bit / 8 + 4);

  // Data before start_bit must remain unchanged
  // Test different start positions
  for (uint8_t *pos = buf + 4; pos < buf + max_align; pos++)
    for (size_t start_bit = 0; start_bit < max_start_bit; start_bit++)
    {
      rng.fill_raw(buf, buf.size());
      uint32_t test = get_uint32(pos - 4, start_bit);

      bs.set(pos, start_bit, 32);
      bs.put(32, rng.next());
      bs.flush();

      uint32_t result = get_uint32(pos - 4, start_bit);
      BOOST_CHECK_EQUAL(result, test);
    }

  // Data after the last bit changed must remain unchanged
  // Test all combinations of num_bits and bits_left
  for (unsigned i = 0; i <= 32; i++)
    for (unsigned j = 0; j <= 32; j++)
    {
      rng.fill_raw(buf, buf.size());
      uint32_t test = get_uint32(buf, i + j);
      uint32_t v1 = (i == 0)? 0: rng.next() >> (32 - i);
      uint32_t v2 = (j == 0)? 0: rng.next() >> (32 - j);

      bs.set(buf, 0, 64);
      bs.put(i, v1);
      bs.put(j, v2);
      bs.flush();

      uint32_t result = get_uint32(buf, i + j);
      BOOST_CHECK_EQUAL(result, test);
    }

  // set_pos_bits() must flush at old position
  *(uint32_t *)buf.begin() = rng.next();
  uint32_t test = rng.next();

  bs.set(buf, 0, 64);
  bs.put(32, test);
  bs.set_pos_bits(0);

  uint32_t result = get_uint32(buf, 0);
  BOOST_CHECK_EQUAL(result, test);
}

BOOST_AUTO_TEST_SUITE_END()
