/*
  Data structures
*/

#ifndef DATA_H
#define DATA_H

#include <string.h>
#include "spk.h"

class DataBuf;
class SampleBuf;

///////////////////////////////////////////////////////////////////////////////
// samples_t
// Block of pointer to sample buffers for each channel for linear format.

struct samples_t
{
  sample_t *samples[NCHANNELS];

  inline sample_t *&operator [](int ch)       { return samples[ch]; }
  inline sample_t  *operator [](int ch) const { return samples[ch]; }

  inline samples_t &operator +=(int n);
  inline samples_t &operator -=(int n);
  inline samples_t &set_null();

  inline void reorder_to_std(Speakers spk, const int order[NCHANNELS]);
  inline void reorder_from_std(Speakers spk, const int order[NCHANNELS]);
  inline void reorder(Speakers spk, const int input_order[NCHANNELS], const int output_order[NCHANNELS]);
};

///////////////////////////////////////////////////////////////////////////////
// DataBuf
// Method of allocating BIG data buffers.
// Now it's just simple aliged data block

class DataBuf
{
protected:
  uint8_t  *buf;
  uint8_t  *buf_aligned;

  size_t    size;
  size_t    allocated;

public:
  // DataBuf()

  // inline bool allocate(size_t _size)
  // inline bool truncate(size_t _size)

  // inline void drop()
  // inline void zero()

  // inline size_t    get_size() const
  // inline uint8_t  *get_data() const
  // inline uint8_t  *operator()() const

  DataBuf()
  {
    buf = 0;
    buf_aligned = 0;

    allocated = 0;
    size = 0;
  }

  ~DataBuf()
  {
    drop();
  }

  inline bool allocate(size_t _size)
  {
    if (allocated < _size)
      return truncate(_size);
    else
    {
      size = _size;
      return true;
    }
  }

  inline bool truncate(size_t _size)
  {
    drop();

    if (_size)
    {
      buf = new uint8_t[_size + 7];
      if (!buf) return false;
      buf_aligned = (uint8_t*)(unsigned(buf + 7) & ~7);

      allocated = _size;
      size = _size;
    }
    return true;
  }

  inline void drop()
  {
    if (buf) delete buf;

    buf = 0;
    buf_aligned = 0;

    allocated = 0;
    size = 0;
  }

  inline void zero()          
  { 
    if (buf) 
      memset(buf_aligned, 0, size);
  }

  inline size_t   get_size()   const { return size;        }
  inline uint8_t *get_data()   const { return buf_aligned; }
  inline operator uint8_t *()  const { return buf_aligned; }
};

///////////////////////////////////////////////////////////////////////////////
// SampleBuf
// Buffer to store both rawdata and linear data.

class SampleBuf
{
protected:
  DataBuf   buf;

  size_t    nch;
  size_t    size;

  uint8_t  *rawdata;
  samples_t samples;

  inline void set_pointers()
  {
    samples[0] = (sample_t *)buf.get_data();

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
    samples.set_null();
  }

  ~SampleBuf()
  {
    drop();
  }

  inline bool allocate(size_t _size)
  {
    if (!buf.allocate(_size))
      return false;

    nch = 0;
    size = _size;
    rawdata = buf;
    samples.set_null();
    return true;
  }

