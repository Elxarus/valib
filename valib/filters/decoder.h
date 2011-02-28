/*
  Universal audio decoder
*/

#ifndef VALIB_DECODER_H
#define VALIB_DECODER_H

#include "parser_filter2.h"

#include "../parsers/aac/aac_adts_header.h"
#include "../parsers/ac3/ac3_header.h"
#include "../parsers/dts/dts_header.h"
#include "../parsers/mpa/mpa_header.h"

#include "../parsers/aac/aac_parser.h"
#include "../parsers/ac3/ac3_parser.h"
#include "../parsers/dts/dts_parser.h"
#include "../parsers/mpa/mpa_parser.h"

#include "parser_filter.h"
#include "../parsers/multi_frame.h"

class AudioDecoder : public ParserFilter2
{
public:
  AACParser aac;
  AC3Parser ac3;
  DTSParser dts;
  MPAParser mpa;

  AudioDecoder()
  {
    add(&adts_header, &aac);
    add(&ac3_header, &ac3);
    add(&dts_header, &dts);

    // Thunk for old interface (FrameParser)
    FrameParser *parsers[] = { &mpa };
    uni_frame.set_parsers(parsers, array_size(parsers));
    uni_parser.set_parser(&uni_frame);
    add(0, &uni_parser);
  }

protected:
  MultiFrame uni_frame;
  ParserFilter uni_parser;
};

#endif
