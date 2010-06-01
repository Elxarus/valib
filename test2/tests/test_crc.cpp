/*
  CRC test
*/

#include <boost/test/unit_test.hpp>
#include "../noise_buf.h"
#include "crc.h"

static const int seed = 3476032;

///////////////////////////////////////////////////////////////////////////////
// Test bytestream interface
// * test different message lengths
// * test different message shifts

static void bytestream_test(int poly, int power, const char *poly_name)
{
  static const int max_size = 16;
  static const int max_shift = 16;

  BOOST_MESSAGE("Bytestream test with " << poly_name << " polinomial");

  CRC crc(poly, power);
  RawNoise buf(max_size + max_shift + 1, seed);

  for (int size = 0; size < max_size; size++)
    for (int shift = 0; shift < max_shift; shift++)
    {
      // Reference crc
      uint32_t ref_crc = 0;
      for (int i = 0; i < size; i++)
        ref_crc = crc.calc(ref_crc, buf[shift + i], 8);

      // Test crc using byte stream interface
      uint32_t test_crc = crc.calc(0, buf + shift, size);

      BOOST_CHECK_EQUAL(test_crc, ref_crc);
    }
}


///////////////////////////////////////////////////////////////////////////////
// Test bitstream interface
// * test different message lengths
// * test different message shifts

void bitstream_test(int poly, int power, const char *poly_name)
{
  static const int max_size = 16*8;  // in bits
  static const int max_shift = 16*8; // in bits

  BOOST_MESSAGE("Bitstream test with " << poly_name << " polinomial");

  CRC crc(poly, power);
  RawNoise buf(max_size + max_shift + 1, seed);

  for (int size = 0; size < max_size; size++)
    for (int shift = 0; shift < max_shift; shift++)
    {
      int start_byte = shift / 8;
      int start_bit  = shift % 8;
      int end_byte   = (shift + size) / 8;
      int end_bit    = (shift + size) % 8;

      // Reference crc

      uint32_t ref_crc = 0;
      if (start_byte == end_byte)
        ref_crc = crc.calc(ref_crc, buf[start_byte] >> (8 - end_bit), size);
      else
      {
        ref_crc = crc.calc(ref_crc, buf[start_byte], 8 - start_bit);
        for (int i = start_byte + 1; i < end_byte; i++)
          ref_crc = crc.calc(ref_crc, buf[i], 8);
        ref_crc = crc.calc(ref_crc, buf[end_byte] >> (8 - end_bit), end_bit);
      }

      // Test crc

      uint32_t test_crc = crc.calc(0, buf, shift, size);

      BOOST_CHECK_EQUAL(test_crc, ref_crc);
    }
}

///////////////////////////////////////////////////////////////////////////////

BOOST_AUTO_TEST_SUITE(crc)

BOOST_AUTO_TEST_CASE(message)
{
  static const uint8_t test_message[] =
  { 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39 };

  BOOST_CHECK_EQUAL(crc16.calc(0, test_message, sizeof(test_message)), 0xfee8);
  BOOST_CHECK_EQUAL(crc16.calc(0, test_message, 0, sizeof(test_message) * 8), 0xfee8);
  BOOST_CHECK_EQUAL(crc32.calc(0xFFFFFFFF, test_message, sizeof(test_message)), 0x376e6e7);
  BOOST_CHECK_EQUAL(crc32.calc(0xFFFFFFFF, test_message, 0, sizeof(test_message) * 8), 0x376e6e7);
}

BOOST_AUTO_TEST_CASE(bytestream)
{
  bytestream_test(POLY_CRC16, 16, "CRC16");
  bytestream_test(POLY_CRC32, 32, "CRC32");
}

BOOST_AUTO_TEST_CASE(bitstream)
{
  bitstream_test(POLY_CRC16, 16, "CRC16");
  bitstream_test(POLY_CRC32, 32, "CRC32");
}

BOOST_AUTO_TEST_SUITE_END()
