#ifndef VALIB_COUNTER_H
#define VALIB_COUNTER_H

#include "filter.h"


class Counter : public NullFilter
{
protected:
  int counter;

public:
  Counter()
  {
    counter = 0;
  };

  inline int get_count() 
  {
    return counter; 
  }

  /////////////////////////////////////////////////////////
  // Filter interface

  void reset()
  {
    NullFilter::reset();
    counter = 0;
  }
  virtual bool query_input(Speakers _spk) const 
  { 
    return true; 
  };
  virtual bool process(const Chunk *_chunk)
  {
    FILTER_SAFE(NullFilter::receive_chunk(_chunk));
    counter += _chunk->size;
    return true;
  };
};

#endif
