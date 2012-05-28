#include "parser_filter.h"

ParserFilter::ParserFilter()
{}

ParserFilter::ParserFilter(FrameParser *frame_parser_, Filter *filter_)
{
  add(frame_parser_, filter_);
}

void
ParserFilter::add(FrameParser *frame_parser_, Filter *filter_)
{
  list.push_back(Entry(frame_parser_, filter_));
}

void
ParserFilter::add(Filter *filter_)
{
  list.push_back(Entry(0, filter_));
}

void
ParserFilter::release()
{
  list.clear();
}

/////////////////////////////////////////////////////////
// Filter overrides

bool
ParserFilter::can_open(Speakers spk) const
{
  for (size_t i = 0; i < list.size(); i++)
    if ((list[i].frame_parser && list[i].frame_parser->can_parse(spk.format)) ||
        (list[i].filter && list[i].filter->can_open(spk)))
      return true;
  return false;
}

bool
ParserFilter::open(Speakers spk)
{
  for (size_t i = 0; i < list.size(); i++)
    if ((list[i].frame_parser && list[i].frame_parser->can_parse(spk.format)) ||
        (list[i].filter && list[i].filter->can_open(spk)))
    {
      frame_parser = list[i].frame_parser;
      filter = list[i].filter;
      sync.set_parser(frame_parser);
      return FilterGraph::open(spk);
    }

  if (is_open())
    close();
  return false;
}

/////////////////////////////////////////////////////////
// FilterGraph overrides

int
ParserFilter::next_id(int id, Speakers spk) const
{
  switch (id)
  {
    case node_start:
      if (frame_parser)
        return node_sync;
      return node_filter;

    case node_sync:
      return node_filter;
  
    case node_filter:
      return node_end;
  }
  return node_err;
}

Filter *
ParserFilter::init_filter(int id, Speakers spk)
{
  switch (id)
  {
    case node_sync:   return &sync;
    case node_filter: return filter;
  }
  return 0;
}
