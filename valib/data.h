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
// Chunk class.
// A part of audio data being processed.
//
// There are several types of chunks:
// 1) Data chunk/empty chunk. If chunk has data then it's data chunk. 
//    Otherwise it's empty chunk.
// 2) Syncronization chunk (sync-chunk). If chunk has syncronization 
//    information then it's sync-chunk.
// 3) End-of-steam chunk (eos-chunk). Chunk that ends data transmission.
//
// Empty chunk (chunk that contain no data) may be used to inform downsteram
// about different events: format change, syncronization or end of steram.
//
// Speakers
// ========
// Chunk always has data format. 
//
// get_spk() - get data format
// set_spk() - set data format
//
//
// Syncronization
// ==============
// Chunk may contain timestamp that indicates position in the stream. In this
// case it is called syncronization chunk. Empty syncronization chunk is used 
// just to indicate position. (see also Sync class at filter.h).
//
// is_sync()  - is this chunk contains timestamp
// get_time() - returns timestamp
// set_sync() - set syncronization parameters
// 
// End-of-stream
// =============
// End-of-stream chunk (eos-chunk) is method to correctly finish the stream. 
// This chunk may contain data and stream is assumed to end after last 
// byte/sample of this chunk. Empty end-of-stream chunk is used just to end 
// the stream correctly. After receiving eos-chunk filter should flush all 
// internal data buffers.
//
// is_eos()  - chunk is end-of-stream chunk
// set_eos() - set chunk as end-of-stream and empty
//
// Empty chunk
// ===========
// Empty chunk may be used to deliver special events to the processing chain
// (format change, syncronization, end-of-stream). Both data buffers may be 
// invalid in this case.
//
// is_empty()  - chunk is empty
// set_empty() - set chunk as empty chunk
//
// Buffers
// =======
// Chunk has 2 types of buffer pointers: raw buffer and linear 
// splitted-channels buffer. Most of internal data processing is done on
// linear format, but data from external sources may be in any format. Some 
// filters works with both raw and linear data so for interface universality 
// it is only one type of chunk with both raw and linear buffers.
//
// get_size()    - number of samples in case of linear data farmat and size of
//                 raw buffer in bytes otherwise.       
// get_buf()     - raw buffer pointer (only for raw data)
// get_samples() - pointers to linear channel buffers (only for linear firmat)
// operator []   - pointer to channel buffer
// set_buf()     - set raw buffer
// set_samples() - set linear buffers
// drop()        - drop data from raw or linear buffer (bytes or samples).
//


class Chunk
{
protected:
  Speakers  spk;

  bool      sync;
  time_t    time;

  bool      eos;

  uint8_t  *buf;
  samples_t samples;
  size_t    size;

public:
  // Chunk()
  //
  // Speakers
  // inline Speakers get_spk() const
  // inline void set_spk(Speakers _spk)

  // Syncronization
  // inline bool is_sync() const
  // inline time_t get_time() const
  // inline void set_sync(bool _sync, time_t _time = 0)

  // End-of-stream
  // inline bool is_eos() const;
  // inline void set_eos(bool eos = true);

  // Empty chunk
  // inline bool is_empty() const
  // inline void set_empty()

  // Buffers
  // inline size_t get_size() const
  // inline uint8_t *get_buf() const
  // inline samples_t get_samples() const
  // inline sample_t *operator[](int _ch) const
  // inline void set_buf(uint8_t *_buf, size_t _size);
  // inline void set_samples(samples_t _samples, size_t _size) 
  // inline void drop(size_t _size)

  Chunk(): spk(unk_spk), time(false), sync(false), size(0)
  {}

  /////////////////////////////////////////////////////////
  // Speakers

  inline Speakers get_spk() const
  {
    return spk;
  }

  inline void set_spk(Speakers _spk)
  { 
    spk = _spk; 
  };

  /////////////////////////////////////////////////////////
  // Syncronization

  inline bool is_sync() const
  {
    return sync;
  }

  inline time_t get_time() const
  {
    return time;
  }

  inline void set_sync(bool _sync, time_t _time = 0)
  { 
    sync = _sync;
    time = _time;
  }

  /////////////////////////////////////////////////////////
  // End-of-stream

  inline bool is_eos() const
  {
    return eos;
  }

  inline void set_eos(bool eos = true)
  {
    size = 0;
    eos = true;
  }

  /////////////////////////////////////////////////////////
  // Empty chunk

  inline bool is_empty() const
  { 
    return size == 0; 
  }

  inline void set_empty()         
  { 
    size = 0; 
  }

  /////////////////////////////////////////////////////////
  // Buffers

  inline size_t get_size() const                 { return size;         }
  inline uint8_t *get_buf() const                { return buf;          }
  inline samples_t get_samples() const           { return samples;      }
  inline sample_t *operator[](int _ch) const     { return samples[_ch]; }

  inline void set_buf(uint8_t *_buf, size_t _size)
  { 
    buf = _buf; 
    size = _size;
  }

  inline void set_samples(const samples_t &_samples, size_t _size) 
  {
    samples = _samples; 
    size = _size; 
  }

  inline void drop(size_t _size)
  {
    if (_size > size)
      _size = size;

    if (spk.format == FORMAT_LINEAR)
      samples += _size;
    else
      buf += _size;

    size -= _size;
    sync = false;
  };

};



///////////////////////////////////////////////////////////////////////////////
// DataBuf
// Method of allocating data buffers.
// Now it's just simple aliged data block

class DataBuf
{
protected:
  uint8_t *buf;
  uint8_t *buf_aligned;
  size_t   buf_size;

public:
  // DataBuf()
  // DataBuf(int _size)

  // inline bool allocate(int _size)
  // inline bool truncate(int _size)
  // inline void drop()
  // inline void zero()

  // inline uint8_t *data() const 
  // inline int size() const 
  // inline bool is_null() const 
  // inline operator uint8_t *() const 

  DataBuf()
  {
    buf = 0;
    buf_size = 0;
  }

  DataBuf(size_t _size)
  { 
    buf = 0;
    buf_size = 0;
    allocate(_size);
  }

  ~DataBuf()
  {
    drop();
  }

  inline bool allocate(size_t _size)
  {
    if (buf_size < _size)
      return truncate(_size);
    else
      return true;
  }

  inline bool truncate(size_t _size)
  {
    drop();

    if (_size)
    {
      buf = new uint8_t[_size + 7];
      if (!buf) return false;
      buf_size = _size;
      buf_aligned = (uint8_t*)(unsigned(buf + 7) & ~7);
    }
    return true;
  }

  inline void drop()
  {
    if (buf) delete buf;

    buf = 0;
    buf_aligned = 0;
    buf_size = 0;
  }

  inline void zero()          
  { 
    if (buf) 
      memset(buf_aligned, 0, buf_size); 
  }

  inline uint8_t *data() const       { return buf_aligned; }
  inline size_t size() const         { return buf_size;    }
  inline bool is_null() const        { return buf == 0;    } 
  inline operator uint8_t *() const  { return buf_aligned; }
};



///////////////////////////////////////////////////////////////////////////////
// SampleBuf
// Method of allocating sample buffers for linear format.
// Implemented using DataBuf.

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
// SampleBuf inlines
///////////////////////////////////////////////////////////////////////////////

inline bool
SampleBuf::allocate(int _nch, int _nsamples)
{
  buf_nch = _nch;
  buf_nsamples = _nsamples;
  if (buf.size() < _nch * _nsamples * sizeof(sample_t))
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

#endif
