/*
  Data structures
*/

#ifndef DATA_H
#define DATA_H

#include <string.h>
#include "spk.h"


class  Chunk;
class  SampleBuf;
struct samples_t;

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


class Chunk
{
public:
  Speakers  spk;

  bool      timestamp;
  time_t    time;

  uint8_t  *buf;
  samples_t samples;
  int       size;

  Chunk()
  { set_empty(); }

  inline void set_spk(Speakers spk);
  inline void set_time(bool timestamp, time_t time = 0);
  inline void set_buf(uint8_t *buf, int size);
  inline void set_samples(samples_t samples, int nsamples);

  inline void set_empty();
  inline bool is_empty() const;

  inline void drop(int size);
};


class DataBuf
{
protected:
  uint8_t *buf;
  uint8_t *buf_aligned;
  int buf_size;

public:
  DataBuf()
  {
    buf = 0;
    buf_size = 0;
  }

  DataBuf(int _size)
  { 
    buf = 0;
    buf_size = 0;
    allocate(_size);
  }

  ~DataBuf()
  {
    drop();
  }
  
  inline bool allocate(int size);
  inline bool truncate(int size);
  inline void drop();

  inline void zero()          { if (buf) memset(buf, 0, buf_size); }
  inline uint8_t *data()const { return buf_aligned; }
  inline int  size()    const { return buf_size;    }
  inline bool is_null() const { return buf == 0;    } 

  inline operator uint8_t *() const { return buf_aligned; }
};

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
  inline bool is_null() const  { return buf.is_null(); } 

  inline int  nch() const      { return buf_nch;       }
  inline int  nsamples() const { return buf_nsamples;  }

  inline operator samples_t () const         { return samples; }
  inline sample_t *operator [](int ch) const { return samples[ch]; }
};


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

  sample_t * tmp[NCHANNELS];

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
// DataBuf inlines
///////////////////////////////////////////////////////////////////////////////

inline bool 
DataBuf::allocate(int _size)
{
  if (buf_size < _size)
    return truncate(_size);
  else
    return true;
}

inline bool 
DataBuf::truncate(int _size)
{  
  if (buf)
    delete buf;

  if (_size)
  {
    buf_size = _size;
    buf = new uint8_t[buf_size + 7];
    if (!buf) return false;
    buf_aligned = buf + 7;  
    buf_aligned -= (int)buf_aligned & 7;
  }
  return true;
}

inline void 
DataBuf::drop()
{
  if (buf) delete buf;
  buf = 0;
  buf_aligned = 0;
  buf_size = 0;
}


///////////////////////////////////////////////////////////////////////////////
// SampleBuf inlines
///////////////////////////////////////////////////////////////////////////////

inline bool
SampleBuf::allocate(int _nch, int _nsamples)
{
  if (buf_nch >= _nch && buf_nsamples >= _nsamples)
  {
    buf_nch = _nch;
    buf_nsamples = _nsamples;
    return true;
  }

  buf_nch = _nch;
  buf_nsamples = _nsamples;
  if (buf.size() < _nch * _nsamples * (int)sizeof(sample_t))
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
  samples[0] = (sample_t *)(uint8_t *)buf;

  int ch;
  for (ch = 1; ch < buf_nch; ch++)
    samples[ch] = samples.samples[ch-1] + buf_nsamples;

  while (ch < NCHANNELS)
    samples[ch++] = 0;
}

///////////////////////////////////////////////////////////////////////////////
// Chunk inlines
///////////////////////////////////////////////////////////////////////////////

// if chunk is empty then all other fields are uninitialized
// if timestamp is false then time is uninitialized
// if size is 0 then buf and samples are uninitialized

inline void Chunk::set_spk(Speakers _spk)
{
  spk = _spk;
}

inline void Chunk::set_time(bool _timestamp, time_t _time)
{
  timestamp = _timestamp;
  time = _time;
}

inline void Chunk::set_buf(uint8_t *_buf, int _size)
{
  buf = _buf;
  size = _size;
  samples.set_null();
}
inline void Chunk::set_samples(samples_t _samples, int _nsamples)
{
  samples = _samples;
  size = _nsamples;
  buf = 0;
}

inline void Chunk::set_empty()
{
  size = 0;
}
inline bool Chunk::is_empty() const
{
  return size == 0;
}

inline void Chunk::drop(int _size)
{
  if (_size > size)
    _size = size;

  if (buf)
    buf += _size;
  else
    samples += _size;
  size -= _size;
  timestamp = false;
}

#endif
