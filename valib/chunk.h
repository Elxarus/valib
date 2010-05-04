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
#include <string>

class ProcError
{
public:
  std::string name;
  std::string info;
  std::string text;
  int error_code;

  ProcError(std::string name_, std::string info_, int error_code_, std::string text_):
  name(name_), info(info_), error_code(error_code_), text(text_)
  {}

  virtual ~ProcError()
  {}
};

class Chunk
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
  Chunk():
    rawdata(0),
    size(0),
    sync(false),
    time(0)
  {}

  // Copy constructors
  Chunk(const Chunk &chunk):
    rawdata(chunk.rawdata),
    samples(chunk.samples),
    size(chunk.size),
    sync(chunk.sync),
    time(chunk.time)
  {}

  // Empty chunk with a timestamp
  Chunk(bool sync_, vtime_t time_):
    rawdata(0),
    size(0),
    sync(sync_),
    time(time_)
  {}

  // Linear format constructor
  Chunk(samples_t samples_, size_t size_,
    bool sync_ = false, vtime_t time_ = 0):
    rawdata(0),
    samples(samples_),
    size(size_),
    sync(sync_),
    time(time_)
  {}

  // Rawdata format constructor
  Chunk(uint8_t *rawdata_, size_t size_,
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

};

#endif
