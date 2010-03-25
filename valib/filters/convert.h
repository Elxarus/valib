/*
  PCM format conversions

  Input formats:   PCMxx, Linear
  Ouptupt formats: PCMxx, Linear
  Format conversions:
    Linear -> PCMxx
    PCMxx  -> Linear
  Default format conversion:
    Linear -> PCM16
  Timing: Preserve original
  Buffering: yes/no
*/

#ifndef VALIB_CONVERT_H
#define VALIB_CONVERT_H

#include "../buffer.h"
#include "../filter2.h"
#include "convert_func.h"

///////////////////////////////////////////////////////////////////////////////
// Converter class
///////////////////////////////////////////////////////////////////////////////

class Converter : public SimpleFilter
{
protected:
  // conversion function pointer
  void (*convert)(uint8_t *, samples_t, size_t);

  // format
  int  format;             // format to convert to
                           // affected only by set_format() function
  order_t order;           // channel order to convert to when converting from linear format
                           // channel order to convert from when converting to linear format

  // converted samples buffer
  Rawdata   buf;           // buffer for converted data
  size_t    nsamples;      // buffer size in samples

  // output data pointers
  uint8_t  *out_rawdata;   // buffer pointer for pcm data
  samples_t out_samples;   // buffer pointers for linear data

  // part of sample from previous call
  uint8_t   part_buf[48];  // partial sample left from previous call
  size_t    part_size;     // partial sample size in bytes

  convert_t find_conversion(int _format, Speakers _spk) const;
  bool convert_pcm2linear(Chunk2 &in, Chunk2 &out);
  bool convert_linear2pcm(Chunk2 &in, Chunk2 &out);
  bool is_lpcm(int format) { return format == FORMAT_LPCM20 || format == FORMAT_LPCM24; }

public:
  Converter(size_t _nsamples);

  /////////////////////////////////////////////////////////
  // Converter interface

  // buffer size
  size_t get_buffer() const;
  bool   set_buffer(size_t _nsamples);
  // output format
  int  get_format() const;
  bool set_format(int _format);
  // output channel order
  void get_order(order_t _order) const;
  void set_order(const order_t _order);

  /////////////////////////////////////////////////////////
  // SimpleFilter overrides

  virtual bool can_open(Speakers spk) const;
  virtual bool init(Speakers spk);

  virtual bool process(Chunk2 &in, Chunk2 &out);
  virtual void reset();

  virtual Speakers get_output() const;
};

#endif
