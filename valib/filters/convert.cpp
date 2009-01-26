#include <math.h>
#include "convert.h"

// todo: PCM-to-PCM conversions

#if defined(_DEBUG)

static inline int set_rounding() { return 0; }
static inline void restore_rounding(int) {}

#define s2i32(s) ((int32_t)floor((s)+0.5))
#define s2i24(s) ((int24_t)(int32_t)floor((s)+0.5))
#define s2i16(s) ((int16_t)floor((s)+0.5))

#elif defined(_M_IX86)

static inline int set_rounding()
{
  // Set FPU rounding mode to the round to the nearest integer mode
  // Returns unchanged FPU control word to restore later

  uint16_t x87_ctrl;
  __asm fnstcw [x87_ctrl];

  uint16_t new_ctrl = x87_ctrl & 0xf3ff;
  __asm fldcw [new_ctrl];

  return x87_ctrl;
}

static inline void restore_rounding(int r)
{
  // Restores old FPU control word

  uint16_t x87_ctrl = r;
  __asm fldcw [x87_ctrl];
}

inline int32_t s2i32(sample_t s)
{
  register int32_t i;
  __asm fld [s]
  __asm fistp [i]
  return i;
}
inline int24_t s2i24(sample_t s)
{
  register int32_t i;
  __asm fld [s]
  __asm fistp [i]
  return i;
}
inline int16_t s2i16(sample_t s)
{
  register int16_t i;
  __asm fld [s]
  __asm fistp [i]
  return i;
}

#else

static inline int set_rounding() { return 0; }
static inline void restore_rounding(int) {}

inline int32_t s2i32(sample_t s) { return (int32_t)floor(s+0.5); }
inline int32_t s2i24(sample_t s) { return (int24_t)(int32_t)floor(s+0.5); }
inline int16_t s2i16(sample_t s) { return (int16_t)floor(s+0.5); }

#endif


typedef void (Converter::*convert_t)();
static const int formats_tbl[] = { FORMAT_PCM16, FORMAT_PCM24, FORMAT_PCM32, FORMAT_PCM16_BE, FORMAT_PCM24_BE, FORMAT_PCM32_BE, FORMAT_PCMFLOAT };
static const int converter_formats = FORMAT_MASK_LINEAR | FORMAT_MASK_PCM16 | FORMAT_MASK_PCM24 | FORMAT_MASK_PCM32 | FORMAT_MASK_PCM16_BE | FORMAT_MASK_PCM24_BE | FORMAT_MASK_PCM32_BE | FORMAT_MASK_PCMFLOAT;

static const convert_t linear2pcm_tbl[NCHANNELS][7] = {
 { &Converter::linear_pcm16_1ch, &Converter::linear_pcm24_1ch, &Converter::linear_pcm32_1ch, &Converter::linear_pcm16_be_1ch, &Converter::linear_pcm24_be_1ch, &Converter::linear_pcm32_be_1ch, &Converter::linear_pcmfloat_1ch },
 { &Converter::linear_pcm16_2ch, &Converter::linear_pcm24_2ch, &Converter::linear_pcm32_2ch, &Converter::linear_pcm16_be_2ch, &Converter::linear_pcm24_be_2ch, &Converter::linear_pcm32_be_2ch, &Converter::linear_pcmfloat_2ch },
 { &Converter::linear_pcm16_3ch, &Converter::linear_pcm24_3ch, &Converter::linear_pcm32_3ch, &Converter::linear_pcm16_be_3ch, &Converter::linear_pcm24_be_3ch, &Converter::linear_pcm32_be_3ch, &Converter::linear_pcmfloat_3ch },
 { &Converter::linear_pcm16_4ch, &Converter::linear_pcm24_4ch, &Converter::linear_pcm32_4ch, &Converter::linear_pcm16_be_4ch, &Converter::linear_pcm24_be_4ch, &Converter::linear_pcm32_be_4ch, &Converter::linear_pcmfloat_4ch },
 { &Converter::linear_pcm16_5ch, &Converter::linear_pcm24_5ch, &Converter::linear_pcm32_5ch, &Converter::linear_pcm16_be_5ch, &Converter::linear_pcm24_be_5ch, &Converter::linear_pcm32_be_5ch, &Converter::linear_pcmfloat_5ch },
 { &Converter::linear_pcm16_6ch, &Converter::linear_pcm24_6ch, &Converter::linear_pcm32_6ch, &Converter::linear_pcm16_be_6ch, &Converter::linear_pcm24_be_6ch, &Converter::linear_pcm32_be_6ch, &Converter::linear_pcmfloat_6ch },
};

static const convert_t pcm2linear_tbl[NCHANNELS][7] = {
 { &Converter::pcm16_linear_1ch, &Converter::pcm24_linear_1ch, &Converter::pcm32_linear_1ch, &Converter::pcm16_be_linear_1ch, &Converter::pcm24_be_linear_1ch, &Converter::pcm32_be_linear_1ch, &Converter::pcmfloat_linear_1ch },
 { &Converter::pcm16_linear_2ch, &Converter::pcm24_linear_2ch, &Converter::pcm32_linear_2ch, &Converter::pcm16_be_linear_2ch, &Converter::pcm24_be_linear_2ch, &Converter::pcm32_be_linear_2ch, &Converter::pcmfloat_linear_2ch },
 { &Converter::pcm16_linear_3ch, &Converter::pcm24_linear_3ch, &Converter::pcm32_linear_3ch, &Converter::pcm16_be_linear_3ch, &Converter::pcm24_be_linear_3ch, &Converter::pcm32_be_linear_3ch, &Converter::pcmfloat_linear_3ch },
 { &Converter::pcm16_linear_4ch, &Converter::pcm24_linear_4ch, &Converter::pcm32_linear_4ch, &Converter::pcm16_be_linear_4ch, &Converter::pcm24_be_linear_4ch, &Converter::pcm32_be_linear_4ch, &Converter::pcmfloat_linear_4ch },
 { &Converter::pcm16_linear_5ch, &Converter::pcm24_linear_5ch, &Converter::pcm32_linear_5ch, &Converter::pcm16_be_linear_5ch, &Converter::pcm24_be_linear_5ch, &Converter::pcm32_be_linear_5ch, &Converter::pcmfloat_linear_5ch },
 { &Converter::pcm16_linear_6ch, &Converter::pcm24_linear_6ch, &Converter::pcm32_linear_6ch, &Converter::pcm16_be_linear_6ch, &Converter::pcm24_be_linear_6ch, &Converter::pcm32_be_linear_6ch, &Converter::pcmfloat_linear_6ch },
};



Converter::Converter(size_t _nsamples)
:NullFilter(0) // use own query_input()
{
  convert = 0;
  format = FORMAT_UNKNOWN;
  memcpy(order, std_order, sizeof(order));
  nsamples = _nsamples;
  out_size = 0;
  part_size = 0;
}

convert_t 
Converter::find_conversion(int _format, Speakers _spk) const
{
  if (_spk.format == _format)
    // no conversion required but we have to return conversion 
    // function to indicate that we can proceed
    return &Converter::passthrough;

  if (_format == FORMAT_LINEAR)
  {
    // find pcm->linear conversion function
    for (int i = 0; i < array_size(formats_tbl); i++)
      if (_spk.format == formats_tbl[i])
        return pcm2linear_tbl[_spk.nch()-1][i];
  }
  else if (_spk.format == FORMAT_LINEAR)
  {
    // find linear->pcm conversion function
    for (int i = 0; i < array_size(formats_tbl); i++)
      if (_format == formats_tbl[i])
        return linear2pcm_tbl[_spk.nch()-1][i];
  }

  return 0;
}

bool 
Converter::initialize()
{
  /////////////////////////////////////////////////////////
  // Initialize convertor:
  // * reset filter state
  // * find conversion function
  // * allocate buffer
  //
  // If we cannot find conversion we have to drop current 
  // input format to spk_unknown to indicate that we cannot
  // proceed with current setup so forcing application to 
  // call set_input() with new input format.

  /////////////////////////////////////////////////////////
  // reset filter state

  reset();

  /////////////////////////////////////////////////////////
  // check if no conversion required

  if (spk.is_unknown())
  {
    // input format is not set
    convert = &Converter::passthrough;
    return true; 
  }

  if (spk.format == format)
  {
    // no conversion required
    // no buffer required
    convert = &Converter::passthrough;
    return true; 
  }
  
  /////////////////////////////////////////////////////////
  // find conversion function

  convert = find_conversion(format, spk);
  if (convert == 0)
  {
    spk = spk_unknown;
    return false;
  }

  /////////////////////////////////////////////////////////
  // allocate buffer

  if (!buf.allocate(spk.nch() * nsamples * sample_size(format)))
  {
    convert = 0;
    spk = spk_unknown;
    return false;
  }

  if (format == FORMAT_LINEAR)
  {
    // set channel pointers
    out_samples[0] = (sample_t *)buf.data();
    for (int ch = 1; ch < spk.nch(); ch++)
      out_samples[ch] = out_samples[ch-1] + nsamples;
    out_samples.reorder_to_std(spk, order);
    out_rawdata = 0;
  }
  else
  {
    // set rawdata pointer
    out_rawdata = buf.data();
    out_samples.zero();
  }

  return true;
}

///////////////////////////////////////////////////////////
// Converter interface

size_t 
Converter::get_buffer() const
{
  return nsamples;
}

bool 
Converter::set_buffer(size_t _nsamples)
{
  nsamples = _nsamples;
  return initialize();
}

int 
Converter::get_format() const
{
  return format;
}

bool 
Converter::set_format(int _format)
{
  if ((FORMAT_MASK(_format) & converter_formats) == 0)
    return false;

  format = _format;
  return initialize();
}

void 
Converter::get_order(int _order[NCHANNELS]) const
{
  memcpy(_order, order, sizeof(order));
}

void 
Converter::set_order(const int _order[NCHANNELS])
{
  if (format == FORMAT_LINEAR)
    out_samples.reorder(spk, order, _order);
  memcpy(order, _order, sizeof(order));
}

///////////////////////////////////////////////////////////
// Filter interface

void
Converter::reset()
{
  NullFilter::reset();
  part_size = 0;
}

bool 
Converter::query_input(Speakers _spk) const
{
  if ((FORMAT_MASK(_spk.format) & converter_formats) == 0)
    return false;

  if (_spk.nch() == 0)
    return false;

  if (find_conversion(format, _spk) == 0)
    return false;

  return true;
}

bool 
Converter::set_input(Speakers _spk)
{
  if (!NullFilter::set_input(_spk))
    return false;

  return initialize();
}

Speakers 
Converter::get_output() const
{
  if (convert == 0)
    return spk_unknown;

  Speakers out = spk;
  out.format = format;
  return out;
}

bool 
Converter::process(const Chunk *_chunk)
{
  // we must ignore dummy chunks
  if (_chunk->is_dummy())
    return true;

  FILTER_SAFE(receive_chunk(_chunk));

  if (spk.format == FORMAT_LINEAR)
    samples.reorder_from_std(spk, order);

  return true;
}

bool 
Converter::get_chunk(Chunk *_chunk)
{
  if (spk.format == format)
  {
    send_chunk_inplace(_chunk, size);
    return true;
  }

  if (convert == 0)
    return false;

  int r = set_rounding();
  (this->*convert)();
  restore_rounding(r);

  _chunk->set
  (
    get_output(), 
    out_rawdata, out_samples, out_size, 
    sync, time, 
    flushing && !size
  );

  flushing = flushing && size;
  sync = false;
  time = 0;
  return true;
}

///////////////////////////////////////////////////////////
// Conversion functions

void
Converter::passthrough()
{
  // do nothing
}

void
Converter::linear_pcm16_1ch()
{
  const int nch = 1;
  const size_t sample_size = sizeof(int16_t) * 1;

  samples_t src = samples;
  int16_t *dst = (int16_t *)out_rawdata;
  size_t n = MIN(size, nsamples);
  drop_samples(n);
  out_size = n * sample_size;

  while (n--)
  {
    dst[0] = int2le16(s2i16(src[0][0])); src[0]++;

    dst += nch;
  }
}
void
Converter::linear_pcm24_1ch()
{
  const int nch = 1;
  const size_t sample_size = sizeof(int24_t) * 1;

  samples_t src = samples;
  int24_t *dst = (int24_t *)out_rawdata;
  size_t n = MIN(size, nsamples);
  drop_samples(n);
  out_size = n * sample_size;

  while (n--)
  {
    dst[0] = int2le24(s2i24(src[0][0])); src[0]++;

    dst += nch;
  }
}
void
Converter::linear_pcm32_1ch()
{
  const int nch = 1;
  const size_t sample_size = sizeof(int32_t) * 1;

  samples_t src = samples;
  int32_t *dst = (int32_t *)out_rawdata;
  size_t n = MIN(size, nsamples);
  drop_samples(n);
  out_size = n * sample_size;

  while (n--)
  {
    dst[0] = int2le32(s2i32(src[0][0])); src[0]++;

    dst += nch;
  }
}
void
Converter::linear_pcm16_be_1ch()
{
  const int nch = 1;
  const size_t sample_size = sizeof(int16_t) * 1;

  samples_t src = samples;
  int16_t *dst = (int16_t *)out_rawdata;
  size_t n = MIN(size, nsamples);
  drop_samples(n);
  out_size = n * sample_size;

  while (n--)
  {
    dst[0] = int2be16(s2i16(src[0][0])); src[0]++;

    dst += nch;
  }
}
void
Converter::linear_pcm24_be_1ch()
{
  const int nch = 1;
  const size_t sample_size = sizeof(int24_t) * 1;

  samples_t src = samples;
  int24_t *dst = (int24_t *)out_rawdata;
  size_t n = MIN(size, nsamples);
  drop_samples(n);
  out_size = n * sample_size;

  while (n--)
  {
    dst[0] = int2be24(s2i24(src[0][0])); src[0]++;

    dst += nch;
  }
}
void
Converter::linear_pcm32_be_1ch()
{
  const int nch = 1;
  const size_t sample_size = sizeof(int32_t) * 1;

  samples_t src = samples;
  int32_t *dst = (int32_t *)out_rawdata;
  size_t n = MIN(size, nsamples);
  drop_samples(n);
  out_size = n * sample_size;

  while (n--)
  {
    dst[0] = int2be32(s2i32(src[0][0])); src[0]++;

    dst += nch;
  }
}
void
Converter::linear_pcmfloat_1ch()
{
  const int nch = 1;
  const size_t sample_size = sizeof(float) * 1;

  samples_t src = samples;
  float *dst = (float *)out_rawdata;
  size_t n = MIN(size, nsamples);
  drop_samples(n);
  out_size = n * sample_size;

  while (n--)
  {
    dst[0] = ((float)(src[0][0])); src[0]++;

    dst += nch;
  }
}

