/*
  Data allocation
*/

#ifndef VALIB_DATA_H
#define VALIB_DATA_H

#include "auto_buf.h"
#include "spk.h"

class SampleBuf;


///////////////////////////////////////////////////////////////////////////////
// SampleBuf
// Buffer to store both rawdata and linear data.

class SampleBuf
{
protected:
  Rawdata   buf;

  size_t    nch;
  size_t    size;

  uint8_t  *rawdata;
  samples_t samples;

  inline void set_pointers()
  {
    samples[0] = (sample_t *)buf.data();

    size_t ch = 1;
    for (ch = 1; ch < nch; ch++)
      samples[ch] = samples.samples[ch-1] + size;

    while (ch < NCHANNELS)
      samples[ch++] = 0;
  }

public:
  // SampleBuf()
  // SampleBuf(int _size)

  // Raw data allocation
  // inline bool allocate(size_t _size)
  // inline bool truncate(size_t _size)

  // Linear buffer allocation
  // inline bool allocate(size_t nch, size_t _size)
  // inline bool truncate(size_t nch, size_t _size)

  // inline void drop()
  // inline void zero()

  // inline size_t    get_size() const
  // inline uint8_t  *get_rawdata() const
  // inline samples_t get_samples() const

  // inline operator uint8_t *() const
  // inline operator samples_t() const
  // inline sample_t *operator[](int _ch) const

  SampleBuf()
  {
    nch  = 0;
    size = 0;
    rawdata = 0;
    samples.zero();
  }

  ~SampleBuf()
  {
    drop();
  }

  inline bool allocate(size_t _nch, size_t _size)
  {
    if (!buf.allocate(_size * sizeof(sample_t) * _nch))
      return false;

    nch = _nch;
    size = _size;
    rawdata = 0;
    set_pointers();
    return true;
  }

  inline void drop()
  {
    buf.drop();

    nch  = 0;
    size = 0;
    rawdata = 0;
    samples.zero();
  }

  inline void zero()          
  { 
    buf.zero();
  }

  inline size_t    get_size() const              { return size;         }
  inline samples_t get_samples() const           { return samples;      }

  inline operator samples_t() const              { return samples;      }
  inline sample_t *operator[](int _ch) const     { return samples[_ch]; }
};

#endif
