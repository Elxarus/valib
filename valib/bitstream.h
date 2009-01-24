/*
  Bitstream operations
  Read & Write classes

  Important note!!!
  -----------------
  Low endian streams MUST be aligned to stream word boundary
  (16 bit for 14 and 16 bit stream and 32 bit for 32 bit stream)

  We must not use ReadBS::get(0).
*/

#ifndef VALIB_BITSTREAM_H              
#define VALIB_BITSTREAM_H

#include "defs.h"

///////////////////////////////////////////////////////////////////////////////

class ReadBS2
{
private:
  /////////////////////////////////////////////////////////
  // start
  //   Aligned pointer to the start of the stream. May
  //   point before the actual start of the stream.
  // start_bit
  //   start + start_bit points to the actual starting bit
  //   of the stream
  // stream_size_bits
  //   Stream size in bits;
  // pos
  //   Pointer to the next word.
  // current_word, bits_left
  //   Current word of the stream and amount of the bits
  //   left unread.

  const uint32_t *start;
  size_t start_bit;
  size_t size_bits;

  const uint32_t *pos;
  uint32_t current_word;
  unsigned bits_left;

  uint32_t get_next(unsigned num_bits);
  int32_t  get_next_signed(unsigned num_bits);

public:
  ReadBS2();

  void set(const uint8_t *buf, size_t start_bit, size_t size_bits);

  void   set_pos_bits(size_t pos_bits);
  size_t get_pos_bits() const { return (pos - start) * 32 - bits_left - start_bit; }

  inline uint32_t get(unsigned num_bits);
  inline int32_t  get_signed(unsigned num_bits);
  inline bool     get_bool();
};

class WriteBS2
{
private:
  /////////////////////////////////////////////////////////
  // start
  //   Aligned pointer to the start of the stream. May
  //   point before the actual start of the stream.
  // start_bit
  //   start + start_bit points to the actual starting bit
  //   of the stream
  // stream_size_bits
  //   Stream size in bits;
  // pos
  //   Pointer to the next word.
  // current_word, bits_left
  //   Current word of the stream and amount of the bits
  //   left unread.

  uint32_t *start;
  size_t start_bit;
  size_t size_bits;

  uint32_t *pos;
  uint32_t current_word;
  unsigned bits_left;

  void put_next(unsigned num_bits, uint32_t value);

public:
  WriteBS2();

  void set(uint8_t *buf, size_t start_bit, size_t size_bits);

  void   set_pos_bits(size_t pos_bits);
  size_t get_pos_bits() const { return (pos - start) * 32 - bits_left - start_bit; }

  inline void put(unsigned num_bits, uint32_t value);
  inline void put_bool(bool value);
  inline void flush();
};

///////////////////////////////////////////////////////////////////////////////

inline uint32_t
ReadBS2::get(unsigned num_bits)
{
  assert(num_bits <= 32);
  assert(get_pos_bits() + num_bits <= size_bits);
  assert(bits_left > 0);

  if (num_bits == 0)
    return 0;

  if (num_bits < bits_left) 
  {
    uint32_t result;
    result = (current_word << (32 - bits_left)) >> (32 - num_bits);
    bits_left -= num_bits;
    return result;
  }

  return get_next(num_bits);
}

inline int32_t
ReadBS2::get_signed(unsigned num_bits)
{
  assert(num_bits <= 32);
  assert(get_pos_bits() + num_bits <= size_bits);
  assert(bits_left > 0);

  if (num_bits == 0)
    return 0;
        
  if (num_bits < bits_left) 
  {
    int32_t result;
    result = int32_t(current_word << (32 - bits_left)) >> (32 - num_bits);
    bits_left -= num_bits;
    return result;
  }

  return get_next_signed(num_bits);
}

inline bool
ReadBS2::get_bool()
{
  return get(1) != 0;
}

///////////////////////////////////////////////////////////////////////////////

inline void
WriteBS2::put(unsigned num_bits, uint32_t value)
{
  assert(num_bits <= 32);
  assert(num_bits == 32 || (value >> num_bits) == 0);
  assert(get_pos_bits() + num_bits <= size_bits);
  assert(bits_left > 0);

  if (num_bits == 0)
    return;

  if (num_bits < bits_left) 
  {
    current_word = (current_word << num_bits) | value;
    bits_left -= num_bits;
  }
  else
    put_next(num_bits, value);
}

inline void
WriteBS2::put_bool(bool value)
{
  put(1, value);
}

///////////////////////////////////////////////////////////////////////////////

class ReadBS
{
private:
  const uint32_t *start;
  const uint32_t *pos;

  unsigned  bits_left;
  uint32_t  current_word;
  int       type;      // BITSTREAM_XXXX constants
  
  inline void fill_current();
  uint32_t get_bh(unsigned num_bits);
  int32_t  get_bh_signed(unsigned num_bits);

public:
  void set_ptr(const uint8_t *buf, int type = BITSTREAM_8);

  inline uint32_t get(unsigned num_bits);
  inline int32_t  get_signed(unsigned num_bits);
  inline bool     get_bool();

  inline int get_type() const { return type; }
  size_t get_pos() const;
};


class WriteBS
{
public:
  uint32_t bit_buf;
  unsigned bit_left;

  uint8_t *buf;           // output buffer begin
  uint8_t *buf_end;       // output buffer end
  uint8_t *buf_ptr;       // current buffer pointer

public:
  void set_ptr(uint8_t *data, size_t data_size);

  void put_bits(unsigned num_bits, int value);
  void put_bool(bool value);
  void flush();

