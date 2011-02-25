#include "multi_header.h"

MultiHeader::MultiHeader()
{
  f_header_size = 0;
  f_min_frame_size = 0;
  f_max_frame_size = 0;
}

MultiHeader::MultiHeader(const list_t &new_parsers)
{
  f_header_size = 0;
  f_min_frame_size = 0;
  f_max_frame_size = 0;
  set_parsers(new_parsers);
}

MultiHeader::MultiHeader(const HeaderParser *const *new_parsers, size_t nparsers)
{
  f_header_size = 0;
  f_min_frame_size = 0;
  f_max_frame_size = 0;
  set_parsers(new_parsers, nparsers);
}

void
MultiHeader::set_parsers(const list_t &new_parsers)
{
  f_header_size = 0;
  f_min_frame_size = 0;
  f_max_frame_size = 0;
  parsers = new_parsers;

  if (parsers.size() > 0)
  {
    f_header_size = parsers[0]->header_size();
    f_min_frame_size = parsers[0]->min_frame_size();
    f_max_frame_size = parsers[0]->max_frame_size();
  }

  for (size_t i = 1; i < parsers.size(); i++)
  {
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
MultiHeader::release_parsers()
{
  f_header_size = 0;
  f_min_frame_size = 0;
  f_max_frame_size = 0;
  parsers.clear();
}

bool
MultiHeader::can_parse(int format) const
{
  for (size_t i = 0; i < parsers.size(); i++)
    if (parsers[i]->can_parse(format))
      return true;
  return false;
}

bool
MultiHeader::parse_header(const uint8_t *hdr, HeaderInfo *hinfo) const
{
  for (size_t i = 0; i < parsers.size(); i++)
    if (parsers[i]->parse_header(hdr, hinfo))
      return true;
  return false;
}

bool
MultiHeader::compare_headers(const uint8_t *hdr1, const uint8_t *hdr2) const
{
  for (size_t i = 0; i < parsers.size(); i++)
    if (parsers[i]->compare_headers(hdr1, hdr2))
      return true;
  return false;
}

string
MultiHeader::header_info(const uint8_t *hdr) const
{
  for (size_t i = 0; i < parsers.size(); i++)
    if (parsers[i]->parse_header(hdr))
      return parsers[i]->header_info(hdr);
  return string();
}
