/*
  PCM format conversions

  Input formats:   PCMxx, Linear
  Ouptupt formats: PCMxx, Linear
  Format conversions:
    PCMxx  -> PCMxx
    Linear -> Linear
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
  DataBuf   buf;          // sample buffer
  samples_t samples;      // pointers to buf for linear fromat
  int nsamples;           // sample buffer size (in samples)

  uint8_t   part_buf[12]; // partial sample left from previous call
  int       part_size;    // partial sample size in bytes

  int format;             // format to convert to
  int order[NCHANNELS];   // channel order to convert to when converting from linear format;
                          // channel order to convert from when converting to linear format;
                          // mean nothing for pcm to pcm conversion and linear to linear

  bool query_format(int format) const;

  bool linear2pcm(Chunk *out);
  bool pcm2linear(Chunk *out);
  bool pcm2pcm(Chunk *out);

public:
  Converter(int _format = FORMAT_PCM16, int _nsamples = 2048)
  { 
    set_format(_format); 
    set_buffer(_nsamples);
    memcpy(order, std_order, sizeof(order));
    reset();
  }

  inline bool set_format(int format);

  inline void set_buffer(int nsamples = 2048);
  inline int  get_buffer();

  inline void set_order(const int order[NCHANNELS]);
  inline void get_order(int order[NCHANNELS]);

  // Filter interface
  virtual void reset();
  virtual bool query_input(Speakers spk) const;
  virtual bool set_input(Speakers spk);
  virtual Speakers get_output();
  virtual bool get_chunk(Chunk *out);
};


inline bool 
Converter::set_format(int _format)
{
  if (query_format(_format))
  {
    spk = Speakers();
    format = _format;
    return true;
  }
  else
    return false;
}

inline void
Converter::set_buffer(int _nsamples)
{
  spk = Speakers();
  nsamples = _nsamples;
}

inline int
Converter::get_buffer()
{
  return nsamples;
}

inline void
Converter::set_order(const int _order[NCHANNELS])
{
  memcpy(order, _order, sizeof(order));
}

inline void
Converter::get_order(int _order[NCHANNELS])
{
  memcpy(_order, order, sizeof(order));
}

#endif
