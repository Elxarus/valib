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


class Sink
{
public:
  virtual bool query_input(Speakers spk) const = 0;
  virtual bool set_input(Speakers spk) = 0;
  virtual bool process(const Chunk *chunk) = 0;
};


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
  // Filter should be able to report its state as before reset()
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
