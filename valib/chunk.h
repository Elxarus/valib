/*
  Chunk

  This structure is used to transfer audio data along with time stamps between
  Source, Filter and Sink objects.

  Format of the data is determined by output format of a Source or Filter.
  (see Filter::get_output() and Source::get_output())

  /////////////////////////////////////////////////////////
  // Data

  uint8_t *rawdata;
    Pointer to raw data buffer.

  samples_t samples;
    Pointer to samples buffer.

  size_t size;
    Size of the buffer. When raw data buffer is given, it means size fo the
    buffer in bytes. When samples buffer is given it means number of samples
    per channel.

  bool sync;
    When this flag is set, time stamp is set. Otherwise time should be ignored.

  vtime_t time;
    Time associated with the begnning of the data in the chunk.

    More specifically, this time should be applied to the first sync point in
    the chunk. For linear format this means the beginning of the first sample.
    For compressed formats such as AC3 or DTS first syncpoint is the beginning
    of the frame. I.e. if chunk contains the tail of a previous frame and a
    head of a next frame, time should be applied to the first sample of the new
    frame.

  /////////////////////////////////////////////////////////
  // Utilities

  Chunk()
    Constructs dummy chunk with no data and no timestamp.

  Chunk(bool sync, vtime_t time):
    Empty chunk with time stamp.

  Chunk(samples_t samples, size_t size,
        bool sync = false, vtime_t time = 0)
    Linear format chunk constructor.

  Chunk(uint8_t *rawdata, size_t size,
        bool sync = false, vtime_t time = 0)
    Raw data format chunk constructor.

  void clear()
    Drop data and time stamp. Chunk becomes dummy.

  void set_linear(samples_t samples, size_t size,
    bool sync = false, vtime_t time = 0)
    Make linear format chunk.

  void set_rawdata(uint8_t *rawdata, size_t size,
    bool sync = false, vtime_t time = 0)
    Make raw data chunk.

  void set_sync(bool sync, vtime_t time)
    Set time stamp for the chunk.

  bool is_dummy() const
    Check for dummy chunk (chunk that does not contain data or time stamp).

  bool is_empty() const
    Check for empty chunk (chunk that does not contain data).

  void drop_rawdata(size_t drop_size)
    Move raw data pointer ahead by drop_size bytes.

  void drop_samples(size_t drop_size)
    Move raw sample pointers ahead by drop_size samples.

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
    assert(rawdata);

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
