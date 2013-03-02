/*
  Demux - extract raw audio stream from a container stream.
  Currently supported:
  * ADTS
  * MPEG PES
  * SPDIF
*/

#ifndef VALIB_DEMUX_H
#define VALIB_DEMUX_H

#include "parser_filter.h"
#include "../parsers/aac/aac_adts_header.h"
#include "../parsers/aac/aac_adts_parser.h"
#include "../parsers/pes/pes_frame_parser.h"
#include "../parsers/pes/pes_parser.h"
#include "../parsers/spdif/spdif_header.h"
#include "../parsers/spdif/spdif_parser.h"

class Demux : public ParserFilter
{
protected:
  ADTSFrameParser  adts_frame;
  PESFrameParser   pes_frame;
  SPDIFFrameParser spdif_frame;

  ADTSParser  adts_parser;
  PESParser   pes_parser;
  SPDIFParser spdif_parser;

public:
  Demux()
  {
    add(&adts_frame,  &adts_parser);
    add(&pes_frame,   &pes_parser);
    add(&spdif_frame, &spdif_parser);
  }
};

#endif
