/*
  Table CRC algorithm speed mainly depends on table access speed.
  If we increase table size (and decrease table accesses) it increases cache
  misses so 8bit table may be considered as optimal choise.

  Some words about 32bit access
  =============================
  This module uses 32bit access everywhere. Why?

  Conversion 16 <-> 32bit word sometimes takes much of time. In some cases it
  is not a problem: 16bit-le with 16bit access works almost at the same speed
  as 32bit access. But 16bit-be works 6 times slower on P4 and 2 times slower
  on P3 because of several 16 <-> 32bit conversions (before and after swab 
  function).

  Also 32bit access is about 30% faster on P4 and about 50% faster on P3 
  compared to simple byte-access CRC algorithm.
*/


#include "crc.h"

const CRC crc16(POLY_CRC16, 16);
const CRC crc32(POLY_CRC32, 32);

void
CRC::init(uint32_t _poly, uint8_t _power)
{
  int byte;
  assert(_power <= 32);

  poly = _poly << (32 - _power);
  power = _power;

  for (byte = 0; byte < 256; byte++)
    tbl[byte] = add_bits(0, byte, 8);

  for (byte = 0; byte < 64; byte++)
    tbl6[byte] = add_bits(0, byte, 6);
}

///////////////////////////////////////////////////////////////////////////////
// Byte stream interface
///////////////////////////////////////////////////////////////////////////////

uint32_t 
CRC::calc(uint32_t crc, uint8_t *data, size_t bytes, int bs_type) const
{
  switch (bs_type)
  {
    case BITSTREAM_8:    return calc(crc, data, bytes);
    case BITSTREAM_14BE: return calc_14be(crc, (uint16_t *)data, bytes >> 1);
    case BITSTREAM_14LE: return calc_14le(crc, (uint16_t *)data, bytes >> 1);
    case BITSTREAM_16BE: return calc(crc, data, bytes);
    case BITSTREAM_16LE: return calc_16le(crc, (uint16_t *)data, bytes >> 1);
    case BITSTREAM_32BE: return calc(crc, data, bytes);
    case BITSTREAM_32LE: return calc_32le(crc, (uint32_t *)data, bytes >> 2);
  }
  assert(false);
  return 0;
}


uint32_t 
CRC::calc(uint32_t crc, uint8_t *data, size_t size) const
{
  uint8_t *end = data + size;

  /////////////////////////////////////////////////////
  // Process unaligned start (8bit)
  // Because data pointer is unaligned we may need up
  // to 3 byte loads to align pointer to 32bit boundary.

  while ((data < end) && ((uint32_t)data & 3))
    crc = add_8(crc, *data++);

  /////////////////////////////////////////////////////
  // Process main block (32bit)

  uint32_t *data32 = (uint32_t *)data;
  uint32_t *end32  = (uint32_t *)((uint32_t)end & ~3);
  while (data32 < end32)
  {
    crc = add_32(crc, be2uint32(*data32));
    data32++;
  }

  /////////////////////////////////////////////////////
  // Process unaligned end (8bit)
  // We may need up to 3 byte loads to finish the 
  // stream.

  data = (uint8_t *)data32;
  while (data < end)
    crc = add_8(crc, *data++);

  return crc;
}

uint32_t 
CRC::calc_14be(uint32_t crc, uint16_t *data, size_t size) const
{
  /////////////////////////////////////////////////////
  // Process unaligned start (16bit)
  // We suppose that data pointer is aligned to 16bit 
  // and threrfore we may need only one 16bit load to
  // align pointer to 32bit boundary.

  if (size && ((uint32_t)data & 3))
  {
    crc = add_14(crc, be2uint16(*data));
    data++; 
    size--;
  }

  /////////////////////////////////////////////////////
  // Process main block (32bit)

  uint32_t *data32 = (uint32_t *)data;
  uint32_t *end32  = data32 + (size >> 1);
  while (data32 < end32)
  {
    uint32_t value = be2uint32(*data32);
    data32++;
    crc = add_14(crc, value >> 16);
    crc = add_14(crc, value);
  }

  /////////////////////////////////////////////////////
  // Process unaligned end (16bit)
  // We may need one 16bit load to finish the stream.

  if (size & 1)
  {
    data = (uint16_t *)data32;
    crc = add_14(crc, be2uint16(*data));
  }

  return crc;
}

