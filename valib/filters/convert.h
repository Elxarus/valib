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

#ifndef CONVERT_H
#define CONVERT_H

#include "filter.h"

///////////////////////////////////////////////////////////////////////////////
// Converter class
///////////////////////////////////////////////////////////////////////////////

class Converter : public NullFilter
{
protected:
  // format
  int  format;             // format to convert to
  int  order[NCHANNELS];   // channel order to convert to when converting from linear format
                           // channel order to convert from when converting to linear format

  // converted samples buffer
  DataBuf buf;             // buffer for converted data
  size_t nsamples;         // buffer size in samples

  // output data pointers
  uint8_t  *out_rawdata;   // buffer pointer for pcm data
  samples_t out_samples;   // buffer pointers for linear data
  size_t    out_size;      // buffer size in bytes/samples for pcm/linear data

  // part of sample from previous call
  uint8_t   part_buf[12];  // partial sample left from previous call
  size_t    part_size;     // partial sample size in bytes
           

  inline bool query_format(int format) const;
  bool alloc_buffer(); // allocate buffer according to format & number of channels

  void pcm2linear();
  void pcm2linear(uint8_t *src, samples_t dst, size_t n);
  void linear2pcm();
  void linear2pcm(samples_t src, uint8_t *dst, size_t n);

public:
  Converter(int _nsamples)
  {
    format = FORMAT_PCM16;
    nsamples = _nsamples;
    memcpy(order, std_order, sizeof(order));

    out_size = 0;
    part_size = 0;
  }

  /////////////////////////////////////////////////////////
  // Converter interface

  // buffer size
  inline size_t get_buffer() const;
  inline bool   set_buffer(size_t _nsamples);
  // output format
  inline int  get_format() const;
  inline bool set_format(int _format);
  // output channel order
  inline void get_order(int _order[NCHANNELS]) const;
  inline void set_order(const int _order[NCHANNELS]);

  /////////////////////////////////////////////////////////
  // Filter interface

  virtual void reset();

  virtual bool query_input(Speakers spk) const;
  virtual bool set_input(Speakers spk);

  virtual Speakers get_output() const;
  virtual bool get_chunk(Chunk *out);
};

///////////////////////////////////////////////////////////////////////////////
// Converter inlines
///////////////////////////////////////////////////////////////////////////////

inline bool 
Converter::query_format(int _format) const 
{
  return (FORMAT_MASK(_format) & (FORMAT_CLASS_PCM | FORMAT_MASK_LINEAR)) != 0;
}

inline size_t 
Converter::get_buffer() const
{
  return nsamples;
}

inline bool 
Converter::set_buffer(size_t _nsamples)
{
  nsamples = _nsamples;
  return alloc_buffer();
}

inline int 
Converter::get_format() const
{
  return format;
}

inline bool 
Converter::set_format(int _format)
{
  if (!query_format(_format))
    return false;

  format = _format;
  return alloc_buffer();
}

inline void 
Converter::get_order(int _order[NCHANNELS]) const
{
  memcpy(_order, order, sizeof(order));
}

inline void 
Converter::set_order(const int _order[NCHANNELS])
{
  memcpy(order, _order, sizeof(order));
}




#endif