  inline bool truncate(size_t _size)
  {
    if (!buf.truncate(_size))
      return false;

    nch = 0;
    size = _size;
    rawdata = buf.get_data();
    samples.set_null();
    return true;
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

  inline bool truncate(size_t _nch, size_t _size)
  {
    if (!buf.truncate(_size * sizeof(sample_t) * _nch))
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
    samples.set_null();
  }

  inline void zero()          
  { 
    buf.zero();
  }

  inline size_t    get_size() const              { return size;         }
  inline uint8_t  *get_rawdata() const           { return rawdata;      }
  inline samples_t get_samples() const           { return samples;      }

  inline operator uint8_t *() const              { return rawdata;      }
  inline operator samples_t() const              { return samples;      }
  inline sample_t *operator[](int _ch) const     { return samples[_ch]; }
};

/*
class SampleBuf
{
protected:
  DataBuf   buf;
  samples_t samples;

  int buf_nch;
  int buf_nsamples;

  inline void set_pointers();

public:
  SampleBuf()
  {
    buf_nch = 0;
    buf_nsamples = 0;
  }

  SampleBuf(int _nch, int _nsamples)
  {
    buf_nch = 0;
    buf_nsamples = 0;
    allocate(_nch, _nsamples);
  }

  inline bool allocate(int nch, int nsamples);
  inline bool truncate(int nch, int nsamples);

  inline void zero()           { buf.zero();           }
//  inline bool is_null() const  { return buf.is_null(); } 

  inline int  nch() const      { return buf_nch;       }
  inline int  nsamples() const { return buf_nsamples;  }

  inline operator samples_t () const         { return samples; }
  inline sample_t *operator [](int ch) const { return samples[ch]; }
};

*/

///////////////////////////////////////////////////////////////////////////////
// samples_t inlines
///////////////////////////////////////////////////////////////////////////////

inline samples_t &
samples_t::operator +=(int _n)
{
  samples[0] += _n;
  samples[1] += _n;
  samples[2] += _n;
  samples[3] += _n;
  samples[4] += _n;
  samples[5] += _n;
  return *this;
}

inline samples_t &
samples_t::operator -=(int _n)
{
  samples[0] -= _n;
  samples[1] -= _n;
  samples[2] -= _n;
  samples[3] -= _n;
  samples[4] -= _n;
  samples[5] -= _n;
  return *this;
}

inline samples_t &
samples_t::set_null()
{
  samples[0] = 0;
  samples[1] = 0;
  samples[2] = 0;
  samples[3] = 0;
  samples[4] = 0;
  samples[5] = 0;
  return *this;
}

inline void
samples_t::reorder_to_std(Speakers _spk, const int _order[NCHANNELS])
{
  int i, ch;
  int mask = _spk.mask;

  sample_t *tmp[NCHANNELS];

  ch = 0;
  for (i = 0; i < NCHANNELS; i++)
    if (mask & CH_MASK(_order[i]))
      tmp[_order[i]] = samples[ch++];

  ch = 0;
  for (i = 0; i < NCHANNELS; i++)
    if (mask & CH_MASK(i))
      samples[ch++] = tmp[i];
}

inline void
samples_t::reorder_from_std(Speakers _spk, const int _order[NCHANNELS])
{
  int i, ch;
  int mask = _spk.mask;

  sample_t *tmp[NCHANNELS];

  ch = 0;
  for (i = 0; i < NCHANNELS; i++)
    if (mask & CH_MASK(i))
      tmp[i] = samples[ch++];

  ch = 0;
  for (i = 0; i < NCHANNELS; i++)
    if (mask & CH_MASK(_order[i]))
      samples[ch++] = tmp[_order[i]];
}

inline void
samples_t::reorder(Speakers _spk, const int _input_order[NCHANNELS], const int _output_order[NCHANNELS])
{
  int i, ch;
  int mask = _spk.mask;

  sample_t *tmp[NCHANNELS];

  ch = 0;
  for (i = 0; i < NCHANNELS; i++)
    if (mask & CH_MASK(_input_order[i]))
      tmp[_input_order[i]] = samples[ch++];

  ch = 0;
  for (i = 0; i < NCHANNELS; i++)
    if (mask & CH_MASK(_output_order[i]))
      samples[ch++] = tmp[_output_order[i]];
}

///////////////////////////////////////////////////////////////////////////////
// SampleBuf inlines
///////////////////////////////////////////////////////////////////////////////
/*
inline bool
SampleBuf::allocate(int _nch, int _nsamples)
{
  buf_nch = _nch;
  buf_nsamples = _nsamples;
  if (buf.get_size() < _nch * _nsamples * sizeof(sample_t))
    return truncate(_nch, _nsamples);

  set_pointers();

  return true;
}

inline bool
SampleBuf::truncate(int _nch, int _nsamples)
{
  if (buf.truncate(_nch * _nsamples * sizeof(sample_t)))
  {
    buf_nch = _nch;
    buf_nsamples = _nsamples;
    set_pointers();
    return true;
  }
  else
  {
    buf_nch = 0;
    buf_nsamples = 0;
    return false;
  }
}

inline void 
SampleBuf::set_pointers()
{
  samples[0] = (sample_t *)buf.get_data();

  int ch = 1;
  for (ch = 1; ch < buf_nch; ch++)
    samples[ch] = samples[ch-1] + buf_nsamples;

  while (ch < NCHANNELS)
    samples[ch++] = 0;
}
*/
#endif
