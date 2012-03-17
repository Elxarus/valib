#include <limits>
#include "slice.h"

const size_t SliceFilter::not_specified = std::numeric_limits<size_t>::max();

SliceFilter::SliceFilter(size_t new_start, size_t new_end):
pos(0), start(new_start), end(new_end) 
{
  assert(start == not_specified || end == not_specified || start <= end);
}

void
SliceFilter::init(size_t new_start, size_t new_end)
{
  reset();
  start = new_start;
  end = new_end;
  assert(start == not_specified || end == not_specified || start <= end);
}

void
SliceFilter::reset()
{
  pos = 0;
}

bool
SliceFilter::process(Chunk &in, Chunk &out)
{
  out = in;
  in.clear();
  if (out.is_dummy())
    return false;

  // ignore everything after the end
  if (end != not_specified && pos >= end)
    return false;

  // ignore everything before the beginning
  if (start != not_specified && pos + out.size <= start)
  {
    pos += out.size;
    return false;
  }

  // cut off the tail
  if (end != not_specified && pos + out.size > end)
    out.size = end - pos;

  // cut of the head
  if (start != not_specified && pos < start)
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
