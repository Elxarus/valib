/**************************************************************************//**
  \file sink_filter.h
  \brief SinkFilter class
******************************************************************************/

#ifndef VALIB_SINK_FILTER_H
#define VALIB_SINK_FILTER_H

#include "../filter.h"
#include "../sink.h"

/**************************************************************************//**
  \class SinkFilter
  \brief Combination of a sink and a filter, acting like a sink.

  SinkFilter may be in 3 states:
  - Uninitialized
  - Sink only
  - Sink with filter

  Uninititalized sink can never be open.
 
  Provided only with a sink, SinkFilter acts like a wrapper for this sink.

  Given both sink and filter, SinkFilter passes each chunk through a filter
  to a sink. Format changes are taken in account.

  If output format of the filter does not match the sink's input format,
  SinkFilter reopens the sink with the correct format.

  EOpenFilter exception may be thrown from set() and process() functions.

  \fn SinkFilter::SinkFilter();
    Creates uninitialized object.

  \fn SinkFilter(Sink *sink, Filter *filter);
    Calls set().

  \fn void SinkFilter::set(Sink *sink, Filter *filter);
    First of all, releases sink and filter previously set. When both
    arguments are zero, it is equivalent to release().

    When only sink is given, takes this sink under control and acts as a
    wrapper for it.

    Given both sink and filter, takes control on both. If output format of
    the filter does not match the sink's input format, reopens the sink
    with the correct format.

    May throw EOpenFilter

  \fn void SinkFilter::release();
    Releases sink and filter previously set (if any).

  \fn Sink *SinkFilter::get_sink() const;
    Returns sink currently set or null pointer.

  \fn Filter *SinkFilter::get_filter() const;
    Returns filter currently set or null pointer.

******************************************************************************/

class SinkFilter : public Sink
{
protected:
  Sink   *sink;
  Filter *filter;

public:
  SinkFilter();
  SinkFilter(Sink *sink, Filter *filter);

  void set(Sink *sink, Filter *filter);
  void release();

  Sink   *get_sink()   const { return sink;   }
  Filter *get_filter() const { return filter; }

  /////////////////////////////////////////////////////////
  // Open/close the sink

  virtual bool can_open(Speakers spk) const;
  virtual bool open(Speakers spk);
  virtual void close();
  virtual bool is_open() const;
  virtual Speakers get_input() const;

  /////////////////////////////////////////////////////////
  // Processing

  virtual void reset();
  virtual void process(const Chunk &in);
  virtual void flush();
};

#endif
