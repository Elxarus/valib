#include "source_filter.h"

void
SourceFilter::set(Source *source_, Filter *filter_)
{
  release();

  need_flushing = false;
  if (source_ && filter_)
  {
    need_flushing = filter_->is_open();
    Speakers source_spk = source_->get_output();
    if (!source_spk.is_unknown())
      if (!filter_->is_open() || filter_->get_input() != source_->get_output())
      {
        filter_->open_throw(source_spk);
        need_flushing = false;
      }
  }

  source = source_;
  filter = filter_;
  is_new_stream = false;
  format_change = false;
  state = state_empty;
}

void
SourceFilter::release()
{
  source = 0;
  filter = 0;
  is_new_stream = false;
  format_change = false;
  need_flushing = false;
  state = state_empty;
}

void
SourceFilter::reset()
{
  if (!source) return;

  source->reset();
  if (filter)
    filter->reset();

  is_new_stream = false;
  format_change = false;
  need_flushing = false;
  state = state_empty;
}

/////////////////////////////////////////////////////////
// Source interface

bool
SourceFilter::get_chunk(Chunk &out)
{
  if (!source) return false;
  if (!filter) return source->get_chunk(out);

  while (true)
  {
    switch (state)
    {
      case state_empty:
      {
        if (!source->get_chunk(chunk))
          if (need_flushing)
          {
            state = state_flush;
            continue;
          }
          else
            return false;

        if (source->new_stream())
        {
          state = state_new_stream;
          continue;
        }

        state = state_filter;
        continue;
      }

      case state_filter:
      {
        assert(filter->is_open());
        if (filter->process(chunk, out))
        {
          is_new_stream = filter->new_stream();
          is_new_stream |= format_change;
          format_change = false;
          return true;
        }
        need_flushing = true;
        state = state_empty;
        continue;
      }

      case state_new_stream:
      {
        assert(!need_flushing || filter->is_open());
        if (need_flushing && filter->flush(out))
        {
          is_new_stream = filter->new_stream();
          is_new_stream |= format_change;
          format_change = false;
          return true;
        }
        filter->open_throw(source->get_output());
        format_change = true;
        need_flushing = false;
        state = state_filter;
        continue;
      }

      case state_flush:
      {
        assert(filter->is_open());
        if (filter->flush(out))
        {
          is_new_stream = filter->new_stream();
          is_new_stream |= format_change;
          format_change = false;
          return true;
        }
        need_flushing = false;
        state = state_empty;
        continue;
      }

      default:
        assert(false);
    }
  }
}

bool
SourceFilter::new_stream() const
{
  if (!source) return false;
  if (!filter) return source->new_stream();
  return is_new_stream;
}

Speakers
SourceFilter::get_output() const
{
  if (!source) return spk_unknown;
  if (!filter) return source->get_output();
  return filter->get_output();
}