inline uint32_t 
CRC::calc_14le(uint32_t crc, uint16_t *data, size_t size) const
{
  /////////////////////////////////////////////////////
  // Process unaligned start (16bit)
  // We suppose that data pointer is aligned to 16bit 
  // and threrfore we may need only one 16bit load to
  // align pointer to 32bit boundary.

  if (size && ((uint32_t)data & 3))
  {
    crc = add_14(crc, le2uint16(*data));
    data++; 
    size--;
  }

  /////////////////////////////////////////////////////
  // Process main block (32bit)

  uint32_t *data32 = (uint32_t *)data;
  uint32_t *end32  = data32 + (size >> 1);
  while (data32 < end32)
  {
    uint32_t value = le2uint32(*data32);
    data32++;
    crc = add_14(crc, value);
    crc = add_14(crc, value >> 16);
  }

  /////////////////////////////////////////////////////
  // Process unaligned end (16bit)
  // We may need one 16bit load to finish the stream.

  if (size & 1)
  {
    data = (uint16_t *)data32;
    crc = add_14(crc, le2uint16(*data));
  }

  return crc;
}

uint32_t 
CRC::calc_16be(uint32_t crc, uint16_t *data, size_t size) const
{
  // 16bit-be stream is equivalent to byte stream
  return calc(crc, (uint8_t *)data, size << 1);
}

uint32_t 
CRC::calc_16le(uint32_t crc, uint16_t *data, size_t size) const
{
  /////////////////////////////////////////////////////
  // Process unaligned start (16bit)
  // We suppose that data pointer is aligned to 16bit 
  // and threrfore we may need only one 16bit load to
  // align pointer to 32bit boundary.

  if (size && ((uint32_t)data & 3))
  {
    crc = add_16(crc, le2uint16(*data));
    data++; 
    size--;
  }

  /////////////////////////////////////////////////////
  // Process main block (32bit)

  uint32_t *data32 = (uint32_t *)data;
  uint32_t *end32  = data32 + (size >> 1);
  while (data32 < end32)
  {
    uint32_t value = le2uint32(*data32);
    data32++;
    crc = add_16(crc, value);
    crc = add_16(crc, value >> 16);
  }

  /////////////////////////////////////////////////////
  // Process unaligned end (16bit)
  // We may need one 16bit load to finish the stream.

  if (size & 1)
  {
    data = (uint16_t *)data32;
    crc = add_16(crc, le2uint16(*data));
  }

  return crc;
}

uint32_t 
CRC::calc_32be(uint32_t crc, uint32_t *data, size_t size) const
{
  // 32bit-be stream is equivalent to byte stream
  return calc(crc, (uint8_t *)data, size << 2);
}

uint32_t 
CRC::calc_32le(uint32_t crc, uint32_t *data, size_t size) const
{
  uint32_t *end = data + size;
  while (data < end)
  {
    crc = add_32(crc, le2uint32(*data));
    data++;
  }
  return crc;
}

///////////////////////////////////////////////////////////////////////////////
// Bit stream interface
// This interface is implemented as wrapper functions that add bit-level 
// prolog and epilog to byte stream functions.
///////////////////////////////////////////////////////////////////////////////

uint32_t 
CRC::calc_bits(uint32_t crc, uint8_t *data, size_t start_bit, size_t bits, int bs_type) const
{
  switch (bs_type)
  {
    case BITSTREAM_8:    return calc_bits(crc, data, start_bit, bits);
    case BITSTREAM_14BE: return calc_bits_14be(crc, (uint16_t *)data, start_bit, bits);
    case BITSTREAM_14LE: return calc_bits_14le(crc, (uint16_t *)data, start_bit, bits);
    case BITSTREAM_16BE: return calc_bits_16be(crc, (uint16_t *)data, start_bit, bits);
    case BITSTREAM_16LE: return calc_bits_16le(crc, (uint16_t *)data, start_bit, bits);
    case BITSTREAM_32BE: return calc_bits_32be(crc, (uint32_t *)data, start_bit, bits);
    case BITSTREAM_32LE: return calc_bits_32le(crc, (uint32_t *)data, start_bit, bits);
  }

  // cannot be here (bitstream is not supported?)
  assert(false);
  return 0;
}

