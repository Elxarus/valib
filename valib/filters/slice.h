/*
  SliceFilter
  This filter cuts a middle of the input stream.
*/

#ifndef VALIB_SLICE_H
#define VALIB_SLICE_H

#include "../filter.h"

class SliceFilter : public SimpleFilter
{
protected:
  size_t pos;
  size_t start;
  size_t end;

public:
  SliceFilter(size_t _start = 0, size_t _end = 0);
  void init(size_t _start, size_t _end);

  /////////////////////////////////////////////////////////
  // SimpleFilter overrides

  void reset();
  bool process(Chunk2 &in, Chunk2 &out);

  virtual bool can_open(Speakers spk) const
  { return true; }
};

#endif
