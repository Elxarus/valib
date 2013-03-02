#ifndef UNI_FRAME_PARSER_H
#define UNI_FRAME_PARSER_H

#include "../multi_header.h"
#include "../aac/aac_adts_header.h"
#include "../ac3/ac3_header.h"
#include "../dolby/dolby_header.h"
#include "../dts/dts_header.h"
#include "../eac3/eac3_header.h"
#include "../mpa/mpa_header.h"
#include "../mlp/mlp_header.h"
#include "../spdif/spdif_header.h"

class UniFrameParser : public MultiFrameParser
{
public:
  ADTSFrameParser   adts;
  AC3FrameParser    ac3;
  DTSFrameParser    dts;
  EAC3FrameParser   eac3;
  MPAFrameParser    mpa;
  MlpFrameParser    mlp;
  TruehdFrameParser truehd;
  SPDIFFrameParser  spdif;

  // Composite parsers.
  // It's possible to use existing parsers (maybe later).
  DolbyFrameParser dolby;

  UniFrameParser()
  {
    // mlp & truehd are not included into 'all parsers' because
    // parser becomes too complicated.
    FrameParser *parsers[] = { &adts, &ac3, &dts, &eac3, &mpa, &spdif, &dolby};
    set_parsers(parsers, array_size(parsers));
  }

  FrameParser *find_parser(int format)
  {
    switch (format)
    {
      case FORMAT_RAWDATA: return this;
      case FORMAT_AAC_ADTS:return &adts;
      case FORMAT_AC3:     return &ac3;
      case FORMAT_DOLBY:   return &dolby;
      case FORMAT_DTS:     return &dts;
      case FORMAT_EAC3:    return &eac3;
      case FORMAT_MPA:     return &mpa;
      case FORMAT_SPDIF:   return &spdif;
      case FORMAT_MLP:     return &mlp;
      case FORMAT_TRUEHD:  return &truehd;
    }
    return 0;
  }
};

#endif