void
Converter::linear_pcm16_2ch()
{
  const int nch = 2;
  const size_t sample_size = sizeof(int16_t) * 2;

  samples_t src = samples;
  int16_t *dst = (int16_t *)out_rawdata;
  size_t n = MIN(size, nsamples);
  drop_samples(n);
  out_size = n * sample_size;

  while (n--)
  {
    dst[0] = int2le16(s2i16(src[0][0])); src[0]++;
    dst[1] = int2le16(s2i16(src[1][0])); src[1]++;

    dst += nch;
  }
}
void
Converter::linear_pcm24_2ch()
{
  const int nch = 2;
  const size_t sample_size = sizeof(int24_t) * 2;

  samples_t src = samples;
  int24_t *dst = (int24_t *)out_rawdata;
  size_t n = MIN(size, nsamples);
  drop_samples(n);
  out_size = n * sample_size;

  while (n--)
  {
    dst[0] = int2le24(s2i24(src[0][0])); src[0]++;
    dst[1] = int2le24(s2i24(src[1][0])); src[1]++;

    dst += nch;
  }
}
void
Converter::linear_pcm32_2ch()
{
  const int nch = 2;
  const size_t sample_size = sizeof(int32_t) * 2;

  samples_t src = samples;
  int32_t *dst = (int32_t *)out_rawdata;
  size_t n = MIN(size, nsamples);
  drop_samples(n);
  out_size = n * sample_size;

  while (n--)
  {
    dst[0] = int2le32(s2i32(src[0][0])); src[0]++;
    dst[1] = int2le32(s2i32(src[1][0])); src[1]++;

    dst += nch;
  }
}
void
Converter::linear_pcm16_be_2ch()
{
  const int nch = 2;
  const size_t sample_size = sizeof(int16_t) * 2;

  samples_t src = samples;
  int16_t *dst = (int16_t *)out_rawdata;
  size_t n = MIN(size, nsamples);
  drop_samples(n);
  out_size = n * sample_size;

  while (n--)
  {
    dst[0] = int2be16(s2i16(src[0][0])); src[0]++;
    dst[1] = int2be16(s2i16(src[1][0])); src[1]++;

    dst += nch;
  }
}
void
Converter::linear_pcm24_be_2ch()
{
  const int nch = 2;
  const size_t sample_size = sizeof(int24_t) * 2;

  samples_t src = samples;
  int24_t *dst = (int24_t *)out_rawdata;
  size_t n = MIN(size, nsamples);
  drop_samples(n);
  out_size = n * sample_size;

  while (n--)
  {
    dst[0] = int2be24(s2i24(src[0][0])); src[0]++;
    dst[1] = int2be24(s2i24(src[1][0])); src[1]++;

    dst += nch;
  }
}
void
Converter::linear_pcm32_be_2ch()
{
  const int nch = 2;
  const size_t sample_size = sizeof(int32_t) * 2;

  samples_t src = samples;
  int32_t *dst = (int32_t *)out_rawdata;
  size_t n = MIN(size, nsamples);
  drop_samples(n);
  out_size = n * sample_size;

  while (n--)
  {
    dst[0] = int2be32(s2i32(src[0][0])); src[0]++;
    dst[1] = int2be32(s2i32(src[1][0])); src[1]++;

    dst += nch;
  }
}
void
Converter::linear_pcmfloat_2ch()
{
  const int nch = 2;
  const size_t sample_size = sizeof(float) * 2;

  samples_t src = samples;
  float *dst = (float *)out_rawdata;
  size_t n = MIN(size, nsamples);
  drop_samples(n);
  out_size = n * sample_size;

  while (n--)
  {
    dst[0] = ((float)(src[0][0])); src[0]++;
    dst[1] = ((float)(src[1][0])); src[1]++;

    dst += nch;
  }
}

void
Converter::linear_pcm16_3ch()
{
  const int nch = 3;
  const size_t sample_size = sizeof(int16_t) * 3;

  samples_t src = samples;
  int16_t *dst = (int16_t *)out_rawdata;
  size_t n = MIN(size, nsamples);
  drop_samples(n);
  out_size = n * sample_size;

  while (n--)
  {
    dst[0] = int2le16(s2i16(src[0][0])); src[0]++;
    dst[1] = int2le16(s2i16(src[1][0])); src[1]++;
    dst[2] = int2le16(s2i16(src[2][0])); src[2]++;

    dst += nch;
  }
}
void
Converter::linear_pcm24_3ch()
{
  const int nch = 3;
  const size_t sample_size = sizeof(int24_t) * 3;

  samples_t src = samples;
  int24_t *dst = (int24_t *)out_rawdata;
  size_t n = MIN(size, nsamples);
  drop_samples(n);
  out_size = n * sample_size;

  while (n--)
  {
    dst[0] = int2le24(s2i24(src[0][0])); src[0]++;
    dst[1] = int2le24(s2i24(src[1][0])); src[1]++;
    dst[2] = int2le24(s2i24(src[2][0])); src[2]++;

    dst += nch;
  }
}
void
Converter::linear_pcm32_3ch()
{
  const int nch = 3;
  const size_t sample_size = sizeof(int32_t) * 3;

  samples_t src = samples;
  int32_t *dst = (int32_t *)out_rawdata;
  size_t n = MIN(size, nsamples);
  drop_samples(n);
  out_size = n * sample_size;

  while (n--)
  {
    dst[0] = int2le32(s2i32(src[0][0])); src[0]++;
    dst[1] = int2le32(s2i32(src[1][0])); src[1]++;
    dst[2] = int2le32(s2i32(src[2][0])); src[2]++;

    dst += nch;
  }
}
void
Converter::linear_pcm16_be_3ch()
{
  const int nch = 3;
  const size_t sample_size = sizeof(int16_t) * 3;

  samples_t src = samples;
  int16_t *dst = (int16_t *)out_rawdata;
  size_t n = MIN(size, nsamples);
  drop_samples(n);
  out_size = n * sample_size;

  while (n--)
  {
    dst[0] = int2be16(s2i16(src[0][0])); src[0]++;
    dst[1] = int2be16(s2i16(src[1][0])); src[1]++;
    dst[2] = int2be16(s2i16(src[2][0])); src[2]++;

    dst += nch;
  }
}
void
Converter::linear_pcm24_be_3ch()
{
  const int nch = 3;
  const size_t sample_size = sizeof(int24_t) * 3;

  samples_t src = samples;
  int24_t *dst = (int24_t *)out_rawdata;
  size_t n = MIN(size, nsamples);
  drop_samples(n);
  out_size = n * sample_size;

  while (n--)
  {
    dst[0] = int2be24(s2i24(src[0][0])); src[0]++;
    dst[1] = int2be24(s2i24(src[1][0])); src[1]++;
    dst[2] = int2be24(s2i24(src[2][0])); src[2]++;

    dst += nch;
  }
}
void
Converter::linear_pcm32_be_3ch()
{
  const int nch = 3;
  const size_t sample_size = sizeof(int32_t) * 3;

  samples_t src = samples;
  int32_t *dst = (int32_t *)out_rawdata;
  size_t n = MIN(size, nsamples);
  drop_samples(n);
  out_size = n * sample_size;

  while (n--)
  {
    dst[0] = int2be32(s2i32(src[0][0])); src[0]++;
    dst[1] = int2be32(s2i32(src[1][0])); src[1]++;
    dst[2] = int2be32(s2i32(src[2][0])); src[2]++;

    dst += nch;
  }
}
void
Converter::linear_pcmfloat_3ch()
{
  const int nch = 3;
  const size_t sample_size = sizeof(float) * 3;

  samples_t src = samples;
  float *dst = (float *)out_rawdata;
  size_t n = MIN(size, nsamples);
  drop_samples(n);
  out_size = n * sample_size;

  while (n--)
  {
    dst[0] = ((float)(src[0][0])); src[0]++;
    dst[1] = ((float)(src[1][0])); src[1]++;
    dst[2] = ((float)(src[2][0])); src[2]++;

    dst += nch;
  }
}

void
Converter::linear_pcm16_4ch()
{
  const int nch = 4;
  const size_t sample_size = sizeof(int16_t) * 4;

  samples_t src = samples;
  int16_t *dst = (int16_t *)out_rawdata;
  size_t n = MIN(size, nsamples);
  drop_samples(n);
  out_size = n * sample_size;

  while (n--)
  {
    dst[0] = int2le16(s2i16(src[0][0])); src[0]++;
    dst[1] = int2le16(s2i16(src[1][0])); src[1]++;
    dst[2] = int2le16(s2i16(src[2][0])); src[2]++;
    dst[3] = int2le16(s2i16(src[3][0])); src[3]++;

    dst += nch;
  }
}
void
Converter::linear_pcm24_4ch()
{
  const int nch = 4;
  const size_t sample_size = sizeof(int24_t) * 4;

  samples_t src = samples;
  int24_t *dst = (int24_t *)out_rawdata;
  size_t n = MIN(size, nsamples);
  drop_samples(n);
  out_size = n * sample_size;

  while (n--)
  {
    dst[0] = int2le24(s2i24(src[0][0])); src[0]++;
    dst[1] = int2le24(s2i24(src[1][0])); src[1]++;
    dst[2] = int2le24(s2i24(src[2][0])); src[2]++;
    dst[3] = int2le24(s2i24(src[3][0])); src[3]++;

    dst += nch;
  }
}
void
Converter::linear_pcm32_4ch()
{
  const int nch = 4;
  const size_t sample_size = sizeof(int32_t) * 4;

  samples_t src = samples;
  int32_t *dst = (int32_t *)out_rawdata;
  size_t n = MIN(size, nsamples);
  drop_samples(n);
  out_size = n * sample_size;

  while (n--)
  {
    dst[0] = int2le32(s2i32(src[0][0])); src[0]++;
    dst[1] = int2le32(s2i32(src[1][0])); src[1]++;
    dst[2] = int2le32(s2i32(src[2][0])); src[2]++;
    dst[3] = int2le32(s2i32(src[3][0])); src[3]++;

    dst += nch;
  }
}
void
Converter::linear_pcm16_be_4ch()
{
  const int nch = 4;
  const size_t sample_size = sizeof(int16_t) * 4;

  samples_t src = samples;
  int16_t *dst = (int16_t *)out_rawdata;
  size_t n = MIN(size, nsamples);
  drop_samples(n);
  out_size = n * sample_size;

  while (n--)
  {
    dst[0] = int2be16(s2i16(src[0][0])); src[0]++;
    dst[1] = int2be16(s2i16(src[1][0])); src[1]++;
    dst[2] = int2be16(s2i16(src[2][0])); src[2]++;
    dst[3] = int2be16(s2i16(src[3][0])); src[3]++;

    dst += nch;
  }
}
void
Converter::linear_pcm24_be_4ch()
{
  const int nch = 4;
  const size_t sample_size = sizeof(int24_t) * 4;

  samples_t src = samples;
  int24_t *dst = (int24_t *)out_rawdata;
  size_t n = MIN(size, nsamples);
  drop_samples(n);
  out_size = n * sample_size;

  while (n--)
  {
    dst[0] = int2be24(s2i24(src[0][0])); src[0]++;
    dst[1] = int2be24(s2i24(src[1][0])); src[1]++;
    dst[2] = int2be24(s2i24(src[2][0])); src[2]++;
    dst[3] = int2be24(s2i24(src[3][0])); src[3]++;

    dst += nch;
  }
}
void
Converter::linear_pcm32_be_4ch()
{
  const int nch = 4;
  const size_t sample_size = sizeof(int32_t) * 4;

  samples_t src = samples;
  int32_t *dst = (int32_t *)out_rawdata;
  size_t n = MIN(size, nsamples);
  drop_samples(n);
  out_size = n * sample_size;

  while (n--)
  {
    dst[0] = int2be32(s2i32(src[0][0])); src[0]++;
    dst[1] = int2be32(s2i32(src[1][0])); src[1]++;
    dst[2] = int2be32(s2i32(src[2][0])); src[2]++;
    dst[3] = int2be32(s2i32(src[3][0])); src[3]++;

    dst += nch;
  }
}
void
Converter::linear_pcmfloat_4ch()
{
  const int nch = 4;
  const size_t sample_size = sizeof(float) * 4;

  samples_t src = samples;
  float *dst = (float *)out_rawdata;
  size_t n = MIN(size, nsamples);
  drop_samples(n);
  out_size = n * sample_size;

  while (n--)
  {
    dst[0] = ((float)(src[0][0])); src[0]++;
    dst[1] = ((float)(src[1][0])); src[1]++;
    dst[2] = ((float)(src[2][0])); src[2]++;
    dst[3] = ((float)(src[3][0])); src[3]++;

    dst += nch;
  }
}

void
Converter::linear_pcm16_5ch()
{
  const int nch = 5;
  const size_t sample_size = sizeof(int16_t) * 5;

  samples_t src = samples;
  int16_t *dst = (int16_t *)out_rawdata;
  size_t n = MIN(size, nsamples);
  drop_samples(n);
  out_size = n * sample_size;

  while (n--)
  {
    dst[0] = int2le16(s2i16(src[0][0])); src[0]++;
    dst[1] = int2le16(s2i16(src[1][0])); src[1]++;
    dst[2] = int2le16(s2i16(src[2][0])); src[2]++;
    dst[3] = int2le16(s2i16(src[3][0])); src[3]++;
    dst[4] = int2le16(s2i16(src[4][0])); src[4]++;

    dst += nch;
  }
}
void
Converter::linear_pcm24_5ch()
{
  const int nch = 5;
  const size_t sample_size = sizeof(int24_t) * 5;

  samples_t src = samples;
  int24_t *dst = (int24_t *)out_rawdata;
  size_t n = MIN(size, nsamples);
  drop_samples(n);
  out_size = n * sample_size;

  while (n--)
  {
    dst[0] = int2le24(s2i24(src[0][0])); src[0]++;
    dst[1] = int2le24(s2i24(src[1][0])); src[1]++;
    dst[2] = int2le24(s2i24(src[2][0])); src[2]++;
    dst[3] = int2le24(s2i24(src[3][0])); src[3]++;
    dst[4] = int2le24(s2i24(src[4][0])); src[4]++;

    dst += nch;
  }
}
void
Converter::linear_pcm32_5ch()
{
  const int nch = 5;
  const size_t sample_size = sizeof(int32_t) * 5;

  samples_t src = samples;
  int32_t *dst = (int32_t *)out_rawdata;
  size_t n = MIN(size, nsamples);
  drop_samples(n);
  out_size = n * sample_size;

  while (n--)
  {
    dst[0] = int2le32(s2i32(src[0][0])); src[0]++;
    dst[1] = int2le32(s2i32(src[1][0])); src[1]++;
    dst[2] = int2le32(s2i32(src[2][0])); src[2]++;
    dst[3] = int2le32(s2i32(src[3][0])); src[3]++;
    dst[4] = int2le32(s2i32(src[4][0])); src[4]++;

    dst += nch;
  }
}
void
Converter::linear_pcm16_be_5ch()
{
  const int nch = 5;
  const size_t sample_size = sizeof(int16_t) * 5;

  samples_t src = samples;
  int16_t *dst = (int16_t *)out_rawdata;
  size_t n = MIN(size, nsamples);
  drop_samples(n);
  out_size = n * sample_size;

  while (n--)
  {
    dst[0] = int2be16(s2i16(src[0][0])); src[0]++;
    dst[1] = int2be16(s2i16(src[1][0])); src[1]++;
    dst[2] = int2be16(s2i16(src[2][0])); src[2]++;
    dst[3] = int2be16(s2i16(src[3][0])); src[3]++;
    dst[4] = int2be16(s2i16(src[4][0])); src[4]++;

    dst += nch;
  }
}
void
Converter::linear_pcm24_be_5ch()
{
  const int nch = 5;
  const size_t sample_size = sizeof(int24_t) * 5;

  samples_t src = samples;
  int24_t *dst = (int24_t *)out_rawdata;
  size_t n = MIN(size, nsamples);
  drop_samples(n);
  out_size = n * sample_size;

  while (n--)
  {
    dst[0] = int2be24(s2i24(src[0][0])); src[0]++;
    dst[1] = int2be24(s2i24(src[1][0])); src[1]++;
    dst[2] = int2be24(s2i24(src[2][0])); src[2]++;
    dst[3] = int2be24(s2i24(src[3][0])); src[3]++;
    dst[4] = int2be24(s2i24(src[4][0])); src[4]++;

    dst += nch;
  }
}
void
Converter::linear_pcm32_be_5ch()
{
  const int nch = 5;
  const size_t sample_size = sizeof(int32_t) * 5;

  samples_t src = samples;
  int32_t *dst = (int32_t *)out_rawdata;
  size_t n = MIN(size, nsamples);
  drop_samples(n);
  out_size = n * sample_size;

  while (n--)
  {
    dst[0] = int2be32(s2i32(src[0][0])); src[0]++;
    dst[1] = int2be32(s2i32(src[1][0])); src[1]++;
    dst[2] = int2be32(s2i32(src[2][0])); src[2]++;
    dst[3] = int2be32(s2i32(src[3][0])); src[3]++;
    dst[4] = int2be32(s2i32(src[4][0])); src[4]++;

    dst += nch;
  }
}
void
Converter::linear_pcmfloat_5ch()
{
  const int nch = 5;
  const size_t sample_size = sizeof(float) * 5;

  samples_t src = samples;
  float *dst = (float *)out_rawdata;
  size_t n = MIN(size, nsamples);
  drop_samples(n);
  out_size = n * sample_size;

  while (n--)
  {
    dst[0] = ((float)(src[0][0])); src[0]++;
    dst[1] = ((float)(src[1][0])); src[1]++;
    dst[2] = ((float)(src[2][0])); src[2]++;
    dst[3] = ((float)(src[3][0])); src[3]++;
    dst[4] = ((float)(src[4][0])); src[4]++;

    dst += nch;
  }
}

