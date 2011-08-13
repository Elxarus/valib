#include "multi_header.h"

MultiHeader::MultiHeader()
{
  f_header_size = 0;
  f_min_frame_size = 0;
  f_max_frame_size = 0;
  p = 0;
  n = 0;
}

MultiHeader::MultiHeader(const list_t &new_parsers)
{
  set_parsers(new_parsers);
}

MultiHeader::MultiHeader(const HeaderParser *const *new_parsers, size_t nparsers)
{
  set_parsers(new_parsers, nparsers);
}

void
MultiHeader::update()
{
  size_t i;
  f_sync_trie.clear();
  f_header_size = 0;
  f_min_frame_size = 0;
  f_max_frame_size = 0;
  p = 0;
  n = 0;

  if (parsers.size() > 0)
  {
    f_sync_trie = parsers[0]->sync_trie();
    f_header_size = parsers[0]->header_size();
    f_min_frame_size = parsers[0]->min_frame_size();
    f_max_frame_size = parsers[0]->max_frame_size();

    // Speed hack: direct array access is MUCH faster in debug mode
    // parse_header() is time critical function and test time desreases ~10times
    p = &parsers[0];
    n = parsers.size();
  }

  for (i = 1; i < parsers.size(); i++)
  {
    f_sync_trie |= parsers[i]->sync_trie();

    if (f_header_size < parsers[i]->header_size())
      f_header_size = parsers[i]->header_size();

    if (f_min_frame_size > parsers[i]->min_frame_size())
      f_min_frame_size = parsers[i]->min_frame_size();

    if (f_max_frame_size < parsers[i]->max_frame_size())
      f_max_frame_size = parsers[i]->max_frame_size();
  }

  if (f_min_frame_size < f_header_size)
    f_min_frame_size = f_header_size;
}

void
MultiHeader::set_parsers(const list_t &new_parsers)
{
  parsers = new_parsers;
  parsers.erase(
    std::remove(parsers.begin(), parsers.end(), static_cast<const HeaderParser *>(0)),
    parsers.end());
  update();
}

MultiHeader::list_t
MultiHeader::get_parsers() const
{
  return parsers;
}

void
MultiHeader::set_parsers(const HeaderParser *const *new_parsers, size_t nparsers)
{
  set_parsers(list_t(new_parsers, new_parsers + nparsers));
}

void
MultiHeader::add_parser(const HeaderParser *parser)
{
  if (parser)
  {
    parsers.push_back(parser);
    update();
  }
}

void
MultiHeader::remove_parser(const HeaderParser *parser)
{
  parsers.erase(
    std::remove(parsers.begin(), parsers.end(), parser),
    parsers.end());
  update();
}

void
MultiHeader::release_parsers()
{
  parsers.clear();
  f_header_size = 0;
  f_min_frame_size = 0;
  f_max_frame_size = 0;
  p = 0;
  n = 0;
}

bool
MultiHeader::can_parse(int format) const
{
  for (size_t i = 0; i < n; i++)
    if (parsers[i]->can_parse(format))
      return true;
  return false;
}

bool
MultiHeader::parse_header(const uint8_t *hdr, HeaderInfo *hinfo) const
{
  if (!n || hdr == 0) return false;
  for (size_t i = 0; i < n; i++)
    if (p[i]->parse_header(hdr, hinfo))
      return true;
  return false;
}

bool
MultiHeader::compare_headers(const uint8_t *hdr1, const uint8_t *hdr2) const
{
  if (!n) return false;
  for (size_t i = 0; i < n; i++)
    if (p[i]->compare_headers(hdr1, hdr2))
      return true;
  return false;
}

string
MultiHeader::header_info(const uint8_t *hdr) const
{
  if (!n) return string();
  for (size_t i = 0; i < n; i++)
    if (p[i]->parse_header(hdr))
      return p[i]->header_info(hdr);
  return string();
}
