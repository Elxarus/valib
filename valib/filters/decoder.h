/*
  AudioDecoder
  Universal audio decoder
*/

#ifndef VALIB_DECODER_H
#define VALIB_DECODER_H

#include "parser_filter.h"
#include "../parsers/mpa/mpa_parser.h"
#include "../parsers/ac3/ac3_parser.h"
#include "../parsers/dts/dts_parser.h"
#include "../parsers/multi_frame.h"


class AudioDecoder : public FilterWrapper
{
protected:
  ParserFilter parser;

  MPAParser mpa;
  AC3Parser ac3;
  DTSParser dts;
  MultiFrame multi_parser;

public:
  AudioDecoder(): FilterWrapper(&parser)
  {
    FrameParser *parsers[] = { &ac3, &dts, &mpa };
    multi_parser.set_parsers(parsers, array_size(parsers));
    parser.set_parser(&multi_parser);
  }

  int        get_frames()  const { return parser.get_frames();  }
  int        get_errors()  const { return parser.get_errors();  }
  string     info()        const { return parser.info();        }
  HeaderInfo header_info() const { return parser.header_info(); }
};

#endif
