/*
  SinkFilter
  Combination of a filter and a sink, acting like a sink.
*/

#ifndef VALIB_SINK_FILTER_H
#define VALIB_SINK_FILTER_H

#include "../filter.h"
#include "../sink.h"

class SinkFilter2 : public Sink
{
protected:
  Sink   *sink;
  Filter *filter;

public:
  SinkFilter2():
  sink(0), filter(0)
  {}

  SinkFilter2(Sink *sink_, Filter *filter_):
  sink(0), filter(0)
  { set(sink_, filter_); }

  bool set(Sink *sink_, Filter *filter_)
  {
    if (!sink_)
      return false;

    sink = sink_;
    filter = filter_;
    return true;
  }
  void release()
  {
    sink = 0;
    filter = 0;
  }

  Sink   *get_sink()   const { return sink;   }
  Filter *get_filter() const { return filter; }

  /////////////////////////////////////////////////////////
  // Open/close the filter

  virtual bool can_open(Speakers spk) const
  {
    if (!sink) return false;
    if (!filter) return sink->can_open(spk);
    return filter->can_open(spk);
  }

  virtual bool open(Speakers spk)
  {
    if (!sink) return false;
    if (!filter) return sink->open(spk);

    if (!filter->open(spk))
      return false;

    if (filter->get_output().is_unknown())
      return true;

    return sink->open(spk);
  }

  virtual void close()
  {
    if (!sink) return;

    sink->close();
    if (filter)
      filter->close();
  }

  /////////////////////////////////////////////////////////
  // Processing

  virtual void reset()
  {
    if (!sink) return;

    sink->reset();
    if (filter)
      filter->reset();
  }

  virtual void process(const Chunk &in)
  {
    if (!sink) return;
    if (!filter)
    {
      sink->process(in);
      return;
    }

    Chunk non_const_in = in;
    Chunk out;
    while (filter->process(non_const_in, out))
    {
      if (filter->new_stream())
      {
        sink->flush();
        if (!sink->open(filter->get_output()))
          throw SinkError(this, 0, "Cannot reopen sink");
      }
      sink->process(out);
    }
  }

  virtual void flush()
  {
    if (!sink) return;
    if (!filter)
    {
      sink->flush();
      return;
    }

    Chunk out;
    while (filter->flush(out))
    {
      if (filter->new_stream())
      {
        sink->flush();
        if (!sink->open(filter->get_output()))
          throw SinkError(this, 0, "Cannot reopen sink");
      }
      sink->process(out);
    }
    sink->flush();
  }

  // Sink state
  virtual bool is_open() const
  {
    if (!sink) return false;
    return sink->is_open();
  }

  virtual Speakers get_input() const
  {
    if (!sink) return spk_unknown;
    if (!filter) return sink->get_input();
    return filter->get_input();
  }
};

#endif
