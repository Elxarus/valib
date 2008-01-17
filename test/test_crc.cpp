/*
  Test CRC class

  * Test CRC correctness for bitstream and bytestream interface using 
    2 standard polinomials (crc16 and crc32), different bitstream types, 
    message lengths and shifts
  * Test CRC of large noise buffer with different bitstreams
  * Test CRC speed of different bitstreams
  * Test speed of simple CRC16 table algorithm
*/

#include "log.h"
#include "crc.h"
#include "rng.h"
#include "source\raw_source.h"
#include "win32\cpu.h"

static const vtime_t time_per_test = 1.0; // 1 sec for each speed test
static const int size = 10000000;         // use 10MB noise buffer
static const int err_dist = 12689;        // generate error each N bytes

const int bs_type[] = 
{
  BITSTREAM_8, 
  BITSTREAM_16BE, BITSTREAM_16LE,
  BITSTREAM_32BE, BITSTREAM_32LE,
  BITSTREAM_14BE, BITSTREAM_14LE,
};
const char *bs_name[] = 
{
  "8bit",
  "16bit BE", "16bit LE",
  "32bit BE", "32bit LE",
  "14bit BE", "14bit LE",
};
const int word_size[] = 
{
  1, 
  2, 2, 
  4, 4,
  2, 2,
};
const int word_bits[] =
{
  8,
  16, 16,
  32, 32,
  14, 14,
};


///////////////////////////////////////////////////////////////////////////////
// Test class
///////////////////////////////////////////////////////////////////////////////

class CRCTest
{
protected:
  Log *log;
  CRC crc;

public:
  CRCTest(Log *_log)
  {
    log = _log;
  }

  int test()
  {
    log->open_group("CRC test");
    math_test();
    speed_test();
    return log->close_group();
  }

  ///////////////////////////////////////////////////////////////////////////////
  // Test math

  void math_test()
  {
    log->open_group("CRC math test");
    bytestream_test(POLY_CRC16, 16, "CRC16");
    bytestream_test(POLY_CRC32, 32, "CRC32");
    bitstream_test(POLY_CRC16, 16, "CRC16");
    bitstream_test(POLY_CRC32, 32, "CRC32");
    log->close_group();
  }

  ///////////////////////////////////////////////////////////////////////////////
  // Test speed of all bitstream types

  void speed_test()
  {
    RNG rng(47564321);
    uint8_t *data = new uint8_t[size];
    rng.fill_raw(data, size);

    crc.init(POLY_CRC16, 16);

    log->open_group("CRC speed test");
    speed_test_table(data, size, 0xc30d);
    speed_test(BITSTREAM_8,    "byte stream", data, size, 0xc30d0000);
    speed_test(BITSTREAM_14BE, "14bit BE",    data, size, 0xe7c70000);
    speed_test(BITSTREAM_14LE, "14bit LE",    data, size, 0x91e80000);
    speed_test(BITSTREAM_16BE, "16bit BE",    data, size, 0xc30d0000);
    speed_test(BITSTREAM_16LE, "16bit LE",    data, size, 0x56860000);
    speed_test(BITSTREAM_32BE, "32bit BE",    data, size, 0xc30d0000);
    speed_test(BITSTREAM_32LE, "32bit LE",    data, size, 0xabff0000);
    log->close_group();

    delete data;
  }

  ///////////////////////////////////////////////////////////////////////////////
  // Test bytestream interface
  // * test different bitstream types
  // * test different message lengths
  // * test different message shifts

