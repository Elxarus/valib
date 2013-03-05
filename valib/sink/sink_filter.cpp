#include "sink_filter.h"

SinkFilter::SinkFilter():
sink(0), filter(0)
{}

SinkFilter::SinkFilter(Sink *sink_, Filter *filter_):
sink(0), filter(0)
{ set(sink_, filter_); }

void
SinkFilter::set(Sink *sink_, Filter *filter_)
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

void
SinkFilter::release()
{
  sink = 0;
  filter = 0;
}

bool
SinkFilter::can_open(Speakers spk) const
{
  if (!sink) return false;
  if (!filter) return sink->can_open(spk);
  return filter->can_open(spk);
}

bool
SinkFilter::open(Speakers spk)
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

void
SinkFilter::close()
{
  if (!sink) return;

  sink->close();
  if (filter)
    filter->close();
}

bool
SinkFilter::is_open() const
{
  if (!sink) return false;
  if (filter) return filter->is_open() && sink->is_open();
  return sink->is_open();
}

Speakers
SinkFilter::get_input() const
{
  if (!sink) return spk_unknown;
  if (!filter) return sink->get_input();
  return filter->get_input();
}

void
SinkFilter::reset()
{
  if (!sink) return;

  sink->reset();
  if (filter)
    filter->reset();
}

void
SinkFilter::process(const Chunk &in)
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

void
SinkFilter::flush()
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
