/*
  SliceFilter
  This filter cuts a middle of the input stream.
*/

#ifndef VALIB_SLICE_H
#define VALIB_SLICE_H

#include "../filter.h"

class SliceFilter : public NullFilter
{
protected:
  size_t pos;
  size_t start;
  size_t end;

public:
  SliceFilter(size_t _start = 0, size_t _end = 0);
  void init(size_t _start, size_t _end);

  void reset();
  bool process(const Chunk *_chunk);
};

#endif
