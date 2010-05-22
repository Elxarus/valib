/*
  Bitstream operations
  Read & Write classes

  Important note!!!
  -----------------
  Low endian streams MUST be aligned to stream word boundary
  (16 bit for 14 and 16 bit stream and 32 bit for 32 bit stream)

  ReadBS
  ======
  Bitstream reader class.

  void set(const uint8_t *buf, size_t start_bit, size_t size_bits)
    Attach the bitstream to the buffer given. Start of the stream is set to the
    start_bit after the pointer given, so you can start reading form any point
    of the bit stream, not only from the byte boundary.

  void set_pos_bits(size_t pos_bits);
    Set position in the bit stream, counting from the starting bit specified at
    set().

  size_t get_pos_bits() const
    Return the current position in the bit stream relative to the position
    specified at set().

  uint32_t get(unsigned num_bits)
    Read 'num_bits' bits from the bit stream and move the current position
    ahead. get(0) returns 0.

  int32_t get_signed(unsigned num_bits);
    Read 'num_bits' bits from the bitstream and move the current position ahead.
    Value is considered to be signed, i.e. the first bit contains the sign.
    get_signed(0) returns 0.

  bool get_bool()
    Read one bit and interpret it as boolean value.



  WriteBS
  =======
  Bitstream writer class.

  void set(uint8_t *buf, size_t start_bit, size_t size_bits)
    Attach the bitstream to the buffer given. Start of the stream is set to the
    start_bit after the pointer given, so you can start reading form any point
    of the bit stream, not only from the byte boundary.

  void set_pos_bits(size_t pos_bits);
    Set position in the bit stream, counting from the starting bit specified at
    set().

  size_t get_pos_bits() const
    Return the current position in the bit stream relative to the position
    specified at set().

  void put(unsigned num_bits, uint32_t value)
    Write 'num_bits' bits from 'value' into bitstream.
    Note, that memory may not be immediately updated. To be sure that you have
    all changes written, call flush().

  void put_bool(bool value)
    Write a boolean value as one bit into the bit stream.
    Note, that memory may not be immediately updated. To be sure that you have
    all changes written, call flush().

  void flush()
    Write uncommited changes into the memory.



  Bitstream conversion functions
  ==============================

  All conversion functions take input buffer, process it to output and return
  number of output bytes. Note that only 2 conversion functions change data
  size: conversion from and to 14bit stream.

  All conversion functions can work in-place. I.e. you can specify the same
  buffer as input and output. But you cannot use overlapped buffers.

  typedef bs_conv_t;
    Type definition for the conversion function.

  size_t bs_convert(const uint8_t *in_buf, size_t size, int in_bs, uint8_t *out_buf, int out_bs);
    Converts the stream from one bitstream type to another.

  bs_conv_t bs_conversion(int bs_from, int bs_to);
    Tries to find a conversion function between 2 stream types.
    Returns the conversion function pointer, or null if it is no direct
    conversion function between the stream types. If no conversion required,
    bs_conv_copy is returned.

  bs_conv_copy
    No conversion. Just copies input buffer to output.
    Output size is equal to input size.

  bs_conv_swab16()
    Swaps bytes in 16bit words.
    If input size is off it adds a zero byte to the end of the stream.
    Output size is equal to input size.
*/

#ifndef VALIB_BITSTREAM_H              
#define VALIB_BITSTREAM_H

#include "defs.h"

///////////////////////////////////////////////////////////////////////////////
// ReadBS - bitstream reader
///////////////////////////////////////////////////////////////////////////////

class ReadBS
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
  ReadBS();
  ReadBS(const uint8_t *buf, size_t start_bit, size_t size_bits);

  void set(const uint8_t *buf, size_t start_bit, size_t size_bits);

  void   set_pos_bits(size_t pos_bits);
  size_t get_pos_bits() const { return (pos - start) * 32 - bits_left - start_bit; }

  inline uint32_t get(unsigned num_bits);
  inline int32_t  get_signed(unsigned num_bits);
  inline bool     get_bool();
};

class WriteBS
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
  //   Pointer to the current word.
  // current_word, bits_left
  //   Current word of the stream and amount of the bits
  //   left unread.

  uint32_t *start;
  size_t start_bit;
  size_t size_bits;

  uint32_t *pos;
  uint32_t current_word;
  unsigned bits_left;

  void move(size_t pos_bits);
  void put_next(unsigned num_bits, uint32_t value);

public:
  WriteBS();
  WriteBS(uint8_t *buf, size_t start_bit, size_t size_bits);

  void set(uint8_t *buf, size_t start_bit, size_t size_bits);

  void   set_pos_bits(size_t pos_bits);
  size_t get_pos_bits() const { return (pos - start + 1) * 32 - bits_left - start_bit; }

  inline void put(unsigned num_bits, uint32_t value);
  inline void put_bool(bool value);
  void flush();
};

///////////////////////////////////////////////////////////////////////////////

inline uint32_t
ReadBS::get(unsigned num_bits)
{
  uint32_t result;
  assert(num_bits <= 32);
  assert(get_pos_bits() + num_bits <= size_bits);
  assert(bits_left > 0);

  if (num_bits == 0)
    return 0;

  if (num_bits < bits_left) 
  {
    result = (current_word << (32 - bits_left)) >> (32 - num_bits);
    bits_left -= num_bits;
    return result;
  }
  else
    return get_next(num_bits);
}

inline int32_t
ReadBS::get_signed(unsigned num_bits)
{
  int32_t result;
  assert(num_bits <= 32);
  assert(get_pos_bits() + num_bits <= size_bits);
  assert(bits_left > 0);

  if (num_bits == 0)
    return 0;
        
  if (num_bits < bits_left) 
  {
    result = int32_t(current_word << (32 - bits_left)) >> (32 - num_bits);
    bits_left -= num_bits;
    return result;
  }
  else
    return get_next_signed(num_bits);
}

inline bool
ReadBS::get_bool()
{
  return get(1) != 0;
}

///////////////////////////////////////////////////////////////////////////////

inline void
WriteBS::put(unsigned num_bits, uint32_t value)
{
  assert(num_bits <= 32);
  assert(num_bits == 32 || (value >> num_bits) == 0);
  assert(get_pos_bits() + num_bits <= size_bits);
  assert(bits_left > 0);

  if (num_bits == 0)
    return;

  if (num_bits < bits_left) 
  {
    bits_left -= num_bits;
    current_word = current_word | (value << bits_left);
  }
  else
    put_next(num_bits, value);
}

inline void
WriteBS::put_bool(bool value)
{
  put(1, value);
}

///////////////////////////////////////////////////////////////////////////////

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

#endif
