/*
  Universal audio decoder
*/

#ifndef VALIB_DECODER_H
#define VALIB_DECODER_H

#include "parser_filter.h"

#include "../parsers/uni/uni_frame_parser.h"

#include "../parsers/aac/aac_parser.h"
#include "../parsers/ac3/ac3_parser.h"
#include "../parsers/dts/dts_parser.h"
#include "../parsers/eac3/eac3_parser.h"
#include "../parsers/flac/flac_parser.h"
#include "../parsers/vorbis/vorbis_parser.h"
#include "../parsers/mpa/mpa_mpg123.h"

class AudioDecoder : public ParserFilter
{
public:
  UniFrameParser uni_frame_parser;
  AACParser    aac;
  AC3Parser    ac3;
  DTSParser    dts;
  EAC3Parser   eac3;
  FlacParser   flac;
  VorbisParser vorbis;
  MPG123Parser mpa;

  AudioDecoder()
  {
    add(&uni_frame_parser.ac3,  &ac3);
    add(&uni_frame_parser.eac3, &eac3);
    add(&uni_frame_parser.dts,  &dts);
    add(&uni_frame_parser.mpa,  &mpa);
    add(&uni_frame_parser.dolby);
    add(&aac);
    add(&flac);
    add(&vorbis);
  }
};

#endif
