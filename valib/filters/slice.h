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
  static const size_t not_specified;

  SliceFilter(size_t start = not_specified, size_t end = not_specified);
  void init(size_t start = not_specified, size_t end = not_specified);

  /////////////////////////////////////////////////////////
  // Own interface

  size_t get_pos() const   { return pos;   }
  size_t get_start() const { return start; }
  size_t get_end() const   { return end;   }

  /////////////////////////////////////////////////////////
  // SimpleFilter overrides

  void reset();
  bool process(Chunk &in, Chunk &out);

  virtual bool can_open(Speakers spk) const
  { return true; }
};

#endif
