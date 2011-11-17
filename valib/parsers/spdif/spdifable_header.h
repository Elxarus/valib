#ifndef VALIB_SPDIFABLE_HEADER_H
#define VALIB_SPDIFABLE_HEADER_H

#include "../multi_header.h"
#include "../ac3/ac3_header.h"
#include "../dts/dts_header.h"
#include "../eac3/eac3_header.h"
#include "../mpa/mpa_header.h"

class SpdifableFrameParser : public MultiFrameParser
{
public:
  AC3FrameParser   ac3;
  DTSFrameParser   dts;
  EAC3FrameParser  eac3;
  MPAFrameParser   mpa;

  SpdifableFrameParser()
  {
    FrameParser *parsers[] = { &ac3, &dts, &eac3, &mpa };
    set_parsers(parsers, array_size(parsers));
  }
};

#include "../../parser.h"
const HeaderParser *spdifable_header();

#endif
