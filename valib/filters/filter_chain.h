/*
  Filter chain class
  Represents multiple filters as one
*/

#ifndef FILTER_CHAIN_H
#define FILTER_CHAIN_H

#include "filter.h"

class FilterChain : public Filter
{
protected:
  class Entry
  {
  public:
    Filter *filter;
    char *desc;
    bool ok;

    Entry *prev;

    Entry(Filter *filter, const char *desc = 0, Entry *prev = 0);
    ~Entry();

    inline void reset();
    inline bool is_empty();
    inline bool process(const Chunk *in);
    inline bool get_chunk(Chunk *out);
  };

  Speakers spk;
  Chunk chunk; // only for pass-through mode
  Entry *last;

public:
  FilterChain();
  ~FilterChain();

  void add(Filter *filter, const char *desc);
  void clear();

  // Filter interface
  virtual void reset();

  virtual bool query_input(Speakers spk) const;
  virtual bool set_input(Speakers spk);
  virtual bool process(const Chunk *in);

  virtual Speakers get_output();
  virtual bool is_empty();
  virtual bool get_chunk(Chunk *out);
}; 


inline void 
FilterChain::Entry::reset()
{
  filter->reset();
  ok = true;
}


inline bool 
FilterChain::Entry::is_empty()
{
  return filter->is_empty();
}

inline bool 
FilterChain::Entry::process(const Chunk *in)
{
  return ok = filter->process(in);
}

inline bool 
FilterChain::Entry::get_chunk(Chunk *out)
{
  Chunk chunk;

  if (is_empty())
  {
    if (!prev)
    {
      out->set_empty();
      return true;
    }

    if (!prev->get_chunk(&chunk))
      return false;
 
    if (!(ok = filter->process(&chunk)))
      return false;
  }

  return ok = filter->get_chunk(out);
}

#endif