uint32_t 
CRC::calc_bits(uint32_t crc, uint8_t *data, size_t start_bit, size_t bits) const
{
  data += start_bit >> 3;
  start_bit &= 7;

  int end_bit = start_bit + bits;
  int size = end_bit >> 3;
  end_bit &= 7;

  if (size)
  {
    // prolog
    crc = add_bits(crc, *data, 8 - start_bit);
    data++;
    // body
    crc = calc(crc, data, size-1);
    data += size-1;
    // epilog
    crc = add_bits(crc, (*data) >> (8 - end_bit), end_bit);
  }
  else
  {
    // all stream is in one word
    crc = add_bits(crc, (*data) >> (8 - end_bit), bits);
  }

  return crc;
}

uint32_t 
CRC::calc_bits_16be(uint32_t crc, uint16_t *data, size_t start_bit, size_t bits) const
{
  // 16bit-be stream is equivalent to general bit stream
  return calc_bits(crc, (uint8_t *)data, start_bit, bits);
}

uint32_t 
CRC::calc_bits_16le(uint32_t crc, uint16_t *data, size_t start_bit, size_t bits) const
{
  data += start_bit >> 4;
  start_bit &= 15;

  int end_bit = start_bit + bits;
  int size = end_bit >> 4;
  end_bit &= 15;

  if (size)
  {
    // prolog
    crc = add_bits(crc, le2uint16(*data), 16 - start_bit);
    data++;
    // body
    crc = calc_16le(crc, data, size-1);
    data += size-1;
    // epilog
    crc = add_bits(crc, le2uint16(*data) >> (16 - end_bit), end_bit);
  }
  else
  {
    // all stream is in one word
    crc = add_bits(crc, le2uint16(*data) >> (16 - end_bit), bits);
  }

  return crc;
}

uint32_t 
CRC::calc_bits_14be(uint32_t crc, uint16_t *data, size_t start_bit, size_t bits) const
{
  data += start_bit / 14;
  start_bit %= 14;

  int end_bit = start_bit + bits;
  int size = end_bit / 14;
  end_bit %= 14;

  if (size)
  {
    // prolog
    crc = add_bits(crc, be2uint16(*data), 14 - start_bit);
    data++;
    // body
    crc = calc_14be(crc, data, size-1);
    data += size-1;
    // epilog
    crc = add_bits(crc, be2uint16(*data) >> (14 - end_bit), end_bit);
  }
  else
  {
    // all stream is in one word
    crc = add_bits(crc, be2uint16(*data) >> (14 - end_bit), bits);
  }

  return crc;
}
uint32_t 
CRC::calc_bits_14le(uint32_t crc, uint16_t *data, size_t start_bit, size_t bits) const
{
  data += start_bit / 14;
  start_bit %= 14;

  int end_bit = start_bit + bits;
  int size = end_bit / 14;
  end_bit %= 14;

  if (size)
  {
    // prolog
    crc = add_bits(crc, le2uint16(*data), 14 - start_bit);
    data++;
    // body
    crc = calc_14le(crc, data, size-1);
    data += size-1;
    // epilog
    crc = add_bits(crc, le2uint16(*data) >> (14 - end_bit), end_bit);
  }
  else
  {
    // all stream is in one word
    crc = add_bits(crc, le2uint16(*data) >> (14 - end_bit), bits);
  }

  return crc;
}
uint32_t 
CRC::calc_bits_32be(uint32_t crc, uint32_t *data, size_t start_bit, size_t bits) const
{
  // 32bit-be stream is equivalent to general bit stream
  return calc_bits(crc, (uint8_t *)data, start_bit, bits);
}
uint32_t 
CRC::calc_bits_32le(uint32_t crc, uint32_t *data, size_t start_bit, size_t bits) const
{
  data += start_bit >> 5;
  start_bit &= 31;

  int end_bit = start_bit + bits;
  int size = end_bit >> 5;
  end_bit &= 31;

  if (size)
  {
    // prolog
    crc = add_bits(crc, le2uint32(*data), 32 - start_bit);
    data++;
    // body
    crc = calc_32le(crc, data, size-1);
    data += size-1;
    // epilog
    crc = add_bits(crc, le2uint32(*data) >> (32 - end_bit), end_bit);
  }
  else
  {
    // all stream is in one word
    crc = add_bits(crc, le2uint32(*data) >> (32 - end_bit), bits);
  }

  return crc;
}
