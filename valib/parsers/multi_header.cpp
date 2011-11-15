#include "multi_header.h"

MultiFrameParser::MultiFrameParser()
{
  p = 0;
  n = 0;
  max_header_size = 0;
  parser = 0;
}

MultiFrameParser::MultiFrameParser(const list_t &new_parsers)
{
  set_parsers(new_parsers);
}

MultiFrameParser::MultiFrameParser(FrameParser *const *new_parsers, size_t nparsers)
{
  set_parsers(new_parsers, nparsers);
}

void
MultiFrameParser::update()
{
  size_t i;

  sinfo = SyncInfo();
  max_header_size = 0;
  p = 0;
  n = 0;

  if (parsers.size() > 0)
  {
    sinfo = parsers[0]->sync_info();
    max_header_size = parsers[0]->header_size();

    // Speed hack: direct array access is MUCH faster in debug mode
    // parse_header() is time critical function and test time desreases ~10times
    p = &parsers[0];
    n = parsers.size();
  }

  for (i = 1; i < parsers.size(); i++)
  {
    SyncInfo new_sinfo = parsers[i]->sync_info();
    sinfo.sync_trie |= new_sinfo.sync_trie;

    if (max_header_size < parsers[i]->header_size())
      max_header_size = parsers[i]->header_size();

    if (sinfo.min_frame_size > new_sinfo.min_frame_size)
      sinfo.min_frame_size = new_sinfo.min_frame_size;

    if (sinfo.max_frame_size < new_sinfo.max_frame_size)
      sinfo.max_frame_size = new_sinfo.max_frame_size;
  }

  sinfo.sync_trie.optimize();
}

void
MultiFrameParser::set_parsers(const list_t &new_parsers)
{
  parsers = new_parsers;
  parsers.erase(
    std::remove(parsers.begin(), parsers.end(), static_cast<FrameParser *>(0)),
    parsers.end());
  update();
}

MultiFrameParser::list_t
MultiFrameParser::get_parsers() const
{
  return parsers;
}

void
MultiFrameParser::set_parsers(FrameParser *const *new_parsers, size_t nparsers)
{
  set_parsers(list_t(new_parsers, new_parsers + nparsers));
}

void
MultiFrameParser::add_parser(FrameParser *parser)
{
  if (parser)
  {
    parsers.push_back(parser);
    update();
  }
}

void
MultiFrameParser::remove_parser(FrameParser *parser)
{
  parsers.erase(
    std::remove(parsers.begin(), parsers.end(), parser),
    parsers.end());
  update();
}

void
MultiFrameParser::release_parsers()
{
  parsers.clear();
  p = 0;
  n = 0;

  sinfo = SyncInfo();
  max_header_size = 0;
}



bool
MultiFrameParser::can_parse(int format) const
{
  for (size_t i = 0; i < n; i++)
    if (parsers[i]->can_parse(format))
      return true;
  return false;
}

SyncInfo
MultiFrameParser::sync_info() const
{
  return sinfo;
}

SyncInfo
MultiFrameParser::sync_info2() const
{
  return parser? parser->sync_info(): sinfo;
}

size_t
MultiFrameParser::header_size() const
{
  return max_header_size;
}

bool
MultiFrameParser::parse_header(const uint8_t *hdr, FrameInfo *finfo) const
{
  if (!n || hdr == 0) return false;
  for (size_t i = 0; i < n; i++)
    if (p[i]->parse_header(hdr, finfo))
      return true;
  return false;
}

bool
MultiFrameParser::compare_headers(const uint8_t *hdr1, const uint8_t *hdr2) const
{
  if (!n) return false;
  for (size_t i = 0; i < n; i++)
    if (p[i]->compare_headers(hdr1, hdr2))
      return true;
  return false;
}

bool
MultiFrameParser::first_frame(const uint8_t *frame, size_t size)
{
  if (!n) return false;

  reset();
  for (size_t i = 0; i < n; i++)
    if (p[i]->first_frame(frame, size))
    {
      parser = p[i];
      return true;
    }

  return false;
}

bool
MultiFrameParser::next_frame(const uint8_t *frame, size_t size)
{
  return parser? parser->next_frame(frame, size): false;
}

void
MultiFrameParser::reset()
{
  if (parser)
  {
    parser->reset();
    parser = 0;
  }
}

bool
MultiFrameParser::in_sync() const
{
  return parser? parser->in_sync(): false;
}

FrameInfo
MultiFrameParser::frame_info() const
{
  return parser? parser->frame_info(): FrameInfo();
}

string
MultiFrameParser::stream_info() const
{
  return parser? parser->stream_info(): string();
}

///////////////////////////////////////////////////////////////////////////////


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
