#include "bitstream.h"

void 
ReadBS::set_ptr(const uint8_t *_buf, int _type)
{
  int align;

  start = (uint32_t *)_buf;
  type = _type;

  align = (long)_buf & 3;
  pos = (uint32_t *) (_buf - align);
  bits_left = 0;
  get(align * 8);
}

uint32_t
ReadBS::get_bh(uint32_t num_bits)
{
  uint32_t result;

  num_bits -= bits_left;
  result = ((current_word << (32 - bits_left)) >> (32 - bits_left));

  fill_current();

  if(num_bits != 0)
    result = (result << num_bits) | ((current_word << (32 - bits_left)) >> (32 - num_bits));

  bits_left -= num_bits;

  return result;
}

int32_t
ReadBS::get_bh_signed(uint32_t num_bits)
{
  int32_t result;

  num_bits -= bits_left;
  result = ((((int32_t)current_word) << (32 - bits_left)) >> (32 - bits_left));

  fill_current();

  if(num_bits != 0)
    result = (result << num_bits) | ((current_word << (32 - bits_left)) >> (32 - num_bits));
        
  bits_left -= num_bits;

  return result;
}

ReadBS::get_pos() const
{
  return (pos - start) * 32 - bits_left;
}


void 
WriteBS::set_ptr(uint8_t *buffer, int buffer_size)
{
  buf      = buffer;
  buf_end  = buf + buffer_size;
  data_out_size = 0;

  buf_ptr  = buf;
  bit_left = 32;
  bit_buf  = 0;
}

void 
WriteBS::flush()
{
  bit_buf <<= bit_left;
  while (bit_left < 32) 
  {
    // todo: should test end of buffer
    *buf_ptr ++= bit_buf >> 24;
    bit_buf  <<= 8;
    bit_left  += 8;
  }
  bit_left = 32;
  bit_buf  = 0;
}

WriteBS::get_pos() const
{
  return (buf_ptr - buf) * 8 + 32 - bit_left;
}
