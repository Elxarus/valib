#ifndef VALIB_DOLBY_HEADER_H
#define VALIB_DOLBY_HEADER_H

#include "../../parser.h"
#include "../ac3/ac3_header.h"
#include "../eac3/eac3_header.h"
#include "../multi_header.h"

class DolbyFrameParser : public MultiFrameParser
{
public:
  AC3FrameParser ac3;
  EAC3FrameParser eac3;

  DolbyFrameParser()
  {
    FrameParser *parsers[] = { &ac3, &eac3 };
    set_parsers(parsers, array_size(parsers));
  }

  bool can_parse(int format) const
  {
    return format == FORMAT_AC3 ||
           format == FORMAT_EAC3 ||
           format == FORMAT_DOLBY;
  }
};

#endif
