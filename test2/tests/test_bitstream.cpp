/*
  ReadBS test
  WriteBS test
  stream conversion functions test
*/

#include "bitstream.h"
#include "../noise_buf.h"
#include <boost/test/unit_test.hpp>

static const int seed = 4967205;
static const size_t max_align = 64;
static const size_t max_start_bit = 64;
static const size_t block_size = 512*1024; // block size for conversion test

static uint32_t get_uint32(uint8_t *buf, size_t start_bit)
{
  unsigned shift = start_bit & 7;
  uint32_t dword = be2uint32(*(uint32_t *)(buf + start_bit / 8));
  uint8_t next_byte = *(buf + start_bit / 8 + 4);
  return (dword << shift) | (next_byte >> (8 - shift));
}

static const int bs_types[] =
{
  BITSTREAM_8,
  BITSTREAM_16BE, BITSTREAM_16LE,
  //BITSTREAM_32BE, BITSTREAM_32LE,
  BITSTREAM_14BE, BITSTREAM_14LE
};

static bool is_14bit(int bs_type)
{
  return bs_type == BITSTREAM_14BE || bs_type == BITSTREAM_14LE;
}


static const char *bs_name(int bs_type)
{
  switch (bs_type)
  {
    case BITSTREAM_8:    return "byte";
    case BITSTREAM_16BE: return "16be";
    case BITSTREAM_16LE: return "16le";
    case BITSTREAM_32BE: return "32be";
    case BITSTREAM_32LE: return "32le";
    case BITSTREAM_14BE: return "14be";
    case BITSTREAM_14LE: return "14le";
    default: return "??";
  }
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
  ReadBS bs;
  RawNoise buf(max_align + max_start_bit / 8 + 4, seed);

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
  ReadBS bs;
  RawNoise buf(max_align + max_start_bit / 8 + 4, seed);

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
  ReadBS bs;
  RawNoise buf(max_align + max_start_bit / 8 + 4, seed);

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
  RawNoise buf(max_align + max_start_bit / 8 + 4, seed);

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
  RawNoise buf(max_align + max_start_bit / 8 + 4, seed);

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
  WriteBS bs;
  RawNoise buf(max_align + max_start_bit / 8 + 4, seed);

  for (uint8_t *pos = buf; pos < buf + max_align; pos++)
  {
    buf.fill_noise();
    for (size_t start_bit = 0; start_bit < max_start_bit; start_bit++)
    {
      uint32_t test = buf.rng.next();

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
  WriteBS bs;
  RawNoise buf(max_align + max_start_bit / 8 + 4, seed);

  for (size_t pos_bits = 0; pos_bits < max_start_bit; pos_bits++)
  {
    uint32_t test = buf.rng.next();

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
  WriteBS bs;
  RawNoise buf(max_align + max_start_bit / 8 + 4, seed);

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
  RawNoise buf(max_align + max_start_bit / 8 + 4, seed);

  // Test all number of bits
  for (unsigned num_bits = 1; num_bits <= 32; num_bits++)
  {
    *(uint32_t *)buf.begin() = buf.rng.next();
    uint32_t test = buf.rng.next() >> (32 - num_bits);

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
      *(uint32_t *)buf.begin() = buf.rng.next();
      uint32_t test1 = buf.rng.next() >> (32 - i);
      uint32_t test2 = buf.rng.next() >> (32 - j);

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
  RawNoise buf(max_align + max_start_bit / 8 + 4, seed);

  // Data before start_bit must remain unchanged
  // Test different start positions
  for (uint8_t *pos = buf + 4; pos < buf + max_align; pos++)
    for (size_t start_bit = 0; start_bit < max_start_bit; start_bit++)
    {
      buf.fill_noise();
      uint32_t test = get_uint32(pos - 4, start_bit);

      bs.set(pos, start_bit, 32);
      bs.put(32, buf.rng.next());
      bs.flush();

      uint32_t result = get_uint32(pos - 4, start_bit);
      BOOST_CHECK_EQUAL(result, test);
    }

  // Data after the last bit changed must remain unchanged
  // Test all combinations of num_bits and bits_left
  for (unsigned i = 0; i <= 32; i++)
    for (unsigned j = 0; j <= 32; j++)
    {
      buf.fill_noise();
      uint32_t test = get_uint32(buf, i + j);
      uint32_t v1 = (i == 0)? 0: buf.rng.next() >> (32 - i);
      uint32_t v2 = (j == 0)? 0: buf.rng.next() >> (32 - j);

      bs.set(buf, 0, 64);
      bs.put(i, v1);
      bs.put(j, v2);
      bs.flush();

      uint32_t result = get_uint32(buf, i + j);
      BOOST_CHECK_EQUAL(result, test);
    }

  // set_pos_bits() must flush at old position
  *(uint32_t *)buf.begin() = buf.rng.next();
  uint32_t test = buf.rng.next();

  bs.set(buf, 0, 64);
  bs.put(32, test);
  bs.set_pos_bits(0);

  uint32_t result = get_uint32(buf, 0);
  BOOST_CHECK_EQUAL(result, test);
}

///////////////////////////////////////////////////////////
// 2-step and 3-step bitstream conversion test

static void bs_convert_test(int bs_type1, int bs_type2, int bs_type3 = -1)
{
  bs_conv_t conv1 = 0, conv2 = 0, conv3 = 0;

  conv1 = bs_conversion(bs_type1, bs_type2);
  BOOST_REQUIRE(conv1 != 0);

  if (bs_type3 == -1)
  {
    conv2 = bs_conversion(bs_type2, bs_type1);
    BOOST_REQUIRE(conv2 != 0);
  }
  else
  {
    conv2 = bs_conversion(bs_type2, bs_type3);
    conv3 = bs_conversion(bs_type3, bs_type1);
    BOOST_REQUIRE(conv2 != 0);
    BOOST_REQUIRE(conv3 != 0);
    BOOST_TEST_MESSAGE("Conversion " << 
      bs_name(bs_type1) << "-" << 
      bs_name(bs_type2) << "-" << 
      bs_name(bs_type3));
  }

  // Prepare buffers
  Rawdata ref_buf(block_size);
  Rawdata buf1(block_size);
  Rawdata buf2(block_size);
  RNG(seed).fill_raw(ref_buf, ref_buf.size());

  size_t ref_size = ref_buf.size();

  // Conversion to 14bit increases amount of data,
  // therefore we must decrease the size of source data
  if (!is_14bit(bs_type1) && (is_14bit(bs_type2) || is_14bit(bs_type3)))
  {
    ref_size /= 8;
    ref_size *= 7;
  }

  // 14bit source must have even buffer size.
  // To maintain buffer size unchanged it must be multiple of 8.
  // Also we must clear 2 high bits for 14bit source stream.
  if (is_14bit(bs_type1))
  {
    ref_size &= ~7;

    size_t n = ref_size;
    if (bs_type1 == BITSTREAM_14LE)
      n++;

    while (n > 1)
    {
      n -= 2;
      ref_buf[n] &= 0x3f;
    }
  }

  size_t size1, size2;

  /////////////////////////////////////////////////////////
  // Inplace conversion test

  memcpy(buf1, ref_buf, ref_size);
  size1 = ref_size;

  size1 = conv1(buf1, size1, buf1);
  size1 = conv2(buf1, size1, buf1);
  if (conv3)
    size1 = conv3(buf1, size1, buf1);

  BOOST_CHECK_EQUAL(size1, ref_size);
  BOOST_CHECK(memcmp(buf1, ref_buf, ref_size) == 0);

  /////////////////////////////////////////////////////////
  // Copy conversion test

  uint8_t *check_buf = buf1;
  size_t check_size = size1;

  memcpy(buf1, ref_buf, ref_size);
  size1 = ref_size;

  buf2.zero();
  size2 = conv1(buf1, size1, buf2);

  buf1.zero();
  size1 = conv2(buf2, size2, buf1);

  if (conv3)
  {
    buf2.zero();
    size2 = conv3(buf1, size1, buf2);
    check_buf = buf2;
    check_size = size2;
  }

  BOOST_CHECK_EQUAL(check_size, ref_size);
  BOOST_CHECK(memcmp(check_buf, ref_buf, ref_size) == 0);
}

BOOST_AUTO_TEST_CASE(bs_convert_2step)
{
  for (int i = 0; i < array_size(bs_types); i++)
    for (int j = 0; j < array_size(bs_types); j++)
      if (i != j)
      {
        BOOST_TEST_MESSAGE("Conversion " << 
          bs_name(bs_types[i]) << "-" << 
          bs_name(bs_types[j]));
        bs_convert_test(bs_types[i], bs_types[j]);
      }
}

BOOST_AUTO_TEST_CASE(bs_convert_3step)
{
  for (int i = 0; i < array_size(bs_types); i++)
    for (int j = 0; j < array_size(bs_types); j++)
      for (int k = 0; k < array_size(bs_types); k++)
        if ((i != j) && (i != k) && (j != k))
        {
          BOOST_TEST_MESSAGE("Conversion " << 
            bs_name(bs_types[i]) << "-" << 
            bs_name(bs_types[j]) << "-" << 
            bs_name(bs_types[k]));
          bs_convert_test(bs_types[i], bs_types[j], bs_types[k]);
        }
}

BOOST_AUTO_TEST_SUITE_END()
