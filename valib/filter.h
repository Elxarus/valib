/*
  Abstract filter interface
  NullFilter implementation
*/

#ifndef VALIB_FILTER_H
#define VALIB_FILTER_H

#include "data.h"

class Sink;
class Source;
class Filter;
class NullFilter;


///////////////////////////////////////////////////////////////////////////////
// Sink class
//
// Abstract audio sink.
// 
// query_input()
//   Check if we can change format now. Returns true if we can and false if we
//   can't. Sink may have some internal state that may prevent format change, 
//   so this function should return true only if it can switch format now. 
//   It is assumed that query result may be changed only as a result of some 
//   state-change calls and does not change all other time. So we can query 
//   sink from one thread and then call set_input() from other and be sure 
//   that set_input() call succeeds. set_input() and process() calls should 
//   not change query result (?). Should work as fast as possible, because it 
//   may be used to try numerous formats to find acceptable conversion. Also, 
//   this function may be called asynchronously from other thread, so it 
//   should be thread-safe.
//
// set_input()
//   Change format. Returns true on success and false otherwise. It is 
//   expected that it returns true if query_input() returned true just
//   before set_input() call (no additional verification required).
//   It may require some time to change format (initialize output, device, 
//   allocate buffers, etc). Sound output is not required to be continious 
//   and clickless in case of format changes, but it is highly recommended 
//   to produce minimum artifacts. This function should be called only from 
//   worker thread. 
//
// process()
//   Receive data chunk. Returns true on success and false otherwise. Data 
//   chunk may have different format than specified by set_input() call so 
//   this function is required to handle format changes correctly (in this
//   case it is the same assumptions as for set_input() call). Empty chunks
//   shuold be inored without errors. Empty chunks may contain invalid format
//   (sink should not change format for empty chunks) and invalid timestamp.


class Sink
{
public:
  virtual bool query_input(Speakers spk) const = 0;
  virtual bool set_input(Speakers spk) = 0;
  virtual bool process(const Chunk *chunk) = 0;
};


///////////////////////////////////////////////////////////////////////////////
// Source class
//
// Abstract audio source.
//
// get_output()
//    Qurey current output format.

class Source
{
public:
  virtual Speakers get_output() = 0;
  virtual bool is_empty() = 0;
  virtual bool get_chunk(Chunk *chunk) = 0;
};


class Filter: public Sink, public Source
{ 
public:
  // Filter should be able to report its state as it was before reset()
  virtual void reset() = 0;

  virtual bool query_input(Speakers spk) const = 0;
  virtual bool set_input(Speakers spk) = 0;
  virtual bool process(const Chunk *chunk) = 0;

  virtual Speakers get_output() = 0;
  virtual bool is_empty() = 0;
  virtual bool get_chunk(Chunk *chunk) = 0;
};

class NullFilter : public Filter
{
protected:
  Speakers spk;   // input configuration
  Chunk    chunk; // input chunk

public:
  virtual void reset()
  {
    chunk.set_empty();
  }

  virtual bool query_input(Speakers _spk) const
  { 
    return _spk.format == FORMAT_LINEAR;
  }

  virtual bool set_input(Speakers _spk)
  {
    if (spk != _spk)
    {
      if (!query_input(_spk)) 
        return false;
      spk = _spk;
      reset();
    }
    return true;
  }

  virtual bool process(const Chunk *_chunk)
  {
    if (_chunk->is_empty())
    {
      chunk.set_empty();
      return true;
    }

    if (spk != _chunk->spk && !set_input(_chunk->spk)) 
      return false;

    chunk = *_chunk;
    return true;
  }

  virtual Speakers get_output()
  {
    return spk;
  }

  virtual bool is_empty()
  {
    return chunk.is_empty();
  };

  virtual bool get_chunk(Chunk *_chunk)
  {
    *_chunk = chunk;
    chunk.set_empty();
    return true;
  };
};

#endif
