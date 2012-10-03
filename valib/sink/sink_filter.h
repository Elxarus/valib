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
  struct Error : public Sink::Error {};

  SinkFilter():
  sink(0), filter(0)
  {}

  SinkFilter(Sink *sink_, Filter *filter_):
  sink(0), filter(0)
  { set(sink_, filter_); }

  void set(Sink *sink_, Filter *filter_)
  {
    release();

    if (sink_ && filter_)
    {
      if (filter_->is_open())
      {
        Speakers filter_spk = filter_->get_output();
        if (!filter_spk.is_unknown())
          if (!sink_->is_open() || sink_->get_input() != filter_spk)
            sink_->open_throw(filter_spk);
      }
    }

    sink = sink_;
    filter = filter_;
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

    Speakers filter_spk = filter->get_output();
    if (filter_spk.is_unknown())
      return true;

    return sink->open(filter_spk);
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
          THROW(Error());
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
          THROW(Error());
      }
      sink->process(out);
    }
    sink->flush();
  }

  virtual bool is_open() const
  {
    if (!sink) return false;
    if (filter) return filter->is_open() && sink->is_open();
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
