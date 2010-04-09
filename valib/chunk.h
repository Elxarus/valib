/*
  Chunk

  This structure is used to transfer audio data along with time stamps between
  Source, Filter and Sink objects.

  Format of the data is determined by output format of a Source or Filter.
  (see Filter::get_output() and Source::get_output())
*/

#ifndef VALIB_CHUNK_H
#define VALIB_CHUNK_H

#include "spk.h"
#include "filter.h"

class Chunk2
{
public:
  /////////////////////////////////////////////////////////
  // Data

  uint8_t  *rawdata;
  samples_t samples;
  size_t    size;

  bool      sync;
  vtime_t   time;

  /////////////////////////////////////////////////////////
  // Utilities

  // Default constructor (dummy chunk)
  Chunk2():
    rawdata(0),
    size(0),
    sync(false),
    time(0)
  {}

  // Copy constructors
  Chunk2(const Chunk2 &chunk):
    rawdata(chunk.rawdata),
    samples(chunk.samples),
    size(chunk.size),
    sync(chunk.sync),
    time(chunk.time)
  {}

  Chunk2(const Chunk &chunk):
    rawdata(chunk.rawdata),
    samples(chunk.samples),
    size(chunk.size),
    sync(chunk.sync),
    time(chunk.time)
  {}

  // Empty chunk with a timestamp
  Chunk2(bool sync_, vtime_t time_):
    rawdata(0),
    size(0),
    sync(sync_),
    time(time_)
  {}

  // Linear format constructor
  Chunk2(samples_t samples_, size_t size_,
    bool sync_ = false, vtime_t time_ = 0):
    rawdata(0),
    samples(samples_),
    size(size_),
    sync(sync_),
    time(time_)
  {}

  // Rawdata format constructor
  Chunk2(uint8_t *rawdata_, size_t size_,
    bool sync_ = false, vtime_t time_ = 0):
    rawdata(rawdata_),
    size(size_),
    sync(sync_),
    time(time_)
  {}

  inline void clear()
  {
    rawdata = 0;
    samples.zero();
    size = 0;
    sync = false;
    time = 0;
  }

  inline void set_linear(samples_t samples_, size_t size_,
    bool sync_ = false, vtime_t time_ = 0)
  {
    rawdata = 0;
    samples = samples_;
    size = size_;
    sync = sync_;
    time = time_;
  }

  inline void set_rawdata(uint8_t *rawdata_, size_t size_,
    bool sync_ = false, vtime_t time_ = 0)
  {
    samples.zero();
    rawdata = rawdata_;
    size = size_;
    sync = sync_;
    time = time_;
  }

  inline void set_sync(bool sync_, vtime_t time_)
  {
    sync = sync_;
    time = time_;
  }

  inline bool is_dummy() const
  {
    return (size == 0) && !sync;
  }

  inline bool is_empty() const
  { 
    return size == 0; 
  }

  inline void drop_rawdata(size_t drop_size)
  {
    if (drop_size > size)
      drop_size = size;

    rawdata += drop_size;
    size -= drop_size;
    sync = false;
  };

  inline void drop_samples(size_t drop_size)
  {
    if (drop_size > size)
      drop_size = size;

    samples += drop_size;
    size -= drop_size;
    sync = false;
  };

  Chunk2 &operator =(const Chunk &chunk)
  {
    rawdata = chunk.rawdata;
    samples = chunk.samples;
    size = chunk.size;
    sync = chunk.sync;
    time = chunk.time;
    return *this;
  }
};

#endif
