#include <memory.h>
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

  if (type == BITSTREAM_14BE || type == BITSTREAM_14LE)
    get(align * 7);
  else
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

size_t
ReadBS::get_pos() const
{
  return (pos - start) * 32 - bits_left;
}


void 
WriteBS::set_ptr(uint8_t *buffer, size_t buffer_size)
{
  buf      = buffer;
  buf_end  = buf + buffer_size;

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

size_t
WriteBS::get_pos() const
{
  return (buf_ptr - buf) * 8 + 32 - bit_left;
}

///////////////////////////////////////////////////////////////////////////////
// Bitstream conversion functions


static const int bs_index[] =
{
  BITSTREAM_8,
  BITSTREAM_16BE,
  BITSTREAM_16LE,
  BITSTREAM_14BE,
  BITSTREAM_14LE
};

static const bs_conv_t conv[7][7] = 
{ 
//        8              16be          16le              14be            14le
  { bs_conv_copy,   bs_conv_copy,   bs_conv_swab16,    bs_conv_8_14be,    bs_conv_8_14le    }, // 8
  { bs_conv_copy,   bs_conv_copy,   bs_conv_swab16,    bs_conv_8_14be,    bs_conv_8_14le    }, // 16be
  { bs_conv_swab16, bs_conv_swab16, bs_conv_copy,      bs_conv_16le_14be, bs_conv_16le_14le }, // 16le
  { bs_conv_14be_8, bs_conv_14be_8, bs_conv_14be_16le, bs_conv_copy,      bs_conv_swab16    }, // 14be
  { bs_conv_14le_8, bs_conv_14le_8, bs_conv_14le_16le, bs_conv_swab16,    bs_conv_copy      }, // 14le
};

bs_conv_t bs_conversion(int bs_from, int bs_to)
{
  int i = 0;
  int ibs_from = -1;
  int ibs_to = -1;

  for (i = 0; i < array_size(bs_index); i++)
    if (bs_index[i] == bs_from)
    {
      ibs_from = i;
      break;
    }

  for (i = 0; i < array_size(bs_index); i++)
    if (bs_index[i] == bs_to)
    {
      ibs_to = i;
      break;
    }

  if (ibs_from == -1 || ibs_to == -1)
    return false;
  else
    return conv[ibs_from][ibs_to];
}

size_t bs_convert(const uint8_t *in_buf, size_t size, int in_bs, uint8_t *out_buf, int out_bs)
{
  bs_conv_t conv = bs_conversion(in_bs, out_bs);
  if (conv)
    return (*conv)(in_buf, size, out_buf);
  else
    return 0;
}



size_t bs_conv_copy(const uint8_t *in_buf, size_t size, uint8_t *out_buf)
{ 
  memcpy(out_buf, in_buf, size);
  return size; 
}

size_t bs_conv_swab16(const uint8_t *in_buf, size_t size, uint8_t *out_buf)
{
  // If input size is odd we add a zero byte to the end.
  // Therefore output buffer size MUST BE LARGER than input buffer.

  if (size & 1)
    out_buf[size++] = 0;

  uint16_t *in16 = (uint16_t *)in_buf;
  uint16_t *out16 = (uint16_t *)out_buf;
  size_t i = size >> 1;
  while (i--)
    out16[i] = swab_u16(in16[i]);

  return size;
}

///////////////////////////////////////////////////////////////////////////////
//                               byte <-> 14be
///////////////////////////////////////////////////////////////////////////////

size_t bs_conv_8_14be(const uint8_t *in_buf, size_t size, uint8_t *out_buf)
{
  // We expand the buffer size so output buffer size MUST BE LARGER than
  // input size specified.

  // We can do inplace conversion. You can specify the same input and output
  // buffer pointers.

  // We convert each 7 bytes into 4 16bit words with 14 data bits each.
  // If input frame size is not multiply of 7 we add zeros to the end of the
  // frame to form integral number of 16bit (14bit) words. So output frame
  // size is always even. But in this case exact backward conversion is not
  // possible!

  static const int inc[7] = { 0, 2, 4, 4, 6, 6, 8 };

  size_t n = size / 7;
  size_t r = size % 7;
  const uint8_t *src = in_buf + n * 7;
  uint8_t *dst = out_buf + n * 8;

  size = n * 8 + inc[r];

  if (r)
  {
    // copy frame's tail and zero the rest
    size_t i = 0;
    for (; i < r; i++) dst[i] = src[i];
    for (; i < 7; i++) dst[i] = 0;

    // convert frame's tail
    uint32_t w1 = be2int32(*(uint32_t *)(dst + 0));
    uint32_t w2 = be2int32(*(uint32_t *)(dst + 3));
    w1 = ((w1 >> 2) & 0x3fff0000) | ((w1 >> 4) & 0x00003fff);
    w2 = ((w2 << 2) & 0x3fff0000) | (w2 & 0x00003fff);
    (*(uint32_t *)(dst + 0)) = int2be32(w1);
    (*(uint32_t *)(dst + 4)) = int2be32(w2);
  }

  src -= 7;
  dst -= 8;
  while (n--)
  {
    uint32_t w1 = be2int32(*(uint32_t *)(src + 0));
    uint32_t w2 = be2int32(*(uint32_t *)(src + 3));
    w1 = ((w1 >> 2) & 0x3fff0000) | ((w1 >> 4) & 0x00003fff);
    w2 = ((w2 << 2) & 0x3fff0000) | (w2 & 0x00003fff);
    (*(uint32_t *)(dst + 0)) = int2be32(w1);
    (*(uint32_t *)(dst + 4)) = int2be32(w2);

    src -= 7;
    dst -= 8;
  }

  return size;
}

size_t bs_conv_14be_8(const uint8_t *in_buf, size_t size, uint8_t *out_buf)
{
  // Input frame size MUST BE EVEN!!!
  assert((size & 1) == 0);

  // We convert each 4 16bit words with 14 data bits each into 7 bytes.
  // If input frame size is not multiply of 8 we add zeros to the end of the
  // frame to form integral number bytes. But in this case exact backward
  // conversion is not possible!

  size_t n = size / 8;
  size_t r = size % 8;
  const uint8_t *src = in_buf;
  uint8_t *dst = out_buf;

  size = n * 7 + r;

  while (n--)
  {
    uint32_t w1 = be2int32(*(uint32_t *)(src + 0));
    uint32_t w2 = be2int32(*(uint32_t *)(src + 4));
    w2 = ((w2 & 0x3fff0000) >> 2) | (w2 & 0x00003fff) | (w1 << 28);
    w1 = ((w1 & 0x3fff0000) << 2) | ((w1 & 0x00003fff) << 4);
    (*(uint32_t *)(dst + 0)) = int2be32(w1);
    (*(uint32_t *)(dst + 3)) = int2be32(w2);

    src += 8;
    dst += 7;
  }

  if (r)
  {
    size_t i = 0;
    for (; i < r; i++) dst[i] = src[i];
    for (; i < 8; i++) dst[i] = 0;

    uint32_t w1 = be2int32(*(uint32_t *)(dst + 0));
    uint32_t w2 = be2int32(*(uint32_t *)(dst + 4));
    w2 = ((w2 & 0x3fff0000) >> 2) | (w2 & 0x00003fff) | (w1 << 28);
    w1 = ((w1 & 0x3fff0000) << 2) | ((w1 & 0x00003fff) << 4);
    (*(uint32_t *)(dst + 0)) = int2be32(w1);
    (*(uint32_t *)(dst + 3)) = int2be32(w2);
  }

  return size;
}

///////////////////////////////////////////////////////////////////////////////
//                               byte <-> 14le
///////////////////////////////////////////////////////////////////////////////


size_t bs_conv_8_14le(const uint8_t *in_buf, size_t size, uint8_t *out_buf)
{
  // We expand the buffer size so output buffer size MUST BE LARGER than
  // input size specified.

  // We can do inplace conversion. You can specify the same input and output
  // buffer pointers.

  // We convert each 7 bytes into 4 16bit words with 14 data bits each.
  // If input frame size is not multiply of 7 we add zeros to the end of the
  // frame to form integral number of 16bit (14bit) words. So output frame
  // size is always even. But in this case exact backward conversion is not
  // possible!

  static const int inc[7] = { 0, 2, 4, 4, 6, 6, 8 };

  size_t n = size / 7;
  size_t r = size % 7;
  const uint8_t *src = in_buf + n * 7;
  uint8_t *dst = out_buf + n * 8;

  size = n * 8 + inc[r];

  if (r)
  {
    // copy frame's tail and zero the rest
    size_t i = 0;
    for (; i < r; i++) dst[i] = src[i];
    for (; i < 7; i++) dst[i] = 0;

    // convert frame's tail
    uint32_t w1 = be2int32(*(uint32_t *)(dst + 0));
    uint32_t w2 = be2int32(*(uint32_t *)(dst + 3));
    w1 = ((w1 >> 2) & 0x3fff0000) | ((w1 >> 4) & 0x00003fff);
    w2 = ((w2 << 2) & 0x3fff0000) | (w2 & 0x00003fff);
    w1 = ((w1 & 0xff00ff00) >> 8) | ((w1 & 0x00ff00ff) << 8);
    w2 = ((w2 & 0xff00ff00) >> 8) | ((w2 & 0x00ff00ff) << 8);
    (*(uint32_t *)(dst + 0)) = int2be32(w1);
    (*(uint32_t *)(dst + 4)) = int2be32(w2);
  }

  src -= 7;
  dst -= 8;
  while (n--)
  {
    uint32_t w1 = be2int32(*(uint32_t *)(src + 0));
    uint32_t w2 = be2int32(*(uint32_t *)(src + 3));
    w1 = ((w1 >> 2) & 0x3fff0000) | ((w1 >> 4) & 0x00003fff);
    w2 = ((w2 << 2) & 0x3fff0000) | (w2 & 0x00003fff);
    w1 = ((w1 & 0xff00ff00) >> 8) | ((w1 & 0x00ff00ff) << 8);
    w2 = ((w2 & 0xff00ff00) >> 8) | ((w2 & 0x00ff00ff) << 8);
    (*(uint32_t *)(dst + 0)) = int2be32(w1);
    (*(uint32_t *)(dst + 4)) = int2be32(w2);
    src -= 7;
    dst -= 8;
  }

  return size;
}

size_t bs_conv_14le_8(const uint8_t *in_buf, size_t size, uint8_t *out_buf)
{
  // Input frame size MUST BE EVEN!!!
  assert((size & 1) == 0);

  // We convert each 4 16bit words with 14 data bits each into 7 bytes.
  // If input frame size is not multiply of 8 we add zeros to the end of the
  // frame to form integral number bytes. But in this case exact backward
  // conversion is not possible!

  size_t n = size / 8;
  size_t r = size % 8;
  const uint8_t *src = in_buf;
  uint8_t *dst = out_buf;

  size = n * 7 + r;

  while (n--)
  {
    uint32_t w1 = be2int32(*(uint32_t *)(src + 0));
    uint32_t w2 = be2int32(*(uint32_t *)(src + 4));
    w2 = ((w2 & 0xff00ff00) >> 8) | ((w2 & 0x00ff00ff) << 8);
    w1 = ((w1 & 0xff00ff00) >> 8) | ((w1 & 0x00ff00ff) << 8);
    w2 = ((w2 & 0x3fff0000) >> 2) | (w2 & 0x00003fff) | (w1 << 28);
    w1 = ((w1 & 0x3fff0000) << 2) | ((w1 & 0x00003fff) << 4);
    (*(uint32_t *)(dst + 0)) = int2be32(w1);
    (*(uint32_t *)(dst + 3)) = int2be32(w2);

    src += 8;
    dst += 7;
  }

  if (r)
  {
    size_t i = 0;
    for (; i < r; i++) dst[i] = src[i];
    for (; i < 8; i++) dst[i] = 0;

    uint32_t w1 = be2int32(*(uint32_t *)(dst + 0));
    uint32_t w2 = be2int32(*(uint32_t *)(dst + 4));
    w2 = ((w2 & 0xff00ff00) >> 8) | ((w2 & 0x00ff00ff) << 8);
    w1 = ((w1 & 0xff00ff00) >> 8) | ((w1 & 0x00ff00ff) << 8);
    w2 = ((w2 & 0x3fff0000) >> 2) | (w2 & 0x00003fff) | (w1 << 28);
    w1 = ((w1 & 0x3fff0000) << 2) | ((w1 & 0x00003fff) << 4);
    (*(uint32_t *)(dst + 0)) = int2be32(w1);
    (*(uint32_t *)(dst + 3)) = int2be32(w2);
  }

  return size;
}

///////////////////////////////////////////////////////////////////////////////
//                              16bit <-> 14bit
///////////////////////////////////////////////////////////////////////////////

size_t bs_conv_16le_14be(const uint8_t *in_buf, size_t size, uint8_t *out_buf)
{
  size = bs_conv_swab16(in_buf, size, out_buf);
  size = bs_conv_8_14be(out_buf, size, out_buf);
  return size; 
}

size_t bs_conv_16le_14le(const uint8_t *in_buf, size_t size, uint8_t *out_buf)
{
  size = bs_conv_swab16(in_buf, size, out_buf);
  size = bs_conv_8_14le(out_buf, size, out_buf);
  return size; 
}

size_t bs_conv_14be_16le(const uint8_t *in_buf, size_t size, uint8_t *out_buf)
{
  size = bs_conv_14be_8(in_buf, size, out_buf);
  size = bs_conv_swab16(out_buf, size, out_buf);
  return size; 
}

size_t bs_conv_14le_16le(const uint8_t *in_buf, size_t size, uint8_t *out_buf)
{
  size = bs_conv_14le_8(in_buf, size, out_buf);
  size = bs_conv_swab16(out_buf, size, out_buf);
  return size; 
}
