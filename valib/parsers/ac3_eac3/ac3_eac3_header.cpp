#include "../ac3/ac3_header.h"
#include "../eac3/eac3_header.h"
#include "../multi_header.h"
#include "ac3_eac3_header.h"

static const HeaderParser *ac3_eac3_parsers[] = 
{ &ac3_header, &eac3_header };

class AC3_EAC3_Header : public MultiHeader
{
public:
  AC3_EAC3_Header():
  MultiHeader(ac3_eac3_parsers, array_size(ac3_eac3_parsers))
  {}

  bool can_parse(int format) const
  {
    return format == FORMAT_AC3 ||
           format == FORMAT_EAC3 ||
           format == FORMAT_AC3_EAC3;
  }
};

const HeaderParser *ac3_eac3_header()
{
  static const AC3_EAC3_Header header;
  return &header;
};
