#ifndef UNI_FRAME_PARSER_H
#define UNI_FRAME_PARSER_H

#include "../multi_header.h"
#include "../aac/aac_adts_header.h"
#include "../ac3/ac3_header.h"
#include "../dts/dts_header.h"
#include "../eac3/eac3_header.h"
#include "../mpa/mpa_header.h"
#include "../spdif/spdif_header.h"

class UniFrameParser : public MultiFrameParser
{
public:
  ADTSFrameParser  adts;
  AC3FrameParser   ac3;
  DTSFrameParser   dts;
  EAC3FrameParser  eac3;
  MPAFrameParser   mpa;
  SPDIFFrameParser spdif;

  UniFrameParser()
  {
    FrameParser *parsers[] = { &adts, &ac3, &dts, &eac3, &mpa, &spdif };
    set_parsers(parsers, array_size(parsers));
  }

  FrameParser *find_parser(int format)
  {
    switch (format)
    {
      case FORMAT_RAWDATA: return this;
      case FORMAT_AAC_ADTS:return &adts;
      case FORMAT_AC3:     return &ac3;
      case FORMAT_DTS:     return &dts;
      case FORMAT_EAC3:    return &eac3;
      case FORMAT_MPA:     return &mpa;
      case FORMAT_SPDIF:   return &spdif;
    }
    return 0;
  }
};

#endif
