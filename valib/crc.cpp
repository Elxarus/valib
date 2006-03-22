#include "crc.h"

void
CRC::init(uint32_t _poly, size_t _power)
{
  // assert(_power <= 32)
  poly = _poly << (32 - _power);
  power = _power;
  for (int byte = 0; byte < 256; byte++)
    tbl[byte] = add_bits(0, byte, 8);
}

uint32_t 
CRC::bitstream(uint32_t crc, uint8_t *data, size_t start_bit, size_t bits, int bs_type)
{
  /////////////////////////////////////////////////////////////////////////////
  // Bitstream consists of 3 parts: head, body and tail:
  //
  //                       +--- head --+------------- body ------------+--- tail ---+
  //                       v           v                               v            v
  //   012345678901234567890123456789010123456789012345678901234567890101234567890123456789
  // --+-------------------------------+----------\ \------------------+-------------------
  //   #       |       |       |       #       |  / /  |       |       #       |       |
  // --+-------------------------------+----------\ \------------------+-------------------
  //           ^           ^                                                        ^
  //           |           +- start bit  < ------ size in bits ------->   last bit -+
  //           +- data pointer
  //
  // Head consists of part of 32bit word. Note that start_bit specifies 
  // bitstream start position relative to data pointer in bits. Therefore:
  // 1. It may point to any byte after data pointer (not only in 32bit word).
  //    So data pointer may not point to head word.
  // 3. Byte position must be calculated considering number of valid bits per 
  //    word. Note that 14bit DTS stream contain 14 valid bits per 16bit word:
  //    (1) start_byte = start_bit / 8 (for 8/16/32bit stream)
  //    (2) start_byte = start_bit / 7 (for 14bit DTS stream)

  return 0;
}

uint32_t 
CRC::bytestream(uint32_t crc, uint8_t *data, size_t bytes, int bs_type)
{
  while (bytes--)
    crc = add_8(crc, *data++);
  return crc;
}


/*

/////////////////////////////////////////////////////////////////////////////////
// OLD STUFF

  inline void reset(uint32_t _crc) { crc = _crc << (32 - poly_power); }
  inline uint32_t get_crc()        { return crc >> (32 - poly_power); }

  inline uint32_t add_bits(uint32_t _data, size_t _bits);
  { crc = (crc << _bits) ^ tbl[_bits][(crc >> (32 - _bits)) ^ _data]; }

  inline uint32_t add_byte(uint8_t _data);
  { crc = (crc << 8) ^ tbl[_bits][(crc >> 24) ^ _data]; }

  inline uint32_t add_word(uint16_t _data);
  {
    crc ^=  ((uint32_t)_data) << 16;
    crc = (crc << 8) ^ tbl[8][crc >> 24];
    crc = (crc << 8) ^ tbl[8][crc >> 24];
  }

  inline uint32_t add_dword(uint32_t _data);
  {
    crc ^=  _data;
    crc = (crc << 8) ^ tbl[8][crc >> 24];
    crc = (crc << 8) ^ tbl[8][crc >> 24];
    crc = (crc << 8) ^ tbl[8][crc >> 24];
    crc = (crc << 8) ^ tbl[8][crc >> 24];
  }

  inline void add_bytes(uint8_t *_data, size_t _bytes)
  {
    int i;

    if (bytes > 3)
    {
      // justify address
      if (_data & 1) { add_byte(*_data); _data++; }
      if (_data & 2) { add_word(*(uint16_t *)_data); _data += 2; }
      _bytes -= _data & 3;

      // main cycle
      i = bytes >> 2;
      while (i--)
      {
        add_dword(*(uint32_t *)_data;
        data += 4;
      }
      _bytes -= _bytes & 3;
    }

    // process tail
    if (_data & 2) { add_word(*(uint16_t *)_data); _data += 2; }
    if (_data & 1) { add_byte(*_data); _data++; }
  }
*/
