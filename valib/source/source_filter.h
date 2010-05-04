/*
  SourceFilter
  Combination of a source and a filter, acting like a source.
*/

#ifndef VALIB_SOURCE_FILTER_H
#define VALIB_SOURCE_FILTER_H

#include "../filter.h"
#include "../source.h"

class SourceFilter2 : public Source2
{
protected:
  Source2 *source;
  Filter2 *filter;
  Chunk2 chunk;

  bool is_new_stream;
  bool format_change;
  enum { state_empty, state_filter, state_new_stream } state;

public:
  SourceFilter2():
  source(0), filter(0)
  {}

  SourceFilter2(Source2 *source_, Filter2 *filter_):
  source(0), filter(0)
  { set(source_, filter_); }

  bool set(Source2 *source_, Filter2 *filter_);
  void release();

  Source2 *get_source() const { return source; }
  Filter2 *get_filter() const { return filter; }

  /////////////////////////////////////////////////////////
  // Source interface

  virtual void reset();
  virtual bool get_chunk(Chunk2 &out);
  virtual bool new_stream() const;
  virtual Speakers get_output() const;
};

#endif
