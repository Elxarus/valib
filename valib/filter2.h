/*
  Filter2
  Filter interface

  bool can_open(Speakers spk) const
    Check format support. Returns true when filter supports the format given.
    Note that filter may refuse to open the stream even when the stream format
    is supported.

  bool open(const Stream *stream)
    Open the filter. Returns true on success. Filter may process data after
    this call.

  void close()
    Close the filter and deallocate resources.

  bool is_open() const
    Return true when filter is open and can process data.

    Output stream may be set after this call when output format may be
    determined from input format (or when filter does not change the format
    at all).

    But sometimes output format may be determined only after some data received
    (parsers for example). In this case output_format() returns zero until the
    first output chunk.

  bool is_inplace() const
    Returns true when filter modifies data inplace at input buffers. Output
    buffers in this case point inside input buffers.

  bool stream_convert() const
    Returns true when filter changes the stream format.

  const Stream *input_stream()
    Returns input stream pointer. Returns pointer passed to open() call.

  /////////////////////////////////////////////////////////
  // Processing

  bool process(Chunk &in, Chunk &out)
    Process data and form one output chunk.

    Filter may process only part of the data. In this case it drops the data
    processed from the input chunk.

    Filter may buffer data. So when it is not enough data at input, filter may
    eat all input and produce no output.

  bool flush(Chunk &out)
    Flush buffered data. We need this only when the filter needs flushing
    (need_flushing() returns true). Note, that filter may produce several
    chunks when flushing, so several flush() calls may be required.

  void reset()
    Reset the filter state, zero all internal buffers and prepare to a new
    stream. Do not deallocate resources, because we're awaiting new data.

  bool eos() const
    End of stream. When filter needs all downstream filters to flush and
    prepare to a new stream. Filter must set eos flag before changing output
    stream.

  bool need_flushing()
    Returns true when filter has buffered data and needs flushing.

  const Stream *output_stream()
    Returns output stream. It may return null when output stream is still
    unknown.
*/

#ifndef VALIB_FILTER2_H
#define VALIB_FILTER2_H

#include "filter.h"
#include "buffer.h"

class Chunk2;
class Filter2;
class Stream;
class FilterThunk;


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

  Chunk2():
    rawdata(0),
    size(0),
    sync(false),
    time(0)
  {}

  Chunk2(bool _sync, vtime_t _time)
  {
    set_empty(_sync, _time);
  }

  Chunk2(samples_t _samples, size_t _size,
    bool _sync = false, vtime_t _time = 0)
  {
    set_linear(_samples, _size, _sync, _time);
  }

  Chunk2(uint8_t *_rawdata, size_t _size,
    bool _sync = false, vtime_t _time = 0)
  {
    set_rawdata(_rawdata, _size, _sync, _time);
  }

  Chunk2(uint8_t *_rawdata, samples_t _samples, size_t _size,
    bool _sync = false, vtime_t _time = 0)
  {
    set(_rawdata, _samples, _size, _sync, _time);
  }


  inline void set_empty(bool _sync = false, vtime_t _time = 0)
  {
    rawdata = 0;
    samples.zero();
    size = 0;
    sync = _sync;
    time = _time;
  }

  inline void set_linear(samples_t _samples, size_t _size,
    bool _sync = false, vtime_t _time = 0)
  {
    rawdata = 0;
    samples = _samples;
    size = _size;
    sync = _sync;
    time = _time;
  }

  inline void set_rawdata(uint8_t *_rawdata, size_t _size,
    bool _sync = false, vtime_t _time = 0)
  {
    rawdata = _rawdata;
    samples.zero();
    size = _size;
    sync = _sync;
    time = _time;
  }

  inline void set(uint8_t *_rawdata, samples_t _samples, size_t _size,
    bool _sync = false, vtime_t _time = 0)
  {
    rawdata = _rawdata;
    samples = _samples;
    size = _size;
    sync = _sync;
    time = _time;
  }

  inline void set_sync(bool _sync, vtime_t _time)
  {
    sync = _sync;
    time = _time;
  }

  inline bool is_dummy() const
  {
    return (size == 0) && !sync;
  }

  inline bool is_empty() const
  { 
    return size == 0; 
  }

  inline void drop_rawdate(size_t _size)
  {
    if (_size > size)
      _size = size;

    rawdata += _size;
    size -= _size;
    sync = false;
  };

  inline void drop_samples(size_t _size)
  {
    if (_size > size)
      _size = size;

    samples += _size;
    size -= _size;
    sync = false;
  };
};

class Filter2
{
protected:
  // Prohibit assignment
  Filter2(const Filter2 &);
  Filter2 &operator =(const Filter2 &);
  FilterThunk *thunk;

public:
  Filter2(): thunk(0) {};
  virtual ~Filter2();