  int bytestream_test(int poly, int power, const char *poly_name)
  {
    static const int max_size = 16;
    static const int max_shift = 16;

    int i;
    RNG rng;
    uint8_t buf[max_size + max_shift + 1];
    uint32_t crc_msg;
    uint32_t crc_test;

    log->msg("Bytestream test with %s polinomial", poly_name);

    crc.init(poly, power);
    for (int bs_index = 0; bs_index < array_size(bs_type); bs_index++)
      for (int size = 0; size < max_size; size += word_size[bs_index])
        for (int shift = 0; shift < max_shift; shift++)
        {
          rng.fill_raw(buf, sizeof(buf));

          // calc message reference crc
          crc_msg = 0;
          for (i = 0; i < size; i += word_size[bs_index])
          {
            uint32_t value;
            switch (bs_type[bs_index])
            {
              case BITSTREAM_8:    value = *(buf + shift + i); break;
              case BITSTREAM_14BE: value = be2uint16(*(uint16_t *)(buf + shift + i)); break;
              case BITSTREAM_14LE: value = le2uint16(*(uint16_t *)(buf + shift + i)); break;
              case BITSTREAM_16BE: value = be2uint16(*(uint16_t *)(buf + shift + i)); break;
              case BITSTREAM_16LE: value = le2uint16(*(uint16_t *)(buf + shift + i)); break;
              case BITSTREAM_32BE: value = be2uint32(*(uint32_t *)(buf + shift + i)); break;
              case BITSTREAM_32LE: value = le2uint32(*(uint32_t *)(buf + shift + i)); break;
              default: assert(false);
            }
            crc_msg = crc.add_bits(crc_msg, value, word_bits[bs_index]);
          }

          // calc message test crc
          crc_test = 0;
          crc_test = crc.calc(crc_test, buf + shift, size, bs_type[bs_index]);

          // test it
          if (crc_test != crc_msg)
            return log->err("bitstream: %s, size = %i, shift: %i, crc = 0x%x (must be 0x%x)", 
              bs_name[bs_index], size, shift, crc_test, crc_msg);
        }
    return 0;
  }

  ///////////////////////////////////////////////////////////////////////////////
  // Test bitstream interface
  // * test different bitstream types
  // * test different message lengths
  // * test different message shifts

