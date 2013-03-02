/*
  Counter
  Counter filter counts the number of samples/bytes passed through the filter.
*/

#ifndef VALIB_COUNTER_H
#define VALIB_COUNTER_H

#include "../filter.h"

class Counter : public SimpleFilter
{
protected:
  size_t counter;

public:
  Counter(): counter(0)
  {}

  inline size_t get_count() const
  { return counter; }

  /////////////////////////////////////////////////////////
  // SimpleFilter overrides

  virtual bool can_open(Speakers spk) const
  { return true; }

  virtual void reset()
  { counter = 0; }

  virtual bool process(Chunk &in, Chunk &out)
  {
    out = in;
    in.clear();
    if (out.is_dummy())
      return false;

    counter += out.size;
    return true;
  }
};

#endif