  Filter *filter();

  /////////////////////////////////////////////////////////
  // Open/close the filter

  virtual bool can_open(Speakers spk) const = 0;
  virtual bool open(Speakers spk) = 0;
  virtual void close() = 0;

  // These flags are set after open() and do not change
  virtual bool is_open() const = 0;
  virtual bool is_inplace() const = 0;
  virtual Speakers get_input() = 0;

  /////////////////////////////////////////////////////////
  // Processing

  virtual bool process(Chunk2 &in, Chunk2 &out) = 0;
  virtual bool flush(Chunk2 &out) = 0;
  virtual void reset() = 0;

  // These flags may change after process() call
  virtual bool eos() const = 0;
  virtual bool need_flushing() const = 0;
  virtual Speakers get_output() const = 0;
};


class SamplesFilter : public Filter2
{
protected:
  bool f_open;
  Speakers spk;

public:
  SamplesFilter(): f_open(false)
  {}

  virtual ~SamplesFilter()
  {
    if (is_open())
      close();
  }

  /////////////////////////////////////////////////////////
  // Open/close the filter

  virtual bool can_open(Speakers spk) const
  {
    return spk.is_linear() && spk.mask != 0 && spk.sample_rate != 0;
  }

  virtual bool open(Speakers new_spk)
  {
    if (!can_open(new_spk))
      return false;

    f_open = true;
    spk = new_spk;
    return true;
  }

  virtual void close()
  {
    f_open = false;
    spk = spk_unknown;
  }

  virtual bool is_open() const
  { return f_open; }

  virtual Speakers get_input()
  { return spk; }

  /////////////////////////////////////////////////////////
  // Processing

  virtual bool flush(Chunk2 &out)
  { return true; }

  virtual void reset()
  {}

  virtual bool eos() const
  { return false; }

  virtual bool need_flushing() const
  { return false; }

  virtual Speakers get_output() const
  { return spk; }
};


class FilterThunk : public Filter
{
protected:
  Filter2 *f;
  Chunk2 chunk2;

  Speakers spk;
  bool flushing;

public:
  FilterThunk(Filter2 *f_): f(f_), flushing(false) {};

  virtual void reset()
  {
    f->reset();
    chunk2.set_empty();
    flushing = false;
  }

  virtual bool is_ofdd() const
  {
    return false;
  }

  virtual bool query_input(Speakers new_spk) const
  {
    return f->can_open(new_spk);
  }

  virtual bool set_input(Speakers new_spk)
  {
    chunk2.set_empty();
    flushing = false;
    if (!f->open(new_spk))
    {
      spk = spk_unknown;
      f->reset();
      return false;
    }

    spk = new_spk;
    f->reset();
    return true;
  }

  virtual Speakers get_input() const
  {
    if (!f->is_open())
      return spk_unknown;
    return spk;
  }

  virtual bool process(const Chunk *chunk)
  {
    assert(chunk2.is_empty());

    // ignore dummy chunks
    if (chunk->is_dummy())
      return true;

    // remember data
    chunk2.set(chunk->rawdata, chunk->samples, chunk->size,
      chunk->sync, chunk->time);

    // format change
    if (spk != chunk->spk)
    {
      chunk2.set_empty();
      flushing = false;
      if (!f->open(chunk->spk))
      {
        spk = spk_unknown;
        f->reset();
        return false;
      }
      spk = chunk->spk;
      f->reset();
    }

    // flushing
    if (chunk->eos)
      flushing = true;
    return true;
  }

  virtual Speakers get_output() const
  {
    return f->get_output();
  }

  virtual bool is_empty() const
  {
    return !flushing && chunk2.is_empty();
  }

  virtual bool get_chunk(Chunk *out_chunk)
  {
    Chunk2 out_chunk2;

    // normal processing
    if (!chunk2.is_empty())
    {
      if (!f->process(chunk2, out_chunk2))
        return false;

      out_chunk->set(spk,
        out_chunk2.rawdata, out_chunk2.samples, out_chunk2.size,
        out_chunk2.sync, out_chunk2.time);
      out_chunk->set_eos(f->eos());
      return true;
    }

    // flushing
    if (flushing)
    {
      if (f->need_flushing())
      {
        f->flush(out_chunk2);
        out_chunk->set(spk,
          out_chunk2.rawdata, out_chunk2.samples, out_chunk2.size,
          out_chunk2.sync, out_chunk2.time);
        out_chunk->set_eos(f->eos());
      }
      else
      {
        out_chunk->set_empty(spk);
        out_chunk->set_eos(true);
        flushing = false;
      }
      return true;
    }

    // dummy
    out_chunk->set_dummy();
    return true;
  }
};

#endif