void
Converter::linear_pcm16_6ch()
{
  const int nch = 6;
  const size_t sample_size = sizeof(int16_t) * 6;

  samples_t src = samples;
  int16_t *dst = (int16_t *)out_rawdata;
  size_t n = MIN(size, nsamples);
  drop_samples(n);
  out_size = n * sample_size;

  while (n--)
  {
    dst[0] = int2le16(s2i16(src[0][0])); src[0]++;
    dst[1] = int2le16(s2i16(src[1][0])); src[1]++;
    dst[2] = int2le16(s2i16(src[2][0])); src[2]++;
    dst[3] = int2le16(s2i16(src[3][0])); src[3]++;
    dst[4] = int2le16(s2i16(src[4][0])); src[4]++;
    dst[5] = int2le16(s2i16(src[5][0])); src[5]++;

    dst += nch;
  }
}
void
Converter::linear_pcm24_6ch()
{
  const int nch = 6;
  const size_t sample_size = sizeof(int24_t) * 6;

  samples_t src = samples;
  int24_t *dst = (int24_t *)out_rawdata;
  size_t n = MIN(size, nsamples);
  drop_samples(n);
  out_size = n * sample_size;

  while (n--)
  {
    dst[0] = int2le24(s2i24(src[0][0])); src[0]++;
    dst[1] = int2le24(s2i24(src[1][0])); src[1]++;
    dst[2] = int2le24(s2i24(src[2][0])); src[2]++;
    dst[3] = int2le24(s2i24(src[3][0])); src[3]++;
    dst[4] = int2le24(s2i24(src[4][0])); src[4]++;
    dst[5] = int2le24(s2i24(src[5][0])); src[5]++;

    dst += nch;
  }
}
void
Converter::linear_pcm32_6ch()
{
  const int nch = 6;
  const size_t sample_size = sizeof(int32_t) * 6;

  samples_t src = samples;
  int32_t *dst = (int32_t *)out_rawdata;
  size_t n = MIN(size, nsamples);
  drop_samples(n);
  out_size = n * sample_size;

  while (n--)
  {
    dst[0] = int2le32(s2i32(src[0][0])); src[0]++;
    dst[1] = int2le32(s2i32(src[1][0])); src[1]++;
    dst[2] = int2le32(s2i32(src[2][0])); src[2]++;
    dst[3] = int2le32(s2i32(src[3][0])); src[3]++;
    dst[4] = int2le32(s2i32(src[4][0])); src[4]++;
    dst[5] = int2le32(s2i32(src[5][0])); src[5]++;

    dst += nch;
  }
}
void
Converter::linear_pcm16_be_6ch()
{
  const int nch = 6;
  const size_t sample_size = sizeof(int16_t) * 6;

  samples_t src = samples;
  int16_t *dst = (int16_t *)out_rawdata;
  size_t n = MIN(size, nsamples);
  drop_samples(n);
  out_size = n * sample_size;

  while (n--)
  {
    dst[0] = int2be16(s2i16(src[0][0])); src[0]++;
    dst[1] = int2be16(s2i16(src[1][0])); src[1]++;
    dst[2] = int2be16(s2i16(src[2][0])); src[2]++;
    dst[3] = int2be16(s2i16(src[3][0])); src[3]++;
    dst[4] = int2be16(s2i16(src[4][0])); src[4]++;
    dst[5] = int2be16(s2i16(src[5][0])); src[5]++;

    dst += nch;
  }
}
void
Converter::linear_pcm24_be_6ch()
{
  const int nch = 6;
  const size_t sample_size = sizeof(int24_t) * 6;

  samples_t src = samples;
  int24_t *dst = (int24_t *)out_rawdata;
  size_t n = MIN(size, nsamples);
  drop_samples(n);
  out_size = n * sample_size;

  while (n--)
  {
    dst[0] = int2be24(s2i24(src[0][0])); src[0]++;
    dst[1] = int2be24(s2i24(src[1][0])); src[1]++;
    dst[2] = int2be24(s2i24(src[2][0])); src[2]++;
    dst[3] = int2be24(s2i24(src[3][0])); src[3]++;
    dst[4] = int2be24(s2i24(src[4][0])); src[4]++;
    dst[5] = int2be24(s2i24(src[5][0])); src[5]++;

    dst += nch;
  }
}
void
Converter::linear_pcm32_be_6ch()
{
  const int nch = 6;
  const size_t sample_size = sizeof(int32_t) * 6;

  samples_t src = samples;
  int32_t *dst = (int32_t *)out_rawdata;
  size_t n = MIN(size, nsamples);
  drop_samples(n);
  out_size = n * sample_size;

  while (n--)
  {
    dst[0] = int2be32(s2i32(src[0][0])); src[0]++;
    dst[1] = int2be32(s2i32(src[1][0])); src[1]++;
    dst[2] = int2be32(s2i32(src[2][0])); src[2]++;
    dst[3] = int2be32(s2i32(src[3][0])); src[3]++;
    dst[4] = int2be32(s2i32(src[4][0])); src[4]++;
    dst[5] = int2be32(s2i32(src[5][0])); src[5]++;

    dst += nch;
  }
}
void
Converter::linear_pcmfloat_6ch()
{
  const int nch = 6;
  const size_t sample_size = sizeof(float) * 6;

  samples_t src = samples;
  float *dst = (float *)out_rawdata;
  size_t n = MIN(size, nsamples);
  drop_samples(n);
  out_size = n * sample_size;

  while (n--)
  {
    dst[0] = ((float)(src[0][0])); src[0]++;
    dst[1] = ((float)(src[1][0])); src[1]++;
    dst[2] = ((float)(src[2][0])); src[2]++;
    dst[3] = ((float)(src[3][0])); src[3]++;
    dst[4] = ((float)(src[4][0])); src[4]++;
    dst[5] = ((float)(src[5][0])); src[5]++;

    dst += nch;
  }
}

void
Converter::pcm16_linear_1ch()
{
  // input: spk, nsamples, rawdata, size
  // output: out_samples, out_size
  // intermediate: part_buf, part_size

  const int nch = 1;
  const size_t sample_size = sizeof(int16_t) * 1;

  int16_t *src;
  int16_t *end;
  sample_t *dst = out_samples[0];
  out_size = 0;

  /////////////////////////////////////////////////////////
  // Process part of sample

  if (part_size)
  {
    // assert: part_size < sample_size
    size_t delta = sample_size - part_size;
    if (size < delta)
    {
      // not enough data to fill sample buffer
      memcpy(part_buf + part_size, rawdata, size);
      part_size += size;
      size = 0;
      return;
    }
    else
    {
      // finish & convert incomplete sample 
      memcpy(part_buf + part_size, rawdata, delta);
      drop_rawdata(delta);
      part_size = 0;

      src = (int16_t *)part_buf;
    dst[nsamples * 0] = int2le16(src[0]);

      dst++;
      out_size++;
    }   
  }

  /////////////////////////////////////////////////////////
  // Set processing buffer start & end and remember 
  // remaining part of sample

  src = (int16_t *)rawdata;

  // integral number of samples (and its size in bytes)
  size_t n = nsamples - out_size;
  size_t n_size = n * sample_size;

  if (n_size < size)
  {
    drop_rawdata(n_size);
    end = (int16_t *)(((uint8_t *)src) + n_size);
    out_size += n;
  }
  else
  {
    n = size / sample_size;
    n_size = n * sample_size;

    drop_rawdata(n_size);
    end = (int16_t *)(((uint8_t *)src) + n_size);
    out_size += n;

    // remember part of sample
    if (size)
    {
      memcpy(part_buf, end, size);
      part_size = size;
      size = 0;
    }
  }

  /////////////////////////////////////////////////////////
  // Convert

  while (src < end)
  {
    dst[nsamples * 0] = int2le16(src[0]);

    src += nch;
    dst++;
  }
}
void
Converter::pcm24_linear_1ch()
{
  // input: spk, nsamples, rawdata, size
  // output: out_samples, out_size
  // intermediate: part_buf, part_size

  const int nch = 1;
  const size_t sample_size = sizeof(int24_t) * 1;

  int24_t *src;
  int24_t *end;
  sample_t *dst = out_samples[0];
  out_size = 0;

  /////////////////////////////////////////////////////////
  // Process part of sample

  if (part_size)
  {
    // assert: part_size < sample_size
    size_t delta = sample_size - part_size;
    if (size < delta)
    {
      // not enough data to fill sample buffer
      memcpy(part_buf + part_size, rawdata, size);
      part_size += size;
      size = 0;
      return;
    }
    else
    {
      // finish & convert incomplete sample 
      memcpy(part_buf + part_size, rawdata, delta);
      drop_rawdata(delta);
      part_size = 0;

      src = (int24_t *)part_buf;
    dst[nsamples * 0] = int2le24(src[0]);

      dst++;
      out_size++;
    }   
  }

  /////////////////////////////////////////////////////////
  // Set processing buffer start & end and remember 
  // remaining part of sample

  src = (int24_t *)rawdata;

  // integral number of samples (and its size in bytes)
  size_t n = nsamples - out_size;
  size_t n_size = n * sample_size;

  if (n_size < size)
  {
    drop_rawdata(n_size);
    end = (int24_t *)(((uint8_t *)src) + n_size);
    out_size += n;
  }
  else
  {
    n = size / sample_size;
    n_size = n * sample_size;

    drop_rawdata(n_size);
    end = (int24_t *)(((uint8_t *)src) + n_size);
    out_size += n;

    // remember part of sample
    if (size)
    {
      memcpy(part_buf, end, size);
      part_size = size;
      size = 0;
    }
  }

  /////////////////////////////////////////////////////////
  // Convert

  while (src < end)
  {
    dst[nsamples * 0] = int2le24(src[0]);

    src += nch;
    dst++;
  }
}
void
Converter::pcm32_linear_1ch()
{
  // input: spk, nsamples, rawdata, size
  // output: out_samples, out_size
  // intermediate: part_buf, part_size

  const int nch = 1;
  const size_t sample_size = sizeof(int32_t) * 1;

  int32_t *src;
  int32_t *end;
  sample_t *dst = out_samples[0];
  out_size = 0;

  /////////////////////////////////////////////////////////
  // Process part of sample

  if (part_size)
  {
    // assert: part_size < sample_size
    size_t delta = sample_size - part_size;
    if (size < delta)
    {
      // not enough data to fill sample buffer
      memcpy(part_buf + part_size, rawdata, size);
      part_size += size;
      size = 0;
      return;
    }
    else
    {
      // finish & convert incomplete sample 
      memcpy(part_buf + part_size, rawdata, delta);
      drop_rawdata(delta);
      part_size = 0;

      src = (int32_t *)part_buf;
    dst[nsamples * 0] = int2le32(src[0]);

      dst++;
      out_size++;
    }   
  }

  /////////////////////////////////////////////////////////
  // Set processing buffer start & end and remember 
  // remaining part of sample

  src = (int32_t *)rawdata;

  // integral number of samples (and its size in bytes)
  size_t n = nsamples - out_size;
  size_t n_size = n * sample_size;

  if (n_size < size)
  {
    drop_rawdata(n_size);
    end = (int32_t *)(((uint8_t *)src) + n_size);
    out_size += n;
  }
  else
  {
    n = size / sample_size;
    n_size = n * sample_size;

    drop_rawdata(n_size);
    end = (int32_t *)(((uint8_t *)src) + n_size);
    out_size += n;

    // remember part of sample
    if (size)
    {
      memcpy(part_buf, end, size);
      part_size = size;
      size = 0;
    }
  }

  /////////////////////////////////////////////////////////
  // Convert

  while (src < end)
  {
    dst[nsamples * 0] = int2le32(src[0]);

    src += nch;
    dst++;
  }
}
void
Converter::pcm16_be_linear_1ch()
{
  // input: spk, nsamples, rawdata, size
  // output: out_samples, out_size
  // intermediate: part_buf, part_size

  const int nch = 1;
  const size_t sample_size = sizeof(int16_t) * 1;

  int16_t *src;
  int16_t *end;
  sample_t *dst = out_samples[0];
  out_size = 0;

  /////////////////////////////////////////////////////////
  // Process part of sample

  if (part_size)
  {
    // assert: part_size < sample_size
    size_t delta = sample_size - part_size;
    if (size < delta)
    {
      // not enough data to fill sample buffer
      memcpy(part_buf + part_size, rawdata, size);
      part_size += size;
      size = 0;
      return;
    }
    else
    {
      // finish & convert incomplete sample 
      memcpy(part_buf + part_size, rawdata, delta);
      drop_rawdata(delta);
      part_size = 0;

      src = (int16_t *)part_buf;
    dst[nsamples * 0] = int2be16(src[0]);

      dst++;
      out_size++;
    }   
  }

  /////////////////////////////////////////////////////////
  // Set processing buffer start & end and remember 
  // remaining part of sample

  src = (int16_t *)rawdata;

  // integral number of samples (and its size in bytes)
  size_t n = nsamples - out_size;
  size_t n_size = n * sample_size;

  if (n_size < size)
  {
    drop_rawdata(n_size);
    end = (int16_t *)(((uint8_t *)src) + n_size);
    out_size += n;
  }
  else
  {
    n = size / sample_size;
    n_size = n * sample_size;

    drop_rawdata(n_size);
    end = (int16_t *)(((uint8_t *)src) + n_size);
    out_size += n;

    // remember part of sample
    if (size)
    {
      memcpy(part_buf, end, size);
      part_size = size;
      size = 0;
    }
  }

  /////////////////////////////////////////////////////////
  // Convert

  while (src < end)
  {
    dst[nsamples * 0] = int2be16(src[0]);

    src += nch;
    dst++;
  }
}
void
Converter::pcm24_be_linear_1ch()
{
  // input: spk, nsamples, rawdata, size
  // output: out_samples, out_size
  // intermediate: part_buf, part_size

  const int nch = 1;
  const size_t sample_size = sizeof(int24_t) * 1;

  int24_t *src;
  int24_t *end;
  sample_t *dst = out_samples[0];
  out_size = 0;

  /////////////////////////////////////////////////////////
  // Process part of sample

  if (part_size)
  {
    // assert: part_size < sample_size
    size_t delta = sample_size - part_size;
    if (size < delta)
    {
      // not enough data to fill sample buffer
      memcpy(part_buf + part_size, rawdata, size);
      part_size += size;
      size = 0;
      return;
    }
    else
    {
      // finish & convert incomplete sample 
      memcpy(part_buf + part_size, rawdata, delta);
      drop_rawdata(delta);
      part_size = 0;

      src = (int24_t *)part_buf;
    dst[nsamples * 0] = int2be24(src[0]);

      dst++;
      out_size++;
    }   
  }

  /////////////////////////////////////////////////////////
  // Set processing buffer start & end and remember 
  // remaining part of sample

  src = (int24_t *)rawdata;

  // integral number of samples (and its size in bytes)
  size_t n = nsamples - out_size;
  size_t n_size = n * sample_size;

  if (n_size < size)
  {
    drop_rawdata(n_size);
    end = (int24_t *)(((uint8_t *)src) + n_size);
    out_size += n;
  }
  else
  {
    n = size / sample_size;
    n_size = n * sample_size;

    drop_rawdata(n_size);
    end = (int24_t *)(((uint8_t *)src) + n_size);
    out_size += n;

    // remember part of sample
    if (size)
    {
      memcpy(part_buf, end, size);
      part_size = size;
      size = 0;
    }
  }

  /////////////////////////////////////////////////////////
  // Convert

  while (src < end)
  {
    dst[nsamples * 0] = int2be24(src[0]);

    src += nch;
    dst++;
  }
}
void
Converter::pcm32_be_linear_1ch()
{
  // input: spk, nsamples, rawdata, size
  // output: out_samples, out_size
  // intermediate: part_buf, part_size

  const int nch = 1;
  const size_t sample_size = sizeof(int32_t) * 1;

  int32_t *src;
  int32_t *end;
  sample_t *dst = out_samples[0];
  out_size = 0;

  /////////////////////////////////////////////////////////
  // Process part of sample

  if (part_size)
  {
    // assert: part_size < sample_size
    size_t delta = sample_size - part_size;
    if (size < delta)
    {
      // not enough data to fill sample buffer
      memcpy(part_buf + part_size, rawdata, size);
      part_size += size;
      size = 0;
      return;
    }
    else
    {
      // finish & convert incomplete sample 
      memcpy(part_buf + part_size, rawdata, delta);
      drop_rawdata(delta);
      part_size = 0;

      src = (int32_t *)part_buf;
    dst[nsamples * 0] = int2be32(src[0]);

      dst++;
      out_size++;
    }   
  }

  /////////////////////////////////////////////////////////
  // Set processing buffer start & end and remember 
  // remaining part of sample

  src = (int32_t *)rawdata;

  // integral number of samples (and its size in bytes)
  size_t n = nsamples - out_size;
  size_t n_size = n * sample_size;

  if (n_size < size)
  {
    drop_rawdata(n_size);
    end = (int32_t *)(((uint8_t *)src) + n_size);
    out_size += n;
  }
  else
  {
    n = size / sample_size;
    n_size = n * sample_size;

    drop_rawdata(n_size);
    end = (int32_t *)(((uint8_t *)src) + n_size);
    out_size += n;

    // remember part of sample
    if (size)
    {
      memcpy(part_buf, end, size);
      part_size = size;
      size = 0;
    }
  }

  /////////////////////////////////////////////////////////
  // Convert

  while (src < end)
  {
    dst[nsamples * 0] = int2be32(src[0]);

    src += nch;
    dst++;
  }
}
void
Converter::pcmfloat_linear_1ch()
{
  // input: spk, nsamples, rawdata, size
  // output: out_samples, out_size
  // intermediate: part_buf, part_size

  const int nch = 1;
  const size_t sample_size = sizeof(float) * 1;

  float *src;
  float *end;
  sample_t *dst = out_samples[0];
  out_size = 0;

  /////////////////////////////////////////////////////////
  // Process part of sample

  if (part_size)
  {
    // assert: part_size < sample_size
    size_t delta = sample_size - part_size;
    if (size < delta)
    {
      // not enough data to fill sample buffer
      memcpy(part_buf + part_size, rawdata, size);
      part_size += size;
      size = 0;
      return;
    }
    else
    {
      // finish & convert incomplete sample 
      memcpy(part_buf + part_size, rawdata, delta);
      drop_rawdata(delta);
      part_size = 0;

      src = (float *)part_buf;
    dst[nsamples * 0] = (src[0]);

      dst++;
      out_size++;
    }   
  }

  /////////////////////////////////////////////////////////
  // Set processing buffer start & end and remember 
  // remaining part of sample

  src = (float *)rawdata;

  // integral number of samples (and its size in bytes)
  size_t n = nsamples - out_size;
  size_t n_size = n * sample_size;

  if (n_size < size)
  {
    drop_rawdata(n_size);
    end = (float *)(((uint8_t *)src) + n_size);
    out_size += n;
  }
  else
  {
    n = size / sample_size;
    n_size = n * sample_size;

    drop_rawdata(n_size);
    end = (float *)(((uint8_t *)src) + n_size);
    out_size += n;

    // remember part of sample
    if (size)
    {
      memcpy(part_buf, end, size);
      part_size = size;
      size = 0;
    }
  }

  /////////////////////////////////////////////////////////
  // Convert

  while (src < end)
  {
    dst[nsamples * 0] = (src[0]);

    src += nch;
    dst++;
  }
}

