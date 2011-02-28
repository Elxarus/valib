#include "parser_filter2.h"

void
ParserFilter2::add(const HeaderParser *new_header, Filter *new_filter)
{
  header.add_parser(new_header);
  filter.add_filter(new_filter);
  sync.set_parser(&header);
}

void
ParserFilter2::add(const HeaderParser *new_header)
{
  header.add_parser(new_header);
  sync.set_parser(&header);
}

void
ParserFilter2::add(Filter *new_filter)
{
  filter.add_filter(new_filter);
}

void
ParserFilter2::release()
{
  header.release_parsers();
  filter.release_filters();
  sync.set_parser(&header);
}

/////////////////////////////////////////////////////////
// FilterGraph overrides

int
ParserFilter2::next_id(int id, Speakers spk) const
{
  switch (id)
  {
    case node_start:
      if (sync.can_open(spk))
        return node_sync;

      if (filter.can_open(spk))
        return node_filter;

      return node_err;

    case node_sync:
      if (filter.can_open(spk))
        return node_filter;

      return node_err;
  
    case node_filter:
      return node_end;
  }
  return node_err;
}

Filter *
ParserFilter2::init_filter(int id, Speakers spk)
{
  switch (id)
  {
    case node_sync:   return &sync;
    case node_filter: return &filter;
  }
  return 0;
}