  int bitstream_test(int poly, int power, const char *poly_name)
  {
    static const int max_size = 16*8;  // in bits!!!
    static const int max_shift = 16*8; // in bits!!!

    int i;
    RNG rng;
    uint8_t buf[(max_size + max_shift) / 8 + 1];
    uint32_t crc_msg;
    uint32_t crc_test;

    log->msg("Bitstream test with %s polinomial", poly_name);

    crc.init(poly, power);
    for (int bs_index = 0; bs_index < array_size(bs_type); bs_index++)
      for (int size = 0; size < max_size; size++)
        for (int shift = 0; shift < max_shift; shift++)
        {
          int bpw = word_bits[bs_index]; // bits per word
          int start_word = shift / bpw;
          int end_word   = (shift + size) / bpw;
          int start_bit  = shift % bpw;
          int end_bit    = (shift + size) % bpw;

          rng.fill_raw(buf, sizeof(buf));

          // calc message reference crc
          crc_msg = 0;
          uint32_t value;

          // prolog
          i = start_word * word_size[bs_index];
          switch (bs_type[bs_index])
          {
            case BITSTREAM_8:    value = *(buf + i); break;
            case BITSTREAM_14BE: value = be2uint16(*(uint16_t *)(buf + i)); break;
            case BITSTREAM_14LE: value = le2uint16(*(uint16_t *)(buf + i)); break;
            case BITSTREAM_16BE: value = be2uint16(*(uint16_t *)(buf + i)); break;
            case BITSTREAM_16LE: value = le2uint16(*(uint16_t *)(buf + i)); break;
            case BITSTREAM_32BE: value = be2uint32(*(uint32_t *)(buf + i)); break;
            case BITSTREAM_32LE: value = le2uint32(*(uint32_t *)(buf + i)); break;
            default: assert(false);
          }
          if (start_word == end_word)
            crc_msg = crc.add_bits(crc_msg, value >> (bpw - end_bit), size);
          else
          {
            crc_msg = crc.add_bits(crc_msg, value, bpw - start_bit);
            i += word_size[bs_index];

            // body
            while (i < end_word * word_size[bs_index])
            {
              switch (bs_type[bs_index])
              {
                case BITSTREAM_8:    value = *(buf + i); break;
                case BITSTREAM_14BE: value = be2uint16(*(uint16_t *)(buf + i)); break;
                case BITSTREAM_14LE: value = le2uint16(*(uint16_t *)(buf + i)); break;
                case BITSTREAM_16BE: value = be2uint16(*(uint16_t *)(buf + i)); break;
                case BITSTREAM_16LE: value = le2uint16(*(uint16_t *)(buf + i)); break;
                case BITSTREAM_32BE: value = be2uint32(*(uint32_t *)(buf + i)); break;
                case BITSTREAM_32LE: value = le2uint32(*(uint32_t *)(buf + i)); break;
                default: assert(false);
              }
              crc_msg = crc.add_bits(crc_msg, value, bpw);
              i += word_size[bs_index];
            }

            // epilog
            switch (bs_type[bs_index])
            {
              case BITSTREAM_8:    value = *(buf + i); break;
              case BITSTREAM_14BE: value = be2uint16(*(uint16_t *)(buf + i)); break;
              case BITSTREAM_14LE: value = le2uint16(*(uint16_t *)(buf + i)); break;
              case BITSTREAM_16BE: value = be2uint16(*(uint16_t *)(buf + i)); break;
              case BITSTREAM_16LE: value = le2uint16(*(uint16_t *)(buf + i)); break;
              case BITSTREAM_32BE: value = be2uint32(*(uint32_t *)(buf + i)); break;
              case BITSTREAM_32LE: value = le2uint32(*(uint32_t *)(buf + i)); break;
              default: assert(false);
            }
            crc_msg = crc.add_bits(crc_msg, value >> (bpw - end_bit), end_bit);
          }

          // calc message test crc
          crc_test = 0;
          crc_test = crc.calc_bits(crc_test, buf, shift, size, bs_type[bs_index]);

          // test it
          if (crc_test != crc_msg)
            return log->err("bitstream: %s, size = %i, shift: %i, crc = 0x%x (must be 0x%x)", 
              bs_name[bs_index], size, shift, crc_test, crc_msg);
        }
    return 0;
  }

  ///////////////////////////////////////////////////////////////////////////////
  // Simple table algorithm speed test

  int speed_test_table(uint8_t *data, size_t size, uint32_t crc_test)
  {
    // table method speed test
    uint32_t result = 0;
    CPUMeter cpu;

    int runs = 0;
    cpu.start();
    while (cpu.get_thread_time() < time_per_test)
    {
      runs++;
      result = calc_crc(0, data, size);
    }
    cpu.stop();

    log->msg("CRC speed (table method): %iMB/s",
      int(double(size) * runs / cpu.get_thread_time() / 1000000));

    if (result != crc_test)
      log->err("crc = 0x%08x but must be 0x%08x", result, crc_test);

    return log->get_errors();
  }

  ///////////////////////////////////////////////////////////////////////////////
  // Speed test
  // Just calc CRC of large block

  int speed_test(int bs_type, const char *bs_text, uint8_t *data, size_t size, uint32_t crc_test)
  {
    uint32_t result = 0;
    CPUMeter cpu;

    int runs = 0;
    cpu.start();
    while (cpu.get_thread_time() < time_per_test)
    {
      runs++;
      result = crc.calc(0, data, size, bs_type);
    }
    cpu.stop();

    log->msg("CRC %s speed: %iMB/s", bs_text,
      int(double(size) * runs / cpu.get_thread_time() / 1000000));

    if (result != crc_test)
      log->err("crc = 0x%08x but must be 0x%08x", result, crc_test);

    return log->get_errors();
  }

};


///////////////////////////////////////////////////////////////////////////////
// Test function
///////////////////////////////////////////////////////////////////////////////

int test_crc(Log *log)
{
  CRCTest test(log);
  return test.test();
}
