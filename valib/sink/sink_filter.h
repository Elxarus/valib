/*
  SinkFilter
  Combination of a filter and a sink, acting like a sink.
*/

#ifndef VALIB_SINK_FILTER_H
#define VALIB_SINK_FILTER_H

#include "../filter.h"
#include "../sink.h"

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
