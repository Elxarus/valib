/*
  Filter chain class
  Represents multiple filters as one
*/

#ifndef FILTER_CHAIN_H
#define FILTER_CHAIN_H

#include "filter.h"

class FilterChain : public NullFilter
{
protected:
  class Entry
  {
  public:
    Filter *filter;
    char   *desc;

    Entry  *source;
    Entry  *sink;

    Entry(Filter *_filter, const char *_desc)
    {
      filter = _filter;
      desc = _desc? strdup(_desc): 0;
      source = 0;
      sink = 0;
    }

    ~Entry()
    {
      if (desc)
        delete desc;
    };

    // Filter wrapper
    inline void reset()                       { filter->reset();                  }

    inline bool query_input(Speakers _spk)    { return filter->query_input(_spk); }
    inline bool set_input(Speakers _spk)      { return filter->set_input(_spk);   }
    inline bool process(const Chunk *_chunk)  { return filter->process(_chunk);   }

    inline Speakers get_output() const        { return filter->get_output();      }
    inline bool is_empty() const              { return filter->is_empty();        }
    inline bool get_chunk(Chunk *_chunk)      { return filter->get_chunk(_chunk); }
  };

  Entry *first;
  Entry *last;

  bool process_internal(Entry *entry);

public:
  FilterChain();
  ~FilterChain();

  void add_front(Filter *filter, const char *desc);
  void add_back(Filter *filter, const char *desc);
  void clear();

  // Filter interface
  virtual void reset();

  virtual bool query_input(Speakers spk) const;
  virtual bool set_input(Speakers spk);
  virtual bool process(const Chunk *in);

  virtual Speakers get_output() const;
  virtual bool is_empty() const;
  virtual bool get_chunk(Chunk *out);
}; 

#endif
