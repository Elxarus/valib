/**************************************************************************//**
  \file chunk.h
  \brief Chunk class
******************************************************************************/

#ifndef VALIB_CHUNK_H
#define VALIB_CHUNK_H

#include "spk.h"
#include <string>

/**************************************************************************//**
  \class Chunk
  \brief A part of audio data

  This structure is used to transfer audio data along with time stamps between
  Source, Filter and Sink objects.

  Format of the data is determined by output format of a Source or Filter.
  (see Filter::get_output() and Source::get_output())

  \var uint8_t *Chunk::rawdata;
    Pointer to raw data buffer.

  \var samples_t Chunk::samples;
    Pointer to samples buffer.

  \var size_t Chunk::size;
    Size of the buffer. When raw data buffer is given, it means size fo the
    buffer in bytes. When samples buffer is given it means number of samples
    per channel.

  \var bool Chunk::sync;
    When this flag is set, time stamp is set. Otherwise time should be ignored.

  \var vtime_t Chunk::time;
    Time associated with the begnning of the data in the chunk.

    More specifically, this time should be applied to the first sync point in
    the chunk. For linear format this means the beginning of the first sample.

    For compressed formats such as AC3 or DTS first syncpoint is the beginning
    of the frame. I.e. if chunk contains the tail of a previous frame and a
    head of a next frame, time should be applied to the first sample of the new
    frame.

  \fn Chunk::Chunk()
    Constructs a dummy chunk with no data and no timestamp.

  \fn Chunk::Chunk(bool sync, vtime_t time):
    \param sync Sync flag
    \param time Time stamp

    Constructs an empty chunk with time stamp.

  \fn Chunk::Chunk(samples_t samples, size_t size, bool sync = false, vtime_t time = 0)
    \param samples Pointers to samples
    \param size    Number of samples per channel
    \param sync    Sync flag
    \param time    Time stamp

    Constructs a chunk with linear format data.

  \fn Chunk::Chunk(uint8_t *rawdata, size_t size, bool sync = false, vtime_t time = 0)
    \param rawdata Pointer to raw data buffer
    \param size    Size of the buffer in bytes
    \param sync    Sync flag
    \param time    Time stamp

    Constructs a chunk with raw data.

  \fn void Chunk::clear()
    Drop data and time stamp. Chunk becomes dummy.

  \fn void Chunk::set_linear(samples_t samples, size_t size, bool sync = false, vtime_t time = 0)
    \param samples Pointers to samples
    \param size    Number of samples per channel
    \param sync    Sync flag
    \param time    Time stamp

    Fills the chunk with linear format data.

  \fn void Chunk::set_rawdata(uint8_t *rawdata, size_t size, bool sync = false, vtime_t time = 0)
    \param rawdata Pointer to raw data buffer
    \param size    Size of the buffer in bytes
    \param sync    Sync flag
    \param time    Time stamp

    Fills the chunk with raw data.

  \fn void Chunk::set_sync(bool sync, vtime_t time)
    \param sync Sync flag
    \param time Time stamp

    Set time stamp for the chunk.

  \fn bool Chunk::is_dummy() const
    Check for dummy chunk (chunk that does not contain data or time stamp).

  \fn bool Chunk::is_empty() const
    Check for empty chunk (chunk that does not contain data).

  \fn void Chunk::drop_rawdata(size_t drop_size)
    \param drop_size Size of data to drop.

    Drops some data from the beginning of the chunk.
    Moves raw data pointer ahead by drop_size bytes.

  \fn void Chunk::drop_samples(size_t drop_size)
    \param drop_size Size of data to drop.

    Drops some data from the beginning of the chunk.
    Move raw sample pointers ahead by drop_size samples.
******************************************************************************/

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