void
Converter::pcm16_linear_2ch()
{
  // input: spk, nsamples, rawdata, size
  // output: out_samples, out_size
  // intermediate: part_buf, part_size

  const int nch = 2;
  const size_t sample_size = sizeof(int16_t) * 2;

  int16_t *src;
  int16_t *end;
  sample_t *dst = out_samples[0];
  out_size = 0;

  /////////////////////////////////////////////////////////
  // Process part of sample

  if (part_size)
  {
    // assert: part_size < sample_size
    size_t delta = sample_size - part_size;
    if (size < delta)
    {
      // not enough data to fill sample buffer
      memcpy(part_buf + part_size, rawdata, size);
      part_size += size;
      size = 0;
      return;
    }
    else
    {
      // finish & convert incomplete sample 
      memcpy(part_buf + part_size, rawdata, delta);
      drop_rawdata(delta);
      part_size = 0;

      src = (int16_t *)part_buf;
    dst[nsamples * 0] = int2le16(src[0]);
    dst[nsamples * 1] = int2le16(src[1]);

      dst++;
      out_size++;
    }   
  }

  /////////////////////////////////////////////////////////
  // Set processing buffer start & end and remember 
  // remaining part of sample

  src = (int16_t *)rawdata;

  // integral number of samples (and its size in bytes)
  size_t n = nsamples - out_size;
  size_t n_size = n * sample_size;

  if (n_size < size)
  {
    drop_rawdata(n_size);
    end = (int16_t *)(((uint8_t *)src) + n_size);
    out_size += n;
  }
  else
  {
    n = size / sample_size;
    n_size = n * sample_size;

    drop_rawdata(n_size);
    end = (int16_t *)(((uint8_t *)src) + n_size);
    out_size += n;

    // remember part of sample
    if (size)
    {
      memcpy(part_buf, end, size);
      part_size = size;
      size = 0;
    }
  }

  /////////////////////////////////////////////////////////
  // Convert

  while (src < end)
  {
    dst[nsamples * 0] = int2le16(src[0]);
    dst[nsamples * 1] = int2le16(src[1]);

    src += nch;
    dst++;
  }
}
void
Converter::pcm24_linear_2ch()
{
  // input: spk, nsamples, rawdata, size
  // output: out_samples, out_size
  // intermediate: part_buf, part_size

  const int nch = 2;
  const size_t sample_size = sizeof(int24_t) * 2;

  int24_t *src;
  int24_t *end;
  sample_t *dst = out_samples[0];
  out_size = 0;

  /////////////////////////////////////////////////////////
  // Process part of sample

  if (part_size)
  {
    // assert: part_size < sample_size
    size_t delta = sample_size - part_size;
    if (size < delta)
    {
      // not enough data to fill sample buffer
      memcpy(part_buf + part_size, rawdata, size);
      part_size += size;
      size = 0;
      return;
    }
    else
    {
      // finish & convert incomplete sample 
      memcpy(part_buf + part_size, rawdata, delta);
      drop_rawdata(delta);
      part_size = 0;

      src = (int24_t *)part_buf;
    dst[nsamples * 0] = int2le24(src[0]);
    dst[nsamples * 1] = int2le24(src[1]);

      dst++;
      out_size++;
    }   
  }

  /////////////////////////////////////////////////////////
  // Set processing buffer start & end and remember 
  // remaining part of sample

  src = (int24_t *)rawdata;

  // integral number of samples (and its size in bytes)
  size_t n = nsamples - out_size;
  size_t n_size = n * sample_size;

  if (n_size < size)
  {
    drop_rawdata(n_size);
    end = (int24_t *)(((uint8_t *)src) + n_size);
    out_size += n;
  }
  else
  {
    n = size / sample_size;
    n_size = n * sample_size;

    drop_rawdata(n_size);
    end = (int24_t *)(((uint8_t *)src) + n_size);
    out_size += n;

    // remember part of sample
    if (size)
    {
      memcpy(part_buf, end, size);
      part_size = size;
      size = 0;
    }
  }

  /////////////////////////////////////////////////////////
  // Convert

  while (src < end)
  {
    dst[nsamples * 0] = int2le24(src[0]);
    dst[nsamples * 1] = int2le24(src[1]);

    src += nch;
    dst++;
  }
}
void
Converter::pcm32_linear_2ch()
{
  // input: spk, nsamples, rawdata, size
  // output: out_samples, out_size
  // intermediate: part_buf, part_size

  const int nch = 2;
  const size_t sample_size = sizeof(int32_t) * 2;

  int32_t *src;
  int32_t *end;
  sample_t *dst = out_samples[0];
  out_size = 0;

  /////////////////////////////////////////////////////////
  // Process part of sample

  if (part_size)
  {
    // assert: part_size < sample_size
    size_t delta = sample_size - part_size;
    if (size < delta)
    {
      // not enough data to fill sample buffer
      memcpy(part_buf + part_size, rawdata, size);
      part_size += size;
      size = 0;
      return;
    }
    else
    {
      // finish & convert incomplete sample 
      memcpy(part_buf + part_size, rawdata, delta);
      drop_rawdata(delta);
      part_size = 0;

      src = (int32_t *)part_buf;
    dst[nsamples * 0] = int2le32(src[0]);
    dst[nsamples * 1] = int2le32(src[1]);

      dst++;
      out_size++;
    }   
  }

  /////////////////////////////////////////////////////////
  // Set processing buffer start & end and remember 
  // remaining part of sample

  src = (int32_t *)rawdata;

  // integral number of samples (and its size in bytes)
  size_t n = nsamples - out_size;
  size_t n_size = n * sample_size;

  if (n_size < size)
  {
    drop_rawdata(n_size);
    end = (int32_t *)(((uint8_t *)src) + n_size);
    out_size += n;
  }
  else
  {
    n = size / sample_size;
    n_size = n * sample_size;

    drop_rawdata(n_size);
    end = (int32_t *)(((uint8_t *)src) + n_size);
    out_size += n;

    // remember part of sample
    if (size)
    {
      memcpy(part_buf, end, size);
      part_size = size;
      size = 0;
    }
  }

  /////////////////////////////////////////////////////////
  // Convert

  while (src < end)
  {
    dst[nsamples * 0] = int2le32(src[0]);
    dst[nsamples * 1] = int2le32(src[1]);

    src += nch;
    dst++;
  }
}
void
Converter::pcm16_be_linear_2ch()
{
  // input: spk, nsamples, rawdata, size
  // output: out_samples, out_size
  // intermediate: part_buf, part_size

  const int nch = 2;
  const size_t sample_size = sizeof(int16_t) * 2;

  int16_t *src;
  int16_t *end;
  sample_t *dst = out_samples[0];
  out_size = 0;

  /////////////////////////////////////////////////////////
  // Process part of sample

  if (part_size)
  {
    // assert: part_size < sample_size
    size_t delta = sample_size - part_size;
    if (size < delta)
    {
      // not enough data to fill sample buffer
      memcpy(part_buf + part_size, rawdata, size);
      part_size += size;
      size = 0;
      return;
    }
    else
    {
      // finish & convert incomplete sample 
      memcpy(part_buf + part_size, rawdata, delta);
      drop_rawdata(delta);
      part_size = 0;

      src = (int16_t *)part_buf;
    dst[nsamples * 0] = int2be16(src[0]);
    dst[nsamples * 1] = int2be16(src[1]);

      dst++;
      out_size++;
    }   
  }

  /////////////////////////////////////////////////////////
  // Set processing buffer start & end and remember 
  // remaining part of sample

  src = (int16_t *)rawdata;

  // integral number of samples (and its size in bytes)
  size_t n = nsamples - out_size;
  size_t n_size = n * sample_size;

  if (n_size < size)
  {
    drop_rawdata(n_size);
    end = (int16_t *)(((uint8_t *)src) + n_size);
    out_size += n;
  }
  else
  {
    n = size / sample_size;
    n_size = n * sample_size;

    drop_rawdata(n_size);
    end = (int16_t *)(((uint8_t *)src) + n_size);
    out_size += n;

    // remember part of sample
    if (size)
    {
      memcpy(part_buf, end, size);
      part_size = size;
      size = 0;
    }
  }

  /////////////////////////////////////////////////////////
  // Convert

  while (src < end)
  {
    dst[nsamples * 0] = int2be16(src[0]);
    dst[nsamples * 1] = int2be16(src[1]);

    src += nch;
    dst++;
  }
}
void
Converter::pcm24_be_linear_2ch()
{
  // input: spk, nsamples, rawdata, size
  // output: out_samples, out_size
  // intermediate: part_buf, part_size

  const int nch = 2;
  const size_t sample_size = sizeof(int24_t) * 2;

  int24_t *src;
  int24_t *end;
  sample_t *dst = out_samples[0];
  out_size = 0;

  /////////////////////////////////////////////////////////
  // Process part of sample

  if (part_size)
  {
    // assert: part_size < sample_size
    size_t delta = sample_size - part_size;
    if (size < delta)
    {
      // not enough data to fill sample buffer
      memcpy(part_buf + part_size, rawdata, size);
      part_size += size;
      size = 0;
      return;
    }
    else
    {
      // finish & convert incomplete sample 
      memcpy(part_buf + part_size, rawdata, delta);
      drop_rawdata(delta);
      part_size = 0;

      src = (int24_t *)part_buf;
    dst[nsamples * 0] = int2be24(src[0]);
    dst[nsamples * 1] = int2be24(src[1]);

      dst++;
      out_size++;
    }   
  }

  /////////////////////////////////////////////////////////
  // Set processing buffer start & end and remember 
  // remaining part of sample

  src = (int24_t *)rawdata;

  // integral number of samples (and its size in bytes)
  size_t n = nsamples - out_size;
  size_t n_size = n * sample_size;

  if (n_size < size)
  {
    drop_rawdata(n_size);
    end = (int24_t *)(((uint8_t *)src) + n_size);
    out_size += n;
  }
  else
  {
    n = size / sample_size;
    n_size = n * sample_size;

    drop_rawdata(n_size);
    end = (int24_t *)(((uint8_t *)src) + n_size);
    out_size += n;

    // remember part of sample
    if (size)
    {
      memcpy(part_buf, end, size);
      part_size = size;
      size = 0;
    }
  }

  /////////////////////////////////////////////////////////
  // Convert

  while (src < end)
  {
    dst[nsamples * 0] = int2be24(src[0]);
    dst[nsamples * 1] = int2be24(src[1]);

    src += nch;
    dst++;
  }
}
void
Converter::pcm32_be_linear_2ch()
{
  // input: spk, nsamples, rawdata, size
  // output: out_samples, out_size
  // intermediate: part_buf, part_size

  const int nch = 2;
  const size_t sample_size = sizeof(int32_t) * 2;

  int32_t *src;
  int32_t *end;
  sample_t *dst = out_samples[0];
  out_size = 0;

  /////////////////////////////////////////////////////////
  // Process part of sample

  if (part_size)
  {
    // assert: part_size < sample_size
    size_t delta = sample_size - part_size;
    if (size < delta)
    {
      // not enough data to fill sample buffer
      memcpy(part_buf + part_size, rawdata, size);
      part_size += size;
      size = 0;
      return;
    }
    else
    {
      // finish & convert incomplete sample 
      memcpy(part_buf + part_size, rawdata, delta);
      drop_rawdata(delta);
      part_size = 0;

      src = (int32_t *)part_buf;
    dst[nsamples * 0] = int2be32(src[0]);
    dst[nsamples * 1] = int2be32(src[1]);

      dst++;
      out_size++;
    }   
  }

  /////////////////////////////////////////////////////////
  // Set processing buffer start & end and remember 
  // remaining part of sample

  src = (int32_t *)rawdata;

  // integral number of samples (and its size in bytes)
  size_t n = nsamples - out_size;
  size_t n_size = n * sample_size;

  if (n_size < size)
  {
    drop_rawdata(n_size);
    end = (int32_t *)(((uint8_t *)src) + n_size);
    out_size += n;
  }
  else
  {
    n = size / sample_size;
    n_size = n * sample_size;

    drop_rawdata(n_size);
    end = (int32_t *)(((uint8_t *)src) + n_size);
    out_size += n;

    // remember part of sample
    if (size)
    {
      memcpy(part_buf, end, size);
      part_size = size;
      size = 0;
    }
  }

  /////////////////////////////////////////////////////////
  // Convert

  while (src < end)
  {
    dst[nsamples * 0] = int2be32(src[0]);
    dst[nsamples * 1] = int2be32(src[1]);

    src += nch;
    dst++;
  }
}
void
Converter::pcmfloat_linear_2ch()
{
  // input: spk, nsamples, rawdata, size
  // output: out_samples, out_size
  // intermediate: part_buf, part_size

  const int nch = 2;
  const size_t sample_size = sizeof(float) * 2;

  float *src;
  float *end;
  sample_t *dst = out_samples[0];
  out_size = 0;

  /////////////////////////////////////////////////////////
  // Process part of sample

  if (part_size)
  {
    // assert: part_size < sample_size
    size_t delta = sample_size - part_size;
    if (size < delta)
    {
      // not enough data to fill sample buffer
      memcpy(part_buf + part_size, rawdata, size);
      part_size += size;
      size = 0;
      return;
    }
    else
    {
      // finish & convert incomplete sample 
      memcpy(part_buf + part_size, rawdata, delta);
      drop_rawdata(delta);
      part_size = 0;

      src = (float *)part_buf;
    dst[nsamples * 0] = (src[0]);
    dst[nsamples * 1] = (src[1]);

      dst++;
      out_size++;
    }   
  }

  /////////////////////////////////////////////////////////
  // Set processing buffer start & end and remember 
  // remaining part of sample

  src = (float *)rawdata;

  // integral number of samples (and its size in bytes)
  size_t n = nsamples - out_size;
  size_t n_size = n * sample_size;

  if (n_size < size)
  {
    drop_rawdata(n_size);
    end = (float *)(((uint8_t *)src) + n_size);
    out_size += n;
  }
  else
  {
    n = size / sample_size;
    n_size = n * sample_size;

    drop_rawdata(n_size);
    end = (float *)(((uint8_t *)src) + n_size);
    out_size += n;

    // remember part of sample
    if (size)
    {
      memcpy(part_buf, end, size);
      part_size = size;
      size = 0;
    }
  }

  /////////////////////////////////////////////////////////
  // Convert

  while (src < end)
  {
    dst[nsamples * 0] = (src[0]);
    dst[nsamples * 1] = (src[1]);

    src += nch;
    dst++;
  }
}

