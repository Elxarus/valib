/*
  Universal audio decoder
*/

#ifndef VALIB_DECODER_H
#define VALIB_DECODER_H

#include "parser_filter.h"

#include "../parsers/aac/aac_adts_header.h"
#include "../parsers/ac3_eac3/ac3_eac3_header.h"
#include "../parsers/dts/dts_header.h"
#include "../parsers/mpa/mpa_header.h"

#include "../parsers/aac/aac_parser.h"
#include "../parsers/ac3/ac3_parser.h"
#include "../parsers/dts/dts_parser.h"
#include "../parsers/eac3/eac3_parser.h"
#include "../parsers/mpa/mpa_mpg123.h"

class AudioDecoder : public ParserFilter
{
public:
  AACParser aac;
  AC3Parser ac3;
  DTSParser dts;
  MPG123Parser mpa;
  EAC3Parser eac3;

  AudioDecoder()
  {
    add(&adts_header, &aac);
    add(0, &ac3);
    add(0, &eac3);
    add(ac3_eac3_header(), 0);
    add(&dts_header, &dts);
    add(&mpa_header, &mpa);
  }
};

#endif
