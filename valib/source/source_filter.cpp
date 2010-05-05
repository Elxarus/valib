#include "source_filter.h"

bool
SourceFilter2::set(Source *source_, Filter *filter_)
{
  if (!source_)
    return false;

  if (filter_)
    if (!source_->get_output().is_unknown())
      if (!filter_->open(source_->get_output()))
        return false;

  source = source_;
  filter = filter_;
  is_new_stream = false;
  format_change = false;
  state = state_empty;

  if (filter)
    filter->reset();
  return true;
}

void
SourceFilter2::release()
{
  source = 0;
  filter = 0;
}

void
SourceFilter2::reset()
{
  if (!source) return;

  source->reset();
  if (filter)
    filter->reset();

  is_new_stream = false;
  format_change = false;
  state = state_empty;
}

/////////////////////////////////////////////////////////
// Source interface

bool
SourceFilter2::get_chunk(Chunk &out)
{
  if (!source) return false;
  if (!filter) return source->get_chunk(out);

  bool processing;

  while (true)
  {
    switch (state)
    {
      case state_empty:
      {
        if (!source->get_chunk(chunk))
        {
          // end of stream
          processing = filter->flush(out);
          is_new_stream = filter->new_stream();
          is_new_stream |= format_change;
          format_change = false;
          return processing;
        }

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
        if (!filter->is_open())
          if (!filter->open(source->get_output()))
            throw SourceError(this, 0, "Cannot open the filter");

        if (filter->process(chunk, out))
        {
          is_new_stream = filter->new_stream();
          is_new_stream |= format_change;
          format_change = false;
          return true;
        }

        // no more data
        state = state_empty;
        continue;
      }

      case state_new_stream:
      {
        if (filter->flush(out))
        {
          is_new_stream = filter->new_stream();
          is_new_stream |= format_change;
          format_change = false;
          return true;
        }

        if (!filter->open(source->get_output()))
          throw SourceError(this, 0, "Cannot reopen the filter");

        filter->reset();

        format_change = true;
        state = state_filter;
        continue;
      }

      default:
        assert(false);
    }
  }
}

bool
SourceFilter2::new_stream() const
{
  if (!source) return false;
  if (!filter) return source->new_stream();
  return is_new_stream;
}

Speakers
SourceFilter2::get_output() const
{
  if (!source) return spk_unknown;
  if (!filter) return source->get_output();
  return filter->get_output();
}