void
Converter::pcm16_linear_3ch()
{
  // input: spk, nsamples, rawdata, size
  // output: out_samples, out_size
  // intermediate: part_buf, part_size

  const int nch = 3;
  const size_t sample_size = sizeof(int16_t) * 3;

  int16_t *src;
  int16_t *end;
  sample_t *dst = out_samples[0];
  out_size = 0;

  /////////////////////////////////////////////////////////
  // Process part of sample

  if (part_size)
  {
    // assert: part_size < sample_size
    size_t delta = sample_size - part_size;
    if (size < delta)
    {
      // not enough data to fill sample buffer
      memcpy(part_buf + part_size, rawdata, size);
      part_size += size;
      size = 0;
      return;
    }
    else
    {
      // finish & convert incomplete sample 
      memcpy(part_buf + part_size, rawdata, delta);
      drop_rawdata(delta);
      part_size = 0;

      src = (int16_t *)part_buf;
    dst[nsamples * 0] = int2le16(src[0]);
    dst[nsamples * 1] = int2le16(src[1]);
    dst[nsamples * 2] = int2le16(src[2]);

      dst++;
      out_size++;
    }   
  }

  /////////////////////////////////////////////////////////
  // Set processing buffer start & end and remember 
  // remaining part of sample

  src = (int16_t *)rawdata;

  // integral number of samples (and its size in bytes)
  size_t n = nsamples - out_size;
  size_t n_size = n * sample_size;

  if (n_size < size)
  {
    drop_rawdata(n_size);
    end = (int16_t *)(((uint8_t *)src) + n_size);
    out_size += n;
  }
  else
  {
    n = size / sample_size;
    n_size = n * sample_size;

    drop_rawdata(n_size);
    end = (int16_t *)(((uint8_t *)src) + n_size);
    out_size += n;

    // remember part of sample
    if (size)
    {
      memcpy(part_buf, end, size);
      part_size = size;
      size = 0;
    }
  }

  /////////////////////////////////////////////////////////
  // Convert

  while (src < end)
  {
    dst[nsamples * 0] = int2le16(src[0]);
    dst[nsamples * 1] = int2le16(src[1]);
    dst[nsamples * 2] = int2le16(src[2]);

    src += nch;
    dst++;
  }
}
void
Converter::pcm24_linear_3ch()
{
  // input: spk, nsamples, rawdata, size
  // output: out_samples, out_size
  // intermediate: part_buf, part_size

  const int nch = 3;
  const size_t sample_size = sizeof(int24_t) * 3;

  int24_t *src;
  int24_t *end;
  sample_t *dst = out_samples[0];
  out_size = 0;

  /////////////////////////////////////////////////////////
  // Process part of sample

  if (part_size)
  {
    // assert: part_size < sample_size
    size_t delta = sample_size - part_size;
    if (size < delta)
    {
      // not enough data to fill sample buffer
      memcpy(part_buf + part_size, rawdata, size);
      part_size += size;
      size = 0;
      return;
    }
    else
    {
      // finish & convert incomplete sample 
      memcpy(part_buf + part_size, rawdata, delta);
      drop_rawdata(delta);
      part_size = 0;

      src = (int24_t *)part_buf;
    dst[nsamples * 0] = int2le24(src[0]);
    dst[nsamples * 1] = int2le24(src[1]);
    dst[nsamples * 2] = int2le24(src[2]);

      dst++;
      out_size++;
    }   
  }

  /////////////////////////////////////////////////////////
  // Set processing buffer start & end and remember 
  // remaining part of sample

  src = (int24_t *)rawdata;

  // integral number of samples (and its size in bytes)
  size_t n = nsamples - out_size;
  size_t n_size = n * sample_size;

  if (n_size < size)
  {
    drop_rawdata(n_size);
    end = (int24_t *)(((uint8_t *)src) + n_size);
    out_size += n;
  }
  else
  {
    n = size / sample_size;
    n_size = n * sample_size;

    drop_rawdata(n_size);
    end = (int24_t *)(((uint8_t *)src) + n_size);
    out_size += n;

    // remember part of sample
    if (size)
    {
      memcpy(part_buf, end, size);
      part_size = size;
      size = 0;
    }
  }

  /////////////////////////////////////////////////////////
  // Convert

  while (src < end)
  {
    dst[nsamples * 0] = int2le24(src[0]);
    dst[nsamples * 1] = int2le24(src[1]);
    dst[nsamples * 2] = int2le24(src[2]);

    src += nch;
    dst++;
  }
}
void
Converter::pcm32_linear_3ch()
{
  // input: spk, nsamples, rawdata, size
  // output: out_samples, out_size
  // intermediate: part_buf, part_size

  const int nch = 3;
  const size_t sample_size = sizeof(int32_t) * 3;

  int32_t *src;
  int32_t *end;
  sample_t *dst = out_samples[0];
  out_size = 0;

  /////////////////////////////////////////////////////////
  // Process part of sample

  if (part_size)
  {
    // assert: part_size < sample_size
    size_t delta = sample_size - part_size;
    if (size < delta)
    {
      // not enough data to fill sample buffer
      memcpy(part_buf + part_size, rawdata, size);
      part_size += size;
      size = 0;
      return;
    }
    else
    {
      // finish & convert incomplete sample 
      memcpy(part_buf + part_size, rawdata, delta);
      drop_rawdata(delta);
      part_size = 0;

      src = (int32_t *)part_buf;
    dst[nsamples * 0] = int2le32(src[0]);
    dst[nsamples * 1] = int2le32(src[1]);
    dst[nsamples * 2] = int2le32(src[2]);

      dst++;
      out_size++;
    }   
  }

  /////////////////////////////////////////////////////////
  // Set processing buffer start & end and remember 
  // remaining part of sample

  src = (int32_t *)rawdata;

  // integral number of samples (and its size in bytes)
  size_t n = nsamples - out_size;
  size_t n_size = n * sample_size;

  if (n_size < size)
  {
    drop_rawdata(n_size);
    end = (int32_t *)(((uint8_t *)src) + n_size);
    out_size += n;
  }
  else
  {
    n = size / sample_size;
    n_size = n * sample_size;

    drop_rawdata(n_size);
    end = (int32_t *)(((uint8_t *)src) + n_size);
    out_size += n;

    // remember part of sample
    if (size)
    {
      memcpy(part_buf, end, size);
      part_size = size;
      size = 0;
    }
  }

  /////////////////////////////////////////////////////////
  // Convert

  while (src < end)
  {
    dst[nsamples * 0] = int2le32(src[0]);
    dst[nsamples * 1] = int2le32(src[1]);
    dst[nsamples * 2] = int2le32(src[2]);

    src += nch;
    dst++;
  }
}
void
Converter::pcm16_be_linear_3ch()
{
  // input: spk, nsamples, rawdata, size
  // output: out_samples, out_size
  // intermediate: part_buf, part_size

  const int nch = 3;
  const size_t sample_size = sizeof(int16_t) * 3;

  int16_t *src;
  int16_t *end;
  sample_t *dst = out_samples[0];
  out_size = 0;

  /////////////////////////////////////////////////////////
  // Process part of sample

  if (part_size)
  {
    // assert: part_size < sample_size
    size_t delta = sample_size - part_size;
    if (size < delta)
    {
      // not enough data to fill sample buffer
      memcpy(part_buf + part_size, rawdata, size);
      part_size += size;
      size = 0;
      return;
    }
    else
    {
      // finish & convert incomplete sample 
      memcpy(part_buf + part_size, rawdata, delta);
      drop_rawdata(delta);
      part_size = 0;

      src = (int16_t *)part_buf;
    dst[nsamples * 0] = int2be16(src[0]);
    dst[nsamples * 1] = int2be16(src[1]);
    dst[nsamples * 2] = int2be16(src[2]);

      dst++;
      out_size++;
    }   
  }

  /////////////////////////////////////////////////////////
  // Set processing buffer start & end and remember 
  // remaining part of sample

  src = (int16_t *)rawdata;

  // integral number of samples (and its size in bytes)
  size_t n = nsamples - out_size;
  size_t n_size = n * sample_size;

  if (n_size < size)
  {
    drop_rawdata(n_size);
    end = (int16_t *)(((uint8_t *)src) + n_size);
    out_size += n;
  }
  else
  {
    n = size / sample_size;
    n_size = n * sample_size;

    drop_rawdata(n_size);
    end = (int16_t *)(((uint8_t *)src) + n_size);
    out_size += n;

    // remember part of sample
    if (size)
    {
      memcpy(part_buf, end, size);
      part_size = size;
      size = 0;
    }
  }

  /////////////////////////////////////////////////////////
  // Convert

  while (src < end)
  {
    dst[nsamples * 0] = int2be16(src[0]);
    dst[nsamples * 1] = int2be16(src[1]);
    dst[nsamples * 2] = int2be16(src[2]);

    src += nch;
    dst++;
  }
}
void
Converter::pcm24_be_linear_3ch()
{
  // input: spk, nsamples, rawdata, size
  // output: out_samples, out_size
  // intermediate: part_buf, part_size

  const int nch = 3;
  const size_t sample_size = sizeof(int24_t) * 3;

  int24_t *src;
  int24_t *end;
  sample_t *dst = out_samples[0];
  out_size = 0;

  /////////////////////////////////////////////////////////
  // Process part of sample

  if (part_size)
  {
    // assert: part_size < sample_size
    size_t delta = sample_size - part_size;
    if (size < delta)
    {
      // not enough data to fill sample buffer
      memcpy(part_buf + part_size, rawdata, size);
      part_size += size;
      size = 0;
      return;
    }
    else
    {
      // finish & convert incomplete sample 
      memcpy(part_buf + part_size, rawdata, delta);
      drop_rawdata(delta);
      part_size = 0;

      src = (int24_t *)part_buf;
    dst[nsamples * 0] = int2be24(src[0]);
    dst[nsamples * 1] = int2be24(src[1]);
    dst[nsamples * 2] = int2be24(src[2]);

      dst++;
      out_size++;
    }   
  }

  /////////////////////////////////////////////////////////
  // Set processing buffer start & end and remember 
  // remaining part of sample

  src = (int24_t *)rawdata;

  // integral number of samples (and its size in bytes)
  size_t n = nsamples - out_size;
  size_t n_size = n * sample_size;

  if (n_size < size)
  {
    drop_rawdata(n_size);
    end = (int24_t *)(((uint8_t *)src) + n_size);
    out_size += n;
  }
  else
  {
    n = size / sample_size;
    n_size = n * sample_size;

    drop_rawdata(n_size);
    end = (int24_t *)(((uint8_t *)src) + n_size);
    out_size += n;

    // remember part of sample
    if (size)
    {
      memcpy(part_buf, end, size);
      part_size = size;
      size = 0;
    }
  }

  /////////////////////////////////////////////////////////
  // Convert

  while (src < end)
  {
    dst[nsamples * 0] = int2be24(src[0]);
    dst[nsamples * 1] = int2be24(src[1]);
    dst[nsamples * 2] = int2be24(src[2]);

    src += nch;
    dst++;
  }
}
void
Converter::pcm32_be_linear_3ch()
{
  // input: spk, nsamples, rawdata, size
  // output: out_samples, out_size
  // intermediate: part_buf, part_size

  const int nch = 3;
  const size_t sample_size = sizeof(int32_t) * 3;

  int32_t *src;
  int32_t *end;
  sample_t *dst = out_samples[0];
  out_size = 0;

  /////////////////////////////////////////////////////////
  // Process part of sample

  if (part_size)
  {
    // assert: part_size < sample_size
    size_t delta = sample_size - part_size;
    if (size < delta)
    {
      // not enough data to fill sample buffer
      memcpy(part_buf + part_size, rawdata, size);
      part_size += size;
      size = 0;
      return;
    }
    else
    {
      // finish & convert incomplete sample 
      memcpy(part_buf + part_size, rawdata, delta);
      drop_rawdata(delta);
      part_size = 0;

      src = (int32_t *)part_buf;
    dst[nsamples * 0] = int2be32(src[0]);
    dst[nsamples * 1] = int2be32(src[1]);
    dst[nsamples * 2] = int2be32(src[2]);

      dst++;
      out_size++;
    }   
  }

  /////////////////////////////////////////////////////////
  // Set processing buffer start & end and remember 
  // remaining part of sample

  src = (int32_t *)rawdata;

  // integral number of samples (and its size in bytes)
  size_t n = nsamples - out_size;
  size_t n_size = n * sample_size;

  if (n_size < size)
  {
    drop_rawdata(n_size);
    end = (int32_t *)(((uint8_t *)src) + n_size);
    out_size += n;
  }
  else
  {
    n = size / sample_size;
    n_size = n * sample_size;

    drop_rawdata(n_size);
    end = (int32_t *)(((uint8_t *)src) + n_size);
    out_size += n;

    // remember part of sample
    if (size)
    {
      memcpy(part_buf, end, size);
      part_size = size;
      size = 0;
    }
  }

  /////////////////////////////////////////////////////////
  // Convert

  while (src < end)
  {
    dst[nsamples * 0] = int2be32(src[0]);
    dst[nsamples * 1] = int2be32(src[1]);
    dst[nsamples * 2] = int2be32(src[2]);

    src += nch;
    dst++;
  }
}
void
Converter::pcmfloat_linear_3ch()
{
  // input: spk, nsamples, rawdata, size
  // output: out_samples, out_size
  // intermediate: part_buf, part_size

  const int nch = 3;
  const size_t sample_size = sizeof(float) * 3;

  float *src;
  float *end;
  sample_t *dst = out_samples[0];
  out_size = 0;

  /////////////////////////////////////////////////////////
  // Process part of sample

  if (part_size)
  {
    // assert: part_size < sample_size
    size_t delta = sample_size - part_size;
    if (size < delta)
    {
      // not enough data to fill sample buffer
      memcpy(part_buf + part_size, rawdata, size);
      part_size += size;
      size = 0;
      return;
    }
    else
    {
      // finish & convert incomplete sample 
      memcpy(part_buf + part_size, rawdata, delta);
      drop_rawdata(delta);
      part_size = 0;

      src = (float *)part_buf;
    dst[nsamples * 0] = (src[0]);
    dst[nsamples * 1] = (src[1]);
    dst[nsamples * 2] = (src[2]);

      dst++;
      out_size++;
    }   
  }

  /////////////////////////////////////////////////////////
  // Set processing buffer start & end and remember 
  // remaining part of sample

  src = (float *)rawdata;

  // integral number of samples (and its size in bytes)
  size_t n = nsamples - out_size;
  size_t n_size = n * sample_size;

  if (n_size < size)
  {
    drop_rawdata(n_size);
    end = (float *)(((uint8_t *)src) + n_size);
    out_size += n;
  }
  else
  {
    n = size / sample_size;
    n_size = n * sample_size;

    drop_rawdata(n_size);
    end = (float *)(((uint8_t *)src) + n_size);
    out_size += n;

    // remember part of sample
    if (size)
    {
      memcpy(part_buf, end, size);
      part_size = size;
      size = 0;
    }
  }

  /////////////////////////////////////////////////////////
  // Convert

  while (src < end)
  {
    dst[nsamples * 0] = (src[0]);
    dst[nsamples * 1] = (src[1]);
    dst[nsamples * 2] = (src[2]);

    src += nch;
    dst++;
  }
}

