#ifndef VALIB_SPDIFABLE_HEADER_H
#define VALIB_SPDIFABLE_HEADER_H

#include "../multi_header.h"
#include "../ac3/ac3_header.h"
#include "../dts/dts_header.h"
#include "../mpa/mpa_header.h"

class SpdifableFrameParser : public MultiFrameParser
{
public:
  AC3FrameParser   ac3;
  DTSFrameParser   dts;
  MPAFrameParser   mpa;

  SpdifableFrameParser()
  {
    FrameParser *parsers[] = { &ac3, &dts, &mpa };
    set_parsers(parsers, array_size(parsers));
  }

  inline const FrameParser *find_parser(int spdif_type) const
  {
    switch (spdif_type)
    {
      // AC3
      case 1: return &ac3;
      // MPA
      case 4:
      case 5:
      case 8:
      case 9: return &mpa;
      // DTS
      case 11:
      case 12:
      case 13: return &dts;
    }
    return 0;
  }

  inline FrameParser *find_parser(int spdif_type)
  {
    switch (spdif_type)
    {
      // AC3
      case 1: return &ac3;
      // MPA
      case 4:
      case 5:
      case 8:
      case 9: return &mpa;
      // DTS
      case 11:
      case 12:
      case 13: return &dts;
    }
    return 0;
  }
};

///////////////////////////////////////////////////////////////////////////////

#include "../../parser.h"
const HeaderParser *spdifable_header();

#endif
