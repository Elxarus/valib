/*
  SourceFilter
  Combination of a source and a filter, acting like a source.
*/

#ifndef VALIB_SOURCE_FILTER_H
#define VALIB_SOURCE_FILTER_H

#include "../filter.h"
#include "../source.h"

class SourceFilter : public Source
{
protected:
  Source *source;
  Filter *filter;
  Chunk chunk;

  bool is_new_stream;
  bool format_change;
  enum { state_empty, state_filter, state_new_stream } state;

public:
  SourceFilter():
  source(0), filter(0)
  {}

  SourceFilter(Source *source_, Filter *filter_):
  source(0), filter(0)
  { set(source_, filter_); }

  bool set(Source *source_, Filter *filter_);
  void release();

  Source *get_source() const { return source; }
  Filter *get_filter() const { return filter; }

  /////////////////////////////////////////////////////////
  // Source interface

  virtual void reset();
  virtual bool get_chunk(Chunk &out);
  virtual bool new_stream() const;
  virtual Speakers get_output() const;
};

#endif