void
Converter::pcm16_linear_4ch()
{
  // input: spk, nsamples, rawdata, size
  // output: out_samples, out_size
  // intermediate: part_buf, part_size

  const int nch = 4;
  const size_t sample_size = sizeof(int16_t) * 4;

  int16_t *src;
  int16_t *end;
  sample_t *dst = out_samples[0];
  out_size = 0;

  /////////////////////////////////////////////////////////
  // Process part of sample

  if (part_size)
  {
    // assert: part_size < sample_size
    size_t delta = sample_size - part_size;
    if (size < delta)
    {
      // not enough data to fill sample buffer
      memcpy(part_buf + part_size, rawdata, size);
      part_size += size;
      size = 0;
      return;
    }
    else
    {
      // finish & convert incomplete sample 
      memcpy(part_buf + part_size, rawdata, delta);
      drop_rawdata(delta);
      part_size = 0;

      src = (int16_t *)part_buf;
    dst[nsamples * 0] = int2le16(src[0]);
    dst[nsamples * 1] = int2le16(src[1]);
    dst[nsamples * 2] = int2le16(src[2]);
    dst[nsamples * 3] = int2le16(src[3]);

      dst++;
      out_size++;
    }   
  }

  /////////////////////////////////////////////////////////
  // Set processing buffer start & end and remember 
  // remaining part of sample

  src = (int16_t *)rawdata;

  // integral number of samples (and its size in bytes)
  size_t n = nsamples - out_size;
  size_t n_size = n * sample_size;

  if (n_size < size)
  {
    drop_rawdata(n_size);
    end = (int16_t *)(((uint8_t *)src) + n_size);
    out_size += n;
  }
  else
  {
    n = size / sample_size;
    n_size = n * sample_size;

    drop_rawdata(n_size);
    end = (int16_t *)(((uint8_t *)src) + n_size);
    out_size += n;

    // remember part of sample
    if (size)
    {
      memcpy(part_buf, end, size);
      part_size = size;
      size = 0;
    }
  }

  /////////////////////////////////////////////////////////
  // Convert

  while (src < end)
  {
    dst[nsamples * 0] = int2le16(src[0]);
    dst[nsamples * 1] = int2le16(src[1]);
    dst[nsamples * 2] = int2le16(src[2]);
    dst[nsamples * 3] = int2le16(src[3]);

    src += nch;
    dst++;
  }
}
void
Converter::pcm24_linear_4ch()
{
  // input: spk, nsamples, rawdata, size
  // output: out_samples, out_size
  // intermediate: part_buf, part_size

  const int nch = 4;
  const size_t sample_size = sizeof(int24_t) * 4;

  int24_t *src;
  int24_t *end;
  sample_t *dst = out_samples[0];
  out_size = 0;

  /////////////////////////////////////////////////////////
  // Process part of sample

  if (part_size)
  {
    // assert: part_size < sample_size
    size_t delta = sample_size - part_size;
    if (size < delta)
    {
      // not enough data to fill sample buffer
      memcpy(part_buf + part_size, rawdata, size);
      part_size += size;
      size = 0;
      return;
    }
    else
    {
      // finish & convert incomplete sample 
      memcpy(part_buf + part_size, rawdata, delta);
      drop_rawdata(delta);
      part_size = 0;

      src = (int24_t *)part_buf;
    dst[nsamples * 0] = int2le24(src[0]);
    dst[nsamples * 1] = int2le24(src[1]);
    dst[nsamples * 2] = int2le24(src[2]);
    dst[nsamples * 3] = int2le24(src[3]);

      dst++;
      out_size++;
    }   
  }

  /////////////////////////////////////////////////////////
  // Set processing buffer start & end and remember 
  // remaining part of sample

  src = (int24_t *)rawdata;

  // integral number of samples (and its size in bytes)
  size_t n = nsamples - out_size;
  size_t n_size = n * sample_size;

  if (n_size < size)
  {
    drop_rawdata(n_size);
    end = (int24_t *)(((uint8_t *)src) + n_size);
    out_size += n;
  }
  else
  {
    n = size / sample_size;
    n_size = n * sample_size;

    drop_rawdata(n_size);
    end = (int24_t *)(((uint8_t *)src) + n_size);
    out_size += n;

    // remember part of sample
    if (size)
    {
      memcpy(part_buf, end, size);
      part_size = size;
      size = 0;
    }
  }

  /////////////////////////////////////////////////////////
  // Convert

  while (src < end)
  {
    dst[nsamples * 0] = int2le24(src[0]);
    dst[nsamples * 1] = int2le24(src[1]);
    dst[nsamples * 2] = int2le24(src[2]);
    dst[nsamples * 3] = int2le24(src[3]);

    src += nch;
    dst++;
  }
}
void
Converter::pcm32_linear_4ch()
{
  // input: spk, nsamples, rawdata, size
  // output: out_samples, out_size
  // intermediate: part_buf, part_size

  const int nch = 4;
  const size_t sample_size = sizeof(int32_t) * 4;

  int32_t *src;
  int32_t *end;
  sample_t *dst = out_samples[0];
  out_size = 0;

  /////////////////////////////////////////////////////////
  // Process part of sample

  if (part_size)
  {
    // assert: part_size < sample_size
    size_t delta = sample_size - part_size;
    if (size < delta)
    {
      // not enough data to fill sample buffer
      memcpy(part_buf + part_size, rawdata, size);
      part_size += size;
      size = 0;
      return;
    }
    else
    {
      // finish & convert incomplete sample 
      memcpy(part_buf + part_size, rawdata, delta);
      drop_rawdata(delta);
      part_size = 0;

      src = (int32_t *)part_buf;
    dst[nsamples * 0] = int2le32(src[0]);
    dst[nsamples * 1] = int2le32(src[1]);
    dst[nsamples * 2] = int2le32(src[2]);
    dst[nsamples * 3] = int2le32(src[3]);

      dst++;
      out_size++;
    }   
  }

  /////////////////////////////////////////////////////////
  // Set processing buffer start & end and remember 
  // remaining part of sample

  src = (int32_t *)rawdata;

  // integral number of samples (and its size in bytes)
  size_t n = nsamples - out_size;
  size_t n_size = n * sample_size;

  if (n_size < size)
  {
    drop_rawdata(n_size);
    end = (int32_t *)(((uint8_t *)src) + n_size);
    out_size += n;
  }
  else
  {
    n = size / sample_size;
    n_size = n * sample_size;

    drop_rawdata(n_size);
    end = (int32_t *)(((uint8_t *)src) + n_size);
    out_size += n;

    // remember part of sample
    if (size)
    {
      memcpy(part_buf, end, size);
      part_size = size;
      size = 0;
    }
  }

  /////////////////////////////////////////////////////////
  // Convert

  while (src < end)
  {
    dst[nsamples * 0] = int2le32(src[0]);
    dst[nsamples * 1] = int2le32(src[1]);
    dst[nsamples * 2] = int2le32(src[2]);
    dst[nsamples * 3] = int2le32(src[3]);

    src += nch;
    dst++;
  }
}
void
Converter::pcm16_be_linear_4ch()
{
  // input: spk, nsamples, rawdata, size
  // output: out_samples, out_size
  // intermediate: part_buf, part_size

  const int nch = 4;
  const size_t sample_size = sizeof(int16_t) * 4;

  int16_t *src;
  int16_t *end;
  sample_t *dst = out_samples[0];
  out_size = 0;

  /////////////////////////////////////////////////////////
  // Process part of sample

  if (part_size)
  {
    // assert: part_size < sample_size
    size_t delta = sample_size - part_size;
    if (size < delta)
    {
      // not enough data to fill sample buffer
      memcpy(part_buf + part_size, rawdata, size);
      part_size += size;
      size = 0;
      return;
    }
    else
    {
      // finish & convert incomplete sample 
      memcpy(part_buf + part_size, rawdata, delta);
      drop_rawdata(delta);
      part_size = 0;

      src = (int16_t *)part_buf;
    dst[nsamples * 0] = int2be16(src[0]);
    dst[nsamples * 1] = int2be16(src[1]);
    dst[nsamples * 2] = int2be16(src[2]);
    dst[nsamples * 3] = int2be16(src[3]);

      dst++;
      out_size++;
    }   
  }

  /////////////////////////////////////////////////////////
  // Set processing buffer start & end and remember 
  // remaining part of sample

  src = (int16_t *)rawdata;

  // integral number of samples (and its size in bytes)
  size_t n = nsamples - out_size;
  size_t n_size = n * sample_size;

  if (n_size < size)
  {
    drop_rawdata(n_size);
    end = (int16_t *)(((uint8_t *)src) + n_size);
    out_size += n;
  }
  else
  {
    n = size / sample_size;
    n_size = n * sample_size;

    drop_rawdata(n_size);
    end = (int16_t *)(((uint8_t *)src) + n_size);
    out_size += n;

    // remember part of sample
    if (size)
    {
      memcpy(part_buf, end, size);
      part_size = size;
      size = 0;
    }
  }

  /////////////////////////////////////////////////////////
  // Convert

  while (src < end)
  {
    dst[nsamples * 0] = int2be16(src[0]);
    dst[nsamples * 1] = int2be16(src[1]);
    dst[nsamples * 2] = int2be16(src[2]);
    dst[nsamples * 3] = int2be16(src[3]);

    src += nch;
    dst++;
  }
}
void
Converter::pcm24_be_linear_4ch()
{
  // input: spk, nsamples, rawdata, size
  // output: out_samples, out_size
  // intermediate: part_buf, part_size

  const int nch = 4;
  const size_t sample_size = sizeof(int24_t) * 4;

  int24_t *src;
  int24_t *end;
  sample_t *dst = out_samples[0];
  out_size = 0;

  /////////////////////////////////////////////////////////
  // Process part of sample

  if (part_size)
  {
    // assert: part_size < sample_size
    size_t delta = sample_size - part_size;
    if (size < delta)
    {
      // not enough data to fill sample buffer
      memcpy(part_buf + part_size, rawdata, size);
      part_size += size;
      size = 0;
      return;
    }
    else
    {
      // finish & convert incomplete sample 
      memcpy(part_buf + part_size, rawdata, delta);
      drop_rawdata(delta);
      part_size = 0;

      src = (int24_t *)part_buf;
    dst[nsamples * 0] = int2be24(src[0]);
    dst[nsamples * 1] = int2be24(src[1]);
    dst[nsamples * 2] = int2be24(src[2]);
    dst[nsamples * 3] = int2be24(src[3]);

      dst++;
      out_size++;
    }   
  }

  /////////////////////////////////////////////////////////
  // Set processing buffer start & end and remember 
  // remaining part of sample

  src = (int24_t *)rawdata;

  // integral number of samples (and its size in bytes)
  size_t n = nsamples - out_size;
  size_t n_size = n * sample_size;

  if (n_size < size)
  {
    drop_rawdata(n_size);
    end = (int24_t *)(((uint8_t *)src) + n_size);
    out_size += n;
  }
  else
  {
    n = size / sample_size;
    n_size = n * sample_size;

    drop_rawdata(n_size);
    end = (int24_t *)(((uint8_t *)src) + n_size);
    out_size += n;

    // remember part of sample
    if (size)
    {
      memcpy(part_buf, end, size);
      part_size = size;
      size = 0;
    }
  }

  /////////////////////////////////////////////////////////
  // Convert

  while (src < end)
  {
    dst[nsamples * 0] = int2be24(src[0]);
    dst[nsamples * 1] = int2be24(src[1]);
    dst[nsamples * 2] = int2be24(src[2]);
    dst[nsamples * 3] = int2be24(src[3]);

    src += nch;
    dst++;
  }
}
void
Converter::pcm32_be_linear_4ch()
{
  // input: spk, nsamples, rawdata, size
  // output: out_samples, out_size
  // intermediate: part_buf, part_size

  const int nch = 4;
  const size_t sample_size = sizeof(int32_t) * 4;

  int32_t *src;
  int32_t *end;
  sample_t *dst = out_samples[0];
  out_size = 0;

  /////////////////////////////////////////////////////////
  // Process part of sample

  if (part_size)
  {
    // assert: part_size < sample_size
    size_t delta = sample_size - part_size;
    if (size < delta)
    {
      // not enough data to fill sample buffer
      memcpy(part_buf + part_size, rawdata, size);
      part_size += size;
      size = 0;
      return;
    }
    else
    {
      // finish & convert incomplete sample 
      memcpy(part_buf + part_size, rawdata, delta);
      drop_rawdata(delta);
      part_size = 0;

      src = (int32_t *)part_buf;
    dst[nsamples * 0] = int2be32(src[0]);
    dst[nsamples * 1] = int2be32(src[1]);
    dst[nsamples * 2] = int2be32(src[2]);
    dst[nsamples * 3] = int2be32(src[3]);

      dst++;
      out_size++;
    }   
  }

  /////////////////////////////////////////////////////////
  // Set processing buffer start & end and remember 
  // remaining part of sample

  src = (int32_t *)rawdata;

  // integral number of samples (and its size in bytes)
  size_t n = nsamples - out_size;
  size_t n_size = n * sample_size;

  if (n_size < size)
  {
    drop_rawdata(n_size);
    end = (int32_t *)(((uint8_t *)src) + n_size);
    out_size += n;
  }
  else
  {
    n = size / sample_size;
    n_size = n * sample_size;

    drop_rawdata(n_size);
    end = (int32_t *)(((uint8_t *)src) + n_size);
    out_size += n;

    // remember part of sample
    if (size)
    {
      memcpy(part_buf, end, size);
      part_size = size;
      size = 0;
    }
  }

  /////////////////////////////////////////////////////////
  // Convert

  while (src < end)
  {
    dst[nsamples * 0] = int2be32(src[0]);
    dst[nsamples * 1] = int2be32(src[1]);
    dst[nsamples * 2] = int2be32(src[2]);
    dst[nsamples * 3] = int2be32(src[3]);

    src += nch;
    dst++;
  }
}
void
Converter::pcmfloat_linear_4ch()
{
  // input: spk, nsamples, rawdata, size
  // output: out_samples, out_size
  // intermediate: part_buf, part_size

  const int nch = 4;
  const size_t sample_size = sizeof(float) * 4;

  float *src;
  float *end;
  sample_t *dst = out_samples[0];
  out_size = 0;

  /////////////////////////////////////////////////////////
  // Process part of sample

  if (part_size)
  {
    // assert: part_size < sample_size
    size_t delta = sample_size - part_size;
    if (size < delta)
    {
      // not enough data to fill sample buffer
      memcpy(part_buf + part_size, rawdata, size);
      part_size += size;
      size = 0;
      return;
    }
    else
    {
      // finish & convert incomplete sample 
      memcpy(part_buf + part_size, rawdata, delta);
      drop_rawdata(delta);
      part_size = 0;

      src = (float *)part_buf;
    dst[nsamples * 0] = (src[0]);
    dst[nsamples * 1] = (src[1]);
    dst[nsamples * 2] = (src[2]);
    dst[nsamples * 3] = (src[3]);

      dst++;
      out_size++;
    }   
  }

  /////////////////////////////////////////////////////////
  // Set processing buffer start & end and remember 
  // remaining part of sample

  src = (float *)rawdata;

  // integral number of samples (and its size in bytes)
  size_t n = nsamples - out_size;
  size_t n_size = n * sample_size;

  if (n_size < size)
  {
    drop_rawdata(n_size);
    end = (float *)(((uint8_t *)src) + n_size);
    out_size += n;
  }
  else
  {
    n = size / sample_size;
    n_size = n * sample_size;

    drop_rawdata(n_size);
    end = (float *)(((uint8_t *)src) + n_size);
    out_size += n;

    // remember part of sample
    if (size)
    {
      memcpy(part_buf, end, size);
      part_size = size;
      size = 0;
    }
  }

  /////////////////////////////////////////////////////////
  // Convert

  while (src < end)
  {
    dst[nsamples * 0] = (src[0]);
    dst[nsamples * 1] = (src[1]);
    dst[nsamples * 2] = (src[2]);
    dst[nsamples * 3] = (src[3]);

    src += nch;
    dst++;
  }
}