  inline uint8_t *get_buf() const { return buf;     }
  inline uint8_t *get_end() const { return buf_end; }
  inline uint8_t *get_ptr() const { return buf_ptr; }
  size_t get_pos() const;
};

///////////////////////////////////////////////////////////////////////////////
// Bitstream conversion functions
//
// All conversion functions take input buffer, process it to output and return
// number of output bytes. Note that only 2 conversion functions change data
// size: conversion from and to 14bit stream.
//
// All conversion functions can work in-place. I.e. you can specify the same
// buffer as input and output. But you cannot use overlapped buffers.
//
// find_conversion()
//   Tries to find a conversion function between 2 stream types.
//   Returns 0 if conversion was not found.
//
// bs_conv_none()
//   No stream conversion required. Just copies input buffer to output.
//   Output size is equal to input size.
//
// bs_conv_swab16()
//   Swaps bytes in 16bit words.
//   If input size is off it adds a zero byte to the end of the stream.
//   Output size is equal to input size.
//
// bs_conv_8_14be()
//   Convert byte stream to 14bit stream (16bit words with 14 data bits).
//   Output size is 8/7 larger than input.
//
// bs_conv_14be_8()
//   Convert 14bit stream (16bit words with 14 data bits) to byte stream.
//   Input size MUST be even.
//   Output size is 7/8 smaller than input.

size_t bs_convert(const uint8_t *in_buf, size_t size, int in_bs, uint8_t *out_buf, int out_bs);

typedef size_t (*bs_conv_t)(const uint8_t *in_buf, size_t size, uint8_t *out_buf);
bs_conv_t bs_conversion(int bs_from, int bs_to);

size_t bs_conv_copy(const uint8_t *in_buf, size_t size, uint8_t *out_buf);
size_t bs_conv_swab16(const uint8_t *in_buf, size_t size, uint8_t *out_buf);

size_t bs_conv_8_14be(const uint8_t *in_buf, size_t size, uint8_t *out_buf);
size_t bs_conv_8_14le(const uint8_t *in_buf, size_t size, uint8_t *out_buf);
size_t bs_conv_14be_8(const uint8_t *in_buf, size_t size, uint8_t *out_buf);
size_t bs_conv_14le_8(const uint8_t *in_buf, size_t size, uint8_t *out_buf);

size_t bs_conv_16le_14be(const uint8_t *in_buf, size_t size, uint8_t *out_buf);
size_t bs_conv_16le_14le(const uint8_t *in_buf, size_t size, uint8_t *out_buf);
size_t bs_conv_14be_16le(const uint8_t *in_buf, size_t size, uint8_t *out_buf);
size_t bs_conv_14le_16le(const uint8_t *in_buf, size_t size, uint8_t *out_buf);



///////////////////////////////////////////////////////////////////////////////
// ReadBS inlines
//

inline void 
ReadBS::fill_current()
{
  uint32_t tmp;
  tmp = *(pos++);
  switch (type)
  {
    case BITSTREAM_8:
    case BITSTREAM_16BE: 
    case BITSTREAM_32BE: 
      current_word = swab_u32(tmp);
      bits_left = 32;
      break;

    case BITSTREAM_16LE:
      current_word = (tmp >> 16) | (tmp << 16);
      bits_left = 32;
      break;

    case BITSTREAM_14BE:
      tmp = swab_u32(tmp);
      current_word = (tmp & 0x3fff) | ((tmp & 0x3fff0000) >> 2);
      bits_left = 28;
      break;

    case BITSTREAM_14LE:
      tmp = (tmp >> 16) | (tmp << 16);
      current_word = (tmp & 0x3fff) | ((tmp & 0x3fff0000) >> 2);
      bits_left = 28;
      break;

    default:
      current_word = 0;
  }
}

inline uint32_t 
ReadBS::get(unsigned num_bits)
{
  uint32_t result;

  if (num_bits < bits_left) 
  {
    result = (current_word << (32 - bits_left)) >> (32 - num_bits);
    bits_left -= num_bits;
    return result;
  }

  return get_bh(num_bits);
}

inline int32_t 
ReadBS::get_signed(unsigned num_bits)
{
  int32_t result;
        
  if (num_bits < bits_left) 
  {
    result = (((int32_t)current_word) << (32 - bits_left)) >> (32 - num_bits);
    bits_left -= num_bits;
    return result;
  }

  return get_bh_signed(num_bits);
}

inline bool 
ReadBS::get_bool()
{
  return get(1) != 0;
}

///////////////////////////////////////////////////////////////////////////////
// WriteBS inlines
//

inline void
WriteBS::put_bits(unsigned num_bits, int value)
{
  if (num_bits < bit_left) 
  {
    bit_buf = (bit_buf << num_bits) | value;
    bit_left -= num_bits;
  } 
  else 
  {
    bit_buf <<= bit_left;
    bit_buf |= value >> (num_bits - bit_left);
    *(uint32_t *)buf_ptr = swab_u32(bit_buf);
    buf_ptr  += 4;
    bit_left += 32 - num_bits;
    bit_buf   = value;
  }
}

inline void
WriteBS::put_bool(bool value)
{
  int v = value & 1;
  if (bit_left > 1) 
  {
    bit_buf = (bit_buf << 1) | v;
    bit_left--;
  } 
  else 
  {
    bit_buf = (bit_buf << 1) | v;
    *(uint32_t *)buf_ptr = swab_u32(bit_buf);
    buf_ptr  += 4;
    bit_left += 31;
    bit_buf   = value;
  }
}

#endif
