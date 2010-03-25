#include "slice.h"

SliceFilter::SliceFilter(size_t _start, size_t _end):
pos(0), start(_start), end(_end) 
{
  assert(start <= end);
}

void
SliceFilter::init(size_t _start, size_t _end)
{
  reset();
  start = _start;
  end = _end;
  assert(start <= end);
}

void
SliceFilter::reset()
{
  pos = 0;
}

bool
SliceFilter::process(Chunk2 &in, Chunk2 &out)
{
  out = in;
  in.set_empty();
  if (out.is_dummy())
    return false;

  // ignore everything after the end
  if (pos >= end)
    return false;

  // ignore everything before the beginning
  if (pos + out.size <= start)
  {
    pos += out.size;
    return false;
  }

  // cut off the tail
  if (pos + out.size > end)
    out.size = end - pos;

  // cut of the head
  if (pos < start)
  {
    if (spk.is_linear())
      out.drop_samples(start - pos);
    else
      out.drop_rawdata(start - pos);
    pos = start;
  }

  pos += out.size;
  return true;
}