void
Converter::pcm16_linear_5ch()
{
  // input: spk, nsamples, rawdata, size
  // output: out_samples, out_size
  // intermediate: part_buf, part_size

  const int nch = 5;
  const size_t sample_size = sizeof(int16_t) * 5;

  int16_t *src;
  int16_t *end;
  sample_t *dst = out_samples[0];
  out_size = 0;

  /////////////////////////////////////////////////////////
  // Process part of sample

  if (part_size)
  {
    // assert: part_size < sample_size
    size_t delta = sample_size - part_size;
    if (size < delta)
    {
      // not enough data to fill sample buffer
      memcpy(part_buf + part_size, rawdata, size);
      part_size += size;
      size = 0;
      return;
    }
    else
    {
      // finish & convert incomplete sample 
      memcpy(part_buf + part_size, rawdata, delta);
      drop_rawdata(delta);
      part_size = 0;

      src = (int16_t *)part_buf;
    dst[nsamples * 0] = int2le16(src[0]);
    dst[nsamples * 1] = int2le16(src[1]);
    dst[nsamples * 2] = int2le16(src[2]);
    dst[nsamples * 3] = int2le16(src[3]);
    dst[nsamples * 4] = int2le16(src[4]);

      dst++;
      out_size++;
    }   
  }

  /////////////////////////////////////////////////////////
  // Set processing buffer start & end and remember 
  // remaining part of sample

  src = (int16_t *)rawdata;

  // integral number of samples (and its size in bytes)
  size_t n = nsamples - out_size;
  size_t n_size = n * sample_size;

  if (n_size < size)
  {
    drop_rawdata(n_size);
    end = (int16_t *)(((uint8_t *)src) + n_size);
    out_size += n;
  }
  else
  {
    n = size / sample_size;
    n_size = n * sample_size;

    drop_rawdata(n_size);
    end = (int16_t *)(((uint8_t *)src) + n_size);
    out_size += n;

    // remember part of sample
    if (size)
    {
      memcpy(part_buf, end, size);
      part_size = size;
      size = 0;
    }
  }

  /////////////////////////////////////////////////////////
  // Convert

  while (src < end)
  {
    dst[nsamples * 0] = int2le16(src[0]);
    dst[nsamples * 1] = int2le16(src[1]);
    dst[nsamples * 2] = int2le16(src[2]);
    dst[nsamples * 3] = int2le16(src[3]);
    dst[nsamples * 4] = int2le16(src[4]);

    src += nch;
    dst++;
  }
}
void
Converter::pcm24_linear_5ch()
{
  // input: spk, nsamples, rawdata, size
  // output: out_samples, out_size
  // intermediate: part_buf, part_size

  const int nch = 5;
  const size_t sample_size = sizeof(int24_t) * 5;

  int24_t *src;
  int24_t *end;
  sample_t *dst = out_samples[0];
  out_size = 0;

  /////////////////////////////////////////////////////////
  // Process part of sample

  if (part_size)
  {
    // assert: part_size < sample_size
    size_t delta = sample_size - part_size;
    if (size < delta)
    {
      // not enough data to fill sample buffer
      memcpy(part_buf + part_size, rawdata, size);
      part_size += size;
      size = 0;
      return;
    }
    else
    {
      // finish & convert incomplete sample 
      memcpy(part_buf + part_size, rawdata, delta);
      drop_rawdata(delta);
      part_size = 0;

      src = (int24_t *)part_buf;
    dst[nsamples * 0] = int2le24(src[0]);
    dst[nsamples * 1] = int2le24(src[1]);
    dst[nsamples * 2] = int2le24(src[2]);
    dst[nsamples * 3] = int2le24(src[3]);
    dst[nsamples * 4] = int2le24(src[4]);

      dst++;
      out_size++;
    }   
  }

  /////////////////////////////////////////////////////////
  // Set processing buffer start & end and remember 
  // remaining part of sample

  src = (int24_t *)rawdata;

  // integral number of samples (and its size in bytes)
  size_t n = nsamples - out_size;
  size_t n_size = n * sample_size;

  if (n_size < size)
  {
    drop_rawdata(n_size);
    end = (int24_t *)(((uint8_t *)src) + n_size);
    out_size += n;
  }
  else
  {
    n = size / sample_size;
    n_size = n * sample_size;

    drop_rawdata(n_size);
    end = (int24_t *)(((uint8_t *)src) + n_size);
    out_size += n;

    // remember part of sample
    if (size)
    {
      memcpy(part_buf, end, size);
      part_size = size;
      size = 0;
    }
  }

  /////////////////////////////////////////////////////////
  // Convert

  while (src < end)
  {
    dst[nsamples * 0] = int2le24(src[0]);
    dst[nsamples * 1] = int2le24(src[1]);
    dst[nsamples * 2] = int2le24(src[2]);
    dst[nsamples * 3] = int2le24(src[3]);
    dst[nsamples * 4] = int2le24(src[4]);

    src += nch;
    dst++;
  }
}
void
Converter::pcm32_linear_5ch()
{
  // input: spk, nsamples, rawdata, size
  // output: out_samples, out_size
  // intermediate: part_buf, part_size

  const int nch = 5;
  const size_t sample_size = sizeof(int32_t) * 5;

  int32_t *src;
  int32_t *end;
  sample_t *dst = out_samples[0];
  out_size = 0;

  /////////////////////////////////////////////////////////
  // Process part of sample

  if (part_size)
  {
    // assert: part_size < sample_size
    size_t delta = sample_size - part_size;
    if (size < delta)
    {
      // not enough data to fill sample buffer
      memcpy(part_buf + part_size, rawdata, size);
      part_size += size;
      size = 0;
      return;
    }
    else
    {
      // finish & convert incomplete sample 
      memcpy(part_buf + part_size, rawdata, delta);
      drop_rawdata(delta);
      part_size = 0;

      src = (int32_t *)part_buf;
    dst[nsamples * 0] = int2le32(src[0]);
    dst[nsamples * 1] = int2le32(src[1]);
    dst[nsamples * 2] = int2le32(src[2]);
    dst[nsamples * 3] = int2le32(src[3]);
    dst[nsamples * 4] = int2le32(src[4]);

      dst++;
      out_size++;
    }   
  }

  /////////////////////////////////////////////////////////
  // Set processing buffer start & end and remember 
  // remaining part of sample

  src = (int32_t *)rawdata;

  // integral number of samples (and its size in bytes)
  size_t n = nsamples - out_size;
  size_t n_size = n * sample_size;

  if (n_size < size)
  {
    drop_rawdata(n_size);
    end = (int32_t *)(((uint8_t *)src) + n_size);
    out_size += n;
  }
  else
  {
    n = size / sample_size;
    n_size = n * sample_size;

    drop_rawdata(n_size);
    end = (int32_t *)(((uint8_t *)src) + n_size);
    out_size += n;

    // remember part of sample
    if (size)
    {
      memcpy(part_buf, end, size);
      part_size = size;
      size = 0;
    }
  }

  /////////////////////////////////////////////////////////
  // Convert

  while (src < end)
  {
    dst[nsamples * 0] = int2le32(src[0]);
    dst[nsamples * 1] = int2le32(src[1]);
    dst[nsamples * 2] = int2le32(src[2]);
    dst[nsamples * 3] = int2le32(src[3]);
    dst[nsamples * 4] = int2le32(src[4]);

    src += nch;
    dst++;
  }
}
void
Converter::pcm16_be_linear_5ch()
{
  // input: spk, nsamples, rawdata, size
  // output: out_samples, out_size
  // intermediate: part_buf, part_size

  const int nch = 5;
  const size_t sample_size = sizeof(int16_t) * 5;

  int16_t *src;
  int16_t *end;
  sample_t *dst = out_samples[0];
  out_size = 0;

  /////////////////////////////////////////////////////////
  // Process part of sample

  if (part_size)
  {
    // assert: part_size < sample_size
    size_t delta = sample_size - part_size;
    if (size < delta)
    {
      // not enough data to fill sample buffer
      memcpy(part_buf + part_size, rawdata, size);
      part_size += size;
      size = 0;
      return;
    }
    else
    {
      // finish & convert incomplete sample 
      memcpy(part_buf + part_size, rawdata, delta);
      drop_rawdata(delta);
      part_size = 0;

      src = (int16_t *)part_buf;
    dst[nsamples * 0] = int2be16(src[0]);
    dst[nsamples * 1] = int2be16(src[1]);
    dst[nsamples * 2] = int2be16(src[2]);
    dst[nsamples * 3] = int2be16(src[3]);
    dst[nsamples * 4] = int2be16(src[4]);

      dst++;
      out_size++;
    }   
  }

  /////////////////////////////////////////////////////////
  // Set processing buffer start & end and remember 
  // remaining part of sample

  src = (int16_t *)rawdata;

  // integral number of samples (and its size in bytes)
  size_t n = nsamples - out_size;
  size_t n_size = n * sample_size;

  if (n_size < size)
  {
    drop_rawdata(n_size);
    end = (int16_t *)(((uint8_t *)src) + n_size);
    out_size += n;
  }
  else
  {
    n = size / sample_size;
    n_size = n * sample_size;

    drop_rawdata(n_size);
    end = (int16_t *)(((uint8_t *)src) + n_size);
    out_size += n;

    // remember part of sample
    if (size)
    {
      memcpy(part_buf, end, size);
      part_size = size;
      size = 0;
    }
  }

  /////////////////////////////////////////////////////////
  // Convert

  while (src < end)
  {
    dst[nsamples * 0] = int2be16(src[0]);
    dst[nsamples * 1] = int2be16(src[1]);
    dst[nsamples * 2] = int2be16(src[2]);
    dst[nsamples * 3] = int2be16(src[3]);
    dst[nsamples * 4] = int2be16(src[4]);

    src += nch;
    dst++;
  }
}
void
Converter::pcm24_be_linear_5ch()
{
  // input: spk, nsamples, rawdata, size
  // output: out_samples, out_size
  // intermediate: part_buf, part_size

  const int nch = 5;
  const size_t sample_size = sizeof(int24_t) * 5;

  int24_t *src;
  int24_t *end;
  sample_t *dst = out_samples[0];
  out_size = 0;

  /////////////////////////////////////////////////////////
  // Process part of sample

  if (part_size)
  {
    // assert: part_size < sample_size
    size_t delta = sample_size - part_size;
    if (size < delta)
    {
      // not enough data to fill sample buffer
      memcpy(part_buf + part_size, rawdata, size);
      part_size += size;
      size = 0;
      return;
    }
    else
    {
      // finish & convert incomplete sample 
      memcpy(part_buf + part_size, rawdata, delta);
      drop_rawdata(delta);
      part_size = 0;

      src = (int24_t *)part_buf;
    dst[nsamples * 0] = int2be24(src[0]);
    dst[nsamples * 1] = int2be24(src[1]);
    dst[nsamples * 2] = int2be24(src[2]);
    dst[nsamples * 3] = int2be24(src[3]);
    dst[nsamples * 4] = int2be24(src[4]);

      dst++;
      out_size++;
    }   
  }

  /////////////////////////////////////////////////////////
  // Set processing buffer start & end and remember 
  // remaining part of sample

  src = (int24_t *)rawdata;

  // integral number of samples (and its size in bytes)
  size_t n = nsamples - out_size;
  size_t n_size = n * sample_size;

  if (n_size < size)
  {
    drop_rawdata(n_size);
    end = (int24_t *)(((uint8_t *)src) + n_size);
    out_size += n;
  }
  else
  {
    n = size / sample_size;
    n_size = n * sample_size;

    drop_rawdata(n_size);
    end = (int24_t *)(((uint8_t *)src) + n_size);
    out_size += n;

    // remember part of sample
    if (size)
    {
      memcpy(part_buf, end, size);
      part_size = size;
      size = 0;
    }
  }

  /////////////////////////////////////////////////////////
  // Convert

  while (src < end)
  {
    dst[nsamples * 0] = int2be24(src[0]);
    dst[nsamples * 1] = int2be24(src[1]);
    dst[nsamples * 2] = int2be24(src[2]);
    dst[nsamples * 3] = int2be24(src[3]);
    dst[nsamples * 4] = int2be24(src[4]);

    src += nch;
    dst++;
  }
}
void
Converter::pcm32_be_linear_5ch()
{
  // input: spk, nsamples, rawdata, size
  // output: out_samples, out_size
  // intermediate: part_buf, part_size

  const int nch = 5;
  const size_t sample_size = sizeof(int32_t) * 5;

  int32_t *src;
  int32_t *end;
  sample_t *dst = out_samples[0];
  out_size = 0;

  /////////////////////////////////////////////////////////
  // Process part of sample

  if (part_size)
  {
    // assert: part_size < sample_size
    size_t delta = sample_size - part_size;
    if (size < delta)
    {
      // not enough data to fill sample buffer
      memcpy(part_buf + part_size, rawdata, size);
      part_size += size;
      size = 0;
      return;
    }
    else
    {
      // finish & convert incomplete sample 
      memcpy(part_buf + part_size, rawdata, delta);
      drop_rawdata(delta);
      part_size = 0;

      src = (int32_t *)part_buf;
    dst[nsamples * 0] = int2be32(src[0]);
    dst[nsamples * 1] = int2be32(src[1]);
    dst[nsamples * 2] = int2be32(src[2]);
    dst[nsamples * 3] = int2be32(src[3]);
    dst[nsamples * 4] = int2be32(src[4]);

      dst++;
      out_size++;
    }   
  }

  /////////////////////////////////////////////////////////
  // Set processing buffer start & end and remember 
  // remaining part of sample

  src = (int32_t *)rawdata;

  // integral number of samples (and its size in bytes)
  size_t n = nsamples - out_size;
  size_t n_size = n * sample_size;

  if (n_size < size)
  {
    drop_rawdata(n_size);
    end = (int32_t *)(((uint8_t *)src) + n_size);
    out_size += n;
  }
  else
  {
    n = size / sample_size;
    n_size = n * sample_size;

    drop_rawdata(n_size);
    end = (int32_t *)(((uint8_t *)src) + n_size);
    out_size += n;

    // remember part of sample
    if (size)
    {
      memcpy(part_buf, end, size);
      part_size = size;
      size = 0;
    }
  }

  /////////////////////////////////////////////////////////
  // Convert

  while (src < end)
  {
    dst[nsamples * 0] = int2be32(src[0]);
    dst[nsamples * 1] = int2be32(src[1]);
    dst[nsamples * 2] = int2be32(src[2]);
    dst[nsamples * 3] = int2be32(src[3]);
    dst[nsamples * 4] = int2be32(src[4]);

    src += nch;
    dst++;
  }
}
void
Converter::pcmfloat_linear_5ch()
{
  // input: spk, nsamples, rawdata, size
  // output: out_samples, out_size
  // intermediate: part_buf, part_size

  const int nch = 5;
  const size_t sample_size = sizeof(float) * 5;

  float *src;
  float *end;
  sample_t *dst = out_samples[0];
  out_size = 0;

  /////////////////////////////////////////////////////////
  // Process part of sample

  if (part_size)
  {
    // assert: part_size < sample_size
    size_t delta = sample_size - part_size;
    if (size < delta)
    {
      // not enough data to fill sample buffer
      memcpy(part_buf + part_size, rawdata, size);
      part_size += size;
      size = 0;
      return;
    }
    else
    {
      // finish & convert incomplete sample 
      memcpy(part_buf + part_size, rawdata, delta);
      drop_rawdata(delta);
      part_size = 0;

      src = (float *)part_buf;
    dst[nsamples * 0] = (src[0]);
    dst[nsamples * 1] = (src[1]);
    dst[nsamples * 2] = (src[2]);
    dst[nsamples * 3] = (src[3]);
    dst[nsamples * 4] = (src[4]);

      dst++;
      out_size++;
    }   
  }

  /////////////////////////////////////////////////////////
  // Set processing buffer start & end and remember 
  // remaining part of sample

  src = (float *)rawdata;

  // integral number of samples (and its size in bytes)
  size_t n = nsamples - out_size;
  size_t n_size = n * sample_size;

  if (n_size < size)
  {
    drop_rawdata(n_size);
    end = (float *)(((uint8_t *)src) + n_size);
    out_size += n;
  }
  else
  {
    n = size / sample_size;
    n_size = n * sample_size;

    drop_rawdata(n_size);
    end = (float *)(((uint8_t *)src) + n_size);
    out_size += n;

    // remember part of sample
    if (size)
    {
      memcpy(part_buf, end, size);
      part_size = size;
      size = 0;
    }
  }

  /////////////////////////////////////////////////////////
  // Convert

  while (src < end)
  {
    dst[nsamples * 0] = (src[0]);
    dst[nsamples * 1] = (src[1]);
    dst[nsamples * 2] = (src[2]);
    dst[nsamples * 3] = (src[3]);
    dst[nsamples * 4] = (src[4]);

    src += nch;
    dst++;
  }
}

