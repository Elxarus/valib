#include <assert.h>
#include "crc.h"

void
CRC::init(uint32_t _poly, size_t _power)
{
  assert(_power <= 32);

  poly = _poly << (32 - _power);
  power = _power;
  for (int byte = 0; byte < 256; byte++)
    tbl[byte] = add_bits(0, byte, 8);
}

uint32_t 
CRC::calc(uint32_t crc, uint8_t *data, size_t bytes, int bs_type)
{
  switch (bs_type)
  {
    case BITSTREAM_8:
    case BITSTREAM_16BE:
    case BITSTREAM_32BE:
      return calc(crc, data, bytes);

    case BITSTREAM_16LE:
      return calc_16le(crc, data, bytes);

    case BITSTREAM_32LE:
      return calc_32le(crc, data, bytes);
  }

  // cannot be here (bitstream is not supported?)
  assert(false);
  return 0;
}


uint32_t 
CRC::calc(uint32_t crc, uint8_t *data, size_t bytes)
{
  /////////////////////////////////////////////////////
  // Byte stream or 16/32bit big endian stream

  uint8_t *end = data + bytes;

  /////////////////////////////////////////////////////
  // Process unaligned start (8bit)

  while ((data < end) && ((uint32_t)data & 3))
    crc = add_8(crc, *data++);

  /////////////////////////////////////////////////////
  // Process main block (32bit)

  uint32_t *data32 = (uint32_t *)data;
  uint32_t *end32  = (uint32_t *)((uint32_t)end & ~3);
  while (data32 < end32)
  {
    crc = add_32(crc, be2int32(*data32));
    data32++;
  }

  /////////////////////////////////////////////////////
  // Process unaligned end (8bit)

  data = (uint8_t *)data32;
  while (data < end)
    crc = add_8(crc, *data++);

  return crc;
}

uint32_t 
CRC::calc_16le(uint32_t crc, uint8_t *data, size_t bytes)
{
  /////////////////////////////////////////////////////
  // 16bit low endian stream
  // pointer must be aligned to 16bit boundary
  // number of bytes must be even

  assert(((uint32_t)data & 1) == 0);
  assert((bytes & 1) == 0);

  uint16_t *data16 = (uint16_t *)data;
  uint16_t *end16  = data16 + (bytes >> 1);

  /////////////////////////////////////////////////////
  // Process main block (16bit)

  while (data16 < end16)
  {
    crc = add_16(crc, le2int16(*data16));
    data16++;
  }

  return crc;
}

uint32_t 
CRC::calc_32le(uint32_t crc, uint8_t *data, size_t bytes)
{
  /////////////////////////////////////////////////////
  // 32bit low endian stream
  // pointer must be aligned to 32bit boundary
  // number of bytes must be multiply of 4

  assert(((uint32_t)data & 3) == 0);
  assert((bytes & 3) == 0);

  /////////////////////////////////////////////////////
  // Process main block (32bit)

  uint32_t *data32 = (uint32_t *)data;
  uint32_t *end32  = data32 + (bytes >> 2);
  while (data32 < end32)
  {
    crc = add_32(crc, le2int32(*data32));
    data32++;
  }

  return crc;
}
