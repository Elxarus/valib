/**************************************************************************//**
  \file source_filter.h
  \brief SourceFilter class
******************************************************************************/
#ifndef VALIB_SOURCE_FILTER_H
#define VALIB_SOURCE_FILTER_H

#include "../filter.h"
#include "../source.h"

/**************************************************************************//**
  \class SourceFilter
  \brief Combination of a source and a filter, acting like a source.

  The source may be in 3 states:
  - Uninitialized
  - Source only
  - Source with filter

  Uninititalized source:
  - always return false on get_chunk() call
  - new_stream() returns false
  - get_output() returns FORMAT_UNKNOWN
  - reset() does nothting

 
  Provided only with a source, SourceFilter acts like a wrapper for this
  source.

  Given both source and filter, SourceFilter passes each chunk fron a source
  through the filter. Format changes are taken in account.

  If output format of the source does not match the filter's input format,
  SourceFilter reopens the filter with correct format.

  EOpenFilter exception may be thrown from set() or get_chunk() functions.

  \fn SourceFilter::SourceFilter();
    Creates uninitialized object;

  \fn SourceFilter(Source *source, Filter *filter);
    Calls set();

  \fn void SourceFilter::set(Source *source, Filter *filter);
    First of all, releases source and filter previously set. When both
    arguments are zero, it is equivalent to release().

    When only source is given, takes this source under control and acts as a
    wrapper for it.

    Given both source and filter, takes control on both. If output format of
    the source does not match the filter's input format, reopens the filter
    with correct format.

    May throw EOpenFilter

  \fn void SourceFilter::release();
    Releases source and filter previously set (if any).

  \fn Source *SourceFilter::get_source() const;
    Returns source currently set or null pointer.

  \fn Filter *SourceFilter::get_filter() const;
    Returns filter currently set or null pointer.

******************************************************************************/

class SourceFilter : public Source
{
protected:
  Source *source;
  Filter *filter;
  Chunk chunk;

  bool is_new_stream;
  bool format_change;
  bool need_flushing;
  enum {
    state_empty,
    state_filter,
    state_new_stream,
    state_flush
  } state;

public:
  SourceFilter():
  source(0), filter(0)
  {}

  SourceFilter(Source *source_, Filter *filter_):
  source(0), filter(0)
  { set(source_, filter_); }

  void set(Source *source_, Filter *filter_);
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