void
Converter::pcm16_linear_6ch()
{
  // input: spk, nsamples, rawdata, size
  // output: out_samples, out_size
  // intermediate: part_buf, part_size

  const int nch = 6;
  const size_t sample_size = sizeof(int16_t) * 6;

  int16_t *src;
  int16_t *end;
  sample_t *dst = out_samples[0];
  out_size = 0;

  /////////////////////////////////////////////////////////
  // Process part of sample

  if (part_size)
  {
    // assert: part_size < sample_size
    size_t delta = sample_size - part_size;
    if (size < delta)
    {
      // not enough data to fill sample buffer
      memcpy(part_buf + part_size, rawdata, size);
      part_size += size;
      size = 0;
      return;
    }
    else
    {
      // finish & convert incomplete sample 
      memcpy(part_buf + part_size, rawdata, delta);
      drop_rawdata(delta);
      part_size = 0;

      src = (int16_t *)part_buf;
    dst[nsamples * 0] = int2le16(src[0]);
    dst[nsamples * 1] = int2le16(src[1]);
    dst[nsamples * 2] = int2le16(src[2]);
    dst[nsamples * 3] = int2le16(src[3]);
    dst[nsamples * 4] = int2le16(src[4]);
    dst[nsamples * 5] = int2le16(src[5]);

      dst++;
      out_size++;
    }   
  }

  /////////////////////////////////////////////////////////
  // Set processing buffer start & end and remember 
  // remaining part of sample

  src = (int16_t *)rawdata;

  // integral number of samples (and its size in bytes)
  size_t n = nsamples - out_size;
  size_t n_size = n * sample_size;

  if (n_size < size)
  {
    drop_rawdata(n_size);
    end = (int16_t *)(((uint8_t *)src) + n_size);
    out_size += n;
  }
  else
  {
    n = size / sample_size;
    n_size = n * sample_size;

    drop_rawdata(n_size);
    end = (int16_t *)(((uint8_t *)src) + n_size);
    out_size += n;

    // remember part of sample
    if (size)
    {
      memcpy(part_buf, end, size);
      part_size = size;
      size = 0;
    }
  }

  /////////////////////////////////////////////////////////
  // Convert

  while (src < end)
  {
    dst[nsamples * 0] = int2le16(src[0]);
    dst[nsamples * 1] = int2le16(src[1]);
    dst[nsamples * 2] = int2le16(src[2]);
    dst[nsamples * 3] = int2le16(src[3]);
    dst[nsamples * 4] = int2le16(src[4]);
    dst[nsamples * 5] = int2le16(src[5]);

    src += nch;
    dst++;
  }
}
void
Converter::pcm24_linear_6ch()
{
  // input: spk, nsamples, rawdata, size
  // output: out_samples, out_size
  // intermediate: part_buf, part_size

  const int nch = 6;
  const size_t sample_size = sizeof(int24_t) * 6;

  int24_t *src;
  int24_t *end;
  sample_t *dst = out_samples[0];
  out_size = 0;

  /////////////////////////////////////////////////////////
  // Process part of sample

  if (part_size)
  {
    // assert: part_size < sample_size
    size_t delta = sample_size - part_size;
    if (size < delta)
    {
      // not enough data to fill sample buffer
      memcpy(part_buf + part_size, rawdata, size);
      part_size += size;
      size = 0;
      return;
    }
    else
    {
      // finish & convert incomplete sample 
      memcpy(part_buf + part_size, rawdata, delta);
      drop_rawdata(delta);
      part_size = 0;

      src = (int24_t *)part_buf;
    dst[nsamples * 0] = int2le24(src[0]);
    dst[nsamples * 1] = int2le24(src[1]);
    dst[nsamples * 2] = int2le24(src[2]);
    dst[nsamples * 3] = int2le24(src[3]);
    dst[nsamples * 4] = int2le24(src[4]);
    dst[nsamples * 5] = int2le24(src[5]);

      dst++;
      out_size++;
    }   
  }

  /////////////////////////////////////////////////////////
  // Set processing buffer start & end and remember 
  // remaining part of sample

  src = (int24_t *)rawdata;

  // integral number of samples (and its size in bytes)
  size_t n = nsamples - out_size;
  size_t n_size = n * sample_size;

  if (n_size < size)
  {
    drop_rawdata(n_size);
    end = (int24_t *)(((uint8_t *)src) + n_size);
    out_size += n;
  }
  else
  {
    n = size / sample_size;
    n_size = n * sample_size;

    drop_rawdata(n_size);
    end = (int24_t *)(((uint8_t *)src) + n_size);
    out_size += n;

    // remember part of sample
    if (size)
    {
      memcpy(part_buf, end, size);
      part_size = size;
      size = 0;
    }
  }

  /////////////////////////////////////////////////////////
  // Convert

  while (src < end)
  {
    dst[nsamples * 0] = int2le24(src[0]);
    dst[nsamples * 1] = int2le24(src[1]);
    dst[nsamples * 2] = int2le24(src[2]);
    dst[nsamples * 3] = int2le24(src[3]);
    dst[nsamples * 4] = int2le24(src[4]);
    dst[nsamples * 5] = int2le24(src[5]);

    src += nch;
    dst++;
  }
}
void
Converter::pcm32_linear_6ch()
{
  // input: spk, nsamples, rawdata, size
  // output: out_samples, out_size
  // intermediate: part_buf, part_size

  const int nch = 6;
  const size_t sample_size = sizeof(int32_t) * 6;

  int32_t *src;
  int32_t *end;
  sample_t *dst = out_samples[0];
  out_size = 0;

  /////////////////////////////////////////////////////////
  // Process part of sample

  if (part_size)
  {
    // assert: part_size < sample_size
    size_t delta = sample_size - part_size;
    if (size < delta)
    {
      // not enough data to fill sample buffer
      memcpy(part_buf + part_size, rawdata, size);
      part_size += size;
      size = 0;
      return;
    }
    else
    {
      // finish & convert incomplete sample 
      memcpy(part_buf + part_size, rawdata, delta);
      drop_rawdata(delta);
      part_size = 0;

      src = (int32_t *)part_buf;
    dst[nsamples * 0] = int2le32(src[0]);
    dst[nsamples * 1] = int2le32(src[1]);
    dst[nsamples * 2] = int2le32(src[2]);
    dst[nsamples * 3] = int2le32(src[3]);
    dst[nsamples * 4] = int2le32(src[4]);
    dst[nsamples * 5] = int2le32(src[5]);

      dst++;
      out_size++;
    }   
  }

  /////////////////////////////////////////////////////////
  // Set processing buffer start & end and remember 
  // remaining part of sample

  src = (int32_t *)rawdata;

  // integral number of samples (and its size in bytes)
  size_t n = nsamples - out_size;
  size_t n_size = n * sample_size;

  if (n_size < size)
  {
    drop_rawdata(n_size);
    end = (int32_t *)(((uint8_t *)src) + n_size);
    out_size += n;
  }
  else
  {
    n = size / sample_size;
    n_size = n * sample_size;

    drop_rawdata(n_size);
    end = (int32_t *)(((uint8_t *)src) + n_size);
    out_size += n;

    // remember part of sample
    if (size)
    {
      memcpy(part_buf, end, size);
      part_size = size;
      size = 0;
    }
  }

  /////////////////////////////////////////////////////////
  // Convert

  while (src < end)
  {
    dst[nsamples * 0] = int2le32(src[0]);
    dst[nsamples * 1] = int2le32(src[1]);
    dst[nsamples * 2] = int2le32(src[2]);
    dst[nsamples * 3] = int2le32(src[3]);
    dst[nsamples * 4] = int2le32(src[4]);
    dst[nsamples * 5] = int2le32(src[5]);

    src += nch;
    dst++;
  }
}
void
Converter::pcm16_be_linear_6ch()
{
  // input: spk, nsamples, rawdata, size
  // output: out_samples, out_size
  // intermediate: part_buf, part_size

  const int nch = 6;
  const size_t sample_size = sizeof(int16_t) * 6;

  int16_t *src;
  int16_t *end;
  sample_t *dst = out_samples[0];
  out_size = 0;

  /////////////////////////////////////////////////////////
  // Process part of sample

  if (part_size)
  {
    // assert: part_size < sample_size
    size_t delta = sample_size - part_size;
    if (size < delta)
    {
      // not enough data to fill sample buffer
      memcpy(part_buf + part_size, rawdata, size);
      part_size += size;
      size = 0;
      return;
    }
    else
    {
      // finish & convert incomplete sample 
      memcpy(part_buf + part_size, rawdata, delta);
      drop_rawdata(delta);
      part_size = 0;

      src = (int16_t *)part_buf;
    dst[nsamples * 0] = int2be16(src[0]);
    dst[nsamples * 1] = int2be16(src[1]);
    dst[nsamples * 2] = int2be16(src[2]);
    dst[nsamples * 3] = int2be16(src[3]);
    dst[nsamples * 4] = int2be16(src[4]);
    dst[nsamples * 5] = int2be16(src[5]);

      dst++;
      out_size++;
    }   
  }

  /////////////////////////////////////////////////////////
  // Set processing buffer start & end and remember 
  // remaining part of sample

  src = (int16_t *)rawdata;

  // integral number of samples (and its size in bytes)
  size_t n = nsamples - out_size;
  size_t n_size = n * sample_size;

  if (n_size < size)
  {
    drop_rawdata(n_size);
    end = (int16_t *)(((uint8_t *)src) + n_size);
    out_size += n;
  }
  else
  {
    n = size / sample_size;
    n_size = n * sample_size;

    drop_rawdata(n_size);
    end = (int16_t *)(((uint8_t *)src) + n_size);
    out_size += n;

    // remember part of sample
    if (size)
    {
      memcpy(part_buf, end, size);
      part_size = size;
      size = 0;
    }
  }

  /////////////////////////////////////////////////////////
  // Convert

  while (src < end)
  {
    dst[nsamples * 0] = int2be16(src[0]);
    dst[nsamples * 1] = int2be16(src[1]);
    dst[nsamples * 2] = int2be16(src[2]);
    dst[nsamples * 3] = int2be16(src[3]);
    dst[nsamples * 4] = int2be16(src[4]);
    dst[nsamples * 5] = int2be16(src[5]);

    src += nch;
    dst++;
  }
}
void
Converter::pcm24_be_linear_6ch()
{
  // input: spk, nsamples, rawdata, size
  // output: out_samples, out_size
  // intermediate: part_buf, part_size

  const int nch = 6;
  const size_t sample_size = sizeof(int24_t) * 6;

  int24_t *src;
  int24_t *end;
  sample_t *dst = out_samples[0];
  out_size = 0;

  /////////////////////////////////////////////////////////
  // Process part of sample

  if (part_size)
  {
    // assert: part_size < sample_size
    size_t delta = sample_size - part_size;
    if (size < delta)
    {
      // not enough data to fill sample buffer
      memcpy(part_buf + part_size, rawdata, size);
      part_size += size;
      size = 0;
      return;
    }
    else
    {
      // finish & convert incomplete sample 
      memcpy(part_buf + part_size, rawdata, delta);
      drop_rawdata(delta);
      part_size = 0;

      src = (int24_t *)part_buf;
    dst[nsamples * 0] = int2be24(src[0]);
    dst[nsamples * 1] = int2be24(src[1]);
    dst[nsamples * 2] = int2be24(src[2]);
    dst[nsamples * 3] = int2be24(src[3]);
    dst[nsamples * 4] = int2be24(src[4]);
    dst[nsamples * 5] = int2be24(src[5]);

      dst++;
      out_size++;
    }   
  }

  /////////////////////////////////////////////////////////
  // Set processing buffer start & end and remember 
  // remaining part of sample

  src = (int24_t *)rawdata;

  // integral number of samples (and its size in bytes)
  size_t n = nsamples - out_size;
  size_t n_size = n * sample_size;

  if (n_size < size)
  {
    drop_rawdata(n_size);
    end = (int24_t *)(((uint8_t *)src) + n_size);
    out_size += n;
  }
  else
  {
    n = size / sample_size;
    n_size = n * sample_size;

    drop_rawdata(n_size);
    end = (int24_t *)(((uint8_t *)src) + n_size);
    out_size += n;

    // remember part of sample
    if (size)
    {
      memcpy(part_buf, end, size);
      part_size = size;
      size = 0;
    }
  }

  /////////////////////////////////////////////////////////
  // Convert

  while (src < end)
  {
    dst[nsamples * 0] = int2be24(src[0]);
    dst[nsamples * 1] = int2be24(src[1]);
    dst[nsamples * 2] = int2be24(src[2]);
    dst[nsamples * 3] = int2be24(src[3]);
    dst[nsamples * 4] = int2be24(src[4]);
    dst[nsamples * 5] = int2be24(src[5]);

    src += nch;
    dst++;
  }
}
void
Converter::pcm32_be_linear_6ch()
{
  // input: spk, nsamples, rawdata, size
  // output: out_samples, out_size
  // intermediate: part_buf, part_size

  const int nch = 6;
  const size_t sample_size = sizeof(int32_t) * 6;

  int32_t *src;
  int32_t *end;
  sample_t *dst = out_samples[0];
  out_size = 0;

  /////////////////////////////////////////////////////////
  // Process part of sample

  if (part_size)
  {
    // assert: part_size < sample_size
    size_t delta = sample_size - part_size;
    if (size < delta)
    {
      // not enough data to fill sample buffer
      memcpy(part_buf + part_size, rawdata, size);
      part_size += size;
      size = 0;
      return;
    }
    else
    {
      // finish & convert incomplete sample 
      memcpy(part_buf + part_size, rawdata, delta);
      drop_rawdata(delta);
      part_size = 0;

      src = (int32_t *)part_buf;
    dst[nsamples * 0] = int2be32(src[0]);
    dst[nsamples * 1] = int2be32(src[1]);
    dst[nsamples * 2] = int2be32(src[2]);
    dst[nsamples * 3] = int2be32(src[3]);
    dst[nsamples * 4] = int2be32(src[4]);
    dst[nsamples * 5] = int2be32(src[5]);

      dst++;
      out_size++;
    }   
  }

  /////////////////////////////////////////////////////////
  // Set processing buffer start & end and remember 
  // remaining part of sample

  src = (int32_t *)rawdata;

  // integral number of samples (and its size in bytes)
  size_t n = nsamples - out_size;
  size_t n_size = n * sample_size;

  if (n_size < size)
  {
    drop_rawdata(n_size);
    end = (int32_t *)(((uint8_t *)src) + n_size);
    out_size += n;
  }
  else
  {
    n = size / sample_size;
    n_size = n * sample_size;

    drop_rawdata(n_size);
    end = (int32_t *)(((uint8_t *)src) + n_size);
    out_size += n;

    // remember part of sample
    if (size)
    {
      memcpy(part_buf, end, size);
      part_size = size;
      size = 0;
    }
  }

  /////////////////////////////////////////////////////////
  // Convert

  while (src < end)
  {
    dst[nsamples * 0] = int2be32(src[0]);
    dst[nsamples * 1] = int2be32(src[1]);
    dst[nsamples * 2] = int2be32(src[2]);
    dst[nsamples * 3] = int2be32(src[3]);
    dst[nsamples * 4] = int2be32(src[4]);
    dst[nsamples * 5] = int2be32(src[5]);

    src += nch;
    dst++;
  }
}
void
Converter::pcmfloat_linear_6ch()
{
  // input: spk, nsamples, rawdata, size
  // output: out_samples, out_size
  // intermediate: part_buf, part_size

  const int nch = 6;
  const size_t sample_size = sizeof(float) * 6;

  float *src;
  float *end;
  sample_t *dst = out_samples[0];
  out_size = 0;

  /////////////////////////////////////////////////////////
  // Process part of sample

  if (part_size)
  {
    // assert: part_size < sample_size
    size_t delta = sample_size - part_size;
    if (size < delta)
    {
      // not enough data to fill sample buffer
      memcpy(part_buf + part_size, rawdata, size);
      part_size += size;
      size = 0;
      return;
    }
    else
    {
      // finish & convert incomplete sample 
      memcpy(part_buf + part_size, rawdata, delta);
      drop_rawdata(delta);
      part_size = 0;

      src = (float *)part_buf;
    dst[nsamples * 0] = (src[0]);
    dst[nsamples * 1] = (src[1]);
    dst[nsamples * 2] = (src[2]);
    dst[nsamples * 3] = (src[3]);
    dst[nsamples * 4] = (src[4]);
    dst[nsamples * 5] = (src[5]);

      dst++;
      out_size++;
    }   
  }

  /////////////////////////////////////////////////////////
  // Set processing buffer start & end and remember 
  // remaining part of sample

  src = (float *)rawdata;

  // integral number of samples (and its size in bytes)
  size_t n = nsamples - out_size;
  size_t n_size = n * sample_size;

  if (n_size < size)
  {
    drop_rawdata(n_size);
    end = (float *)(((uint8_t *)src) + n_size);
    out_size += n;
  }
  else
  {
    n = size / sample_size;
    n_size = n * sample_size;

    drop_rawdata(n_size);
    end = (float *)(((uint8_t *)src) + n_size);
    out_size += n;

    // remember part of sample
    if (size)
    {
      memcpy(part_buf, end, size);
      part_size = size;
      size = 0;
    }
  }

  /////////////////////////////////////////////////////////
  // Convert

  while (src < end)
  {
    dst[nsamples * 0] = (src[0]);
    dst[nsamples * 1] = (src[1]);
    dst[nsamples * 2] = (src[2]);
    dst[nsamples * 3] = (src[3]);
    dst[nsamples * 4] = (src[4]);
    dst[nsamples * 5] = (src[5]);

    src += nch;
    dst++;
  }
}

