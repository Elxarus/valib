/*
  PCM format conversions

  Input formats:   PCMxx, Linear
  Ouptupt formats: PCMxx, Linear
  Format conversions:
    Linear -> PCMxx
    PCMxx  -> Linear
  Timing: Preserve original
  Buffering: yes/no
*/

#ifndef CONVERT_H
#define CONVERT_H

#include "filter.h"

class Converter : public NullFilter
{
protected:
  // format
  int  format;             // format to convert to
  int  order[NCHANNELS];   // channel order to convert to when converting from linear format
                           // channel order to convert from when converting to linear format

  // converted samples buffer
  DataBuf buffer;          // big buffer for converted data
  size_t  nsamples;        // buffer size in samples

  // output data pointers
  uint8_t  *out_buf;       // buffer pointer for pcm data
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
  Converter(int _format = FORMAT_PCM16, int _nsamples = 2048)
  { 
    set_format(_format); 
    set_buffer(_nsamples);
    memcpy(order, std_order, sizeof(order));
    reset();
  }

  inline size_t get_buffer() const
  {
    return nsamples;
  }

  inline bool set_buffer(size_t _nsamples)
  {
    nsamples = _nsamples;
    return alloc_buffer();
  }

  inline int get_format() const
  {
    return format;
  }

  inline bool set_format(int _format)
  {
    if (!query_format(_format))
      return false;

    format = _format;
    return alloc_buffer();
  }

  inline void get_order(int _order[NCHANNELS]) const
  {
    memcpy(_order, order, sizeof(order));
  }

  inline void set_order(const int _order[NCHANNELS])
  {
    memcpy(order, _order, sizeof(order));
  }

  // Filter interface
  virtual void reset();

  virtual bool query_input(Speakers spk) const;
  virtual bool set_input(Speakers spk);

  virtual Speakers get_output() const;
  virtual bool get_chunk(Chunk *out);
};


#endif
