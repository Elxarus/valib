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


class AudioDecoder : public Filter
{
protected:
  ParserFilter parser;

  MPAParser mpa;
  AC3Parser ac3;
  DTSParser dts;
  MultiFrame multi_parser;

public:
  AudioDecoder()
  {
    FrameParser *parsers[] = { &ac3, &dts, &mpa };
    multi_parser.set_parsers(parsers, array_size(parsers));
    parser.set_parser(&multi_parser);
  }

  int        get_frames()                    const { return parser.get_frames();       }
  int        get_errors()                    const { return parser.get_errors();       }

  size_t     get_info(char *buf, size_t len) const { return parser.get_info(buf, len); }
  HeaderInfo header_info()                   const { return parser.header_info();      }

  /////////////////////////////////////////////////////////
  // Open/close the filter

  virtual bool can_open(Speakers spk) const { return parser.can_open(spk); }
  virtual bool open(Speakers spk)           { return parser.open(spk);     }
  virtual void close()                      { parser.close();              }
  virtual bool is_open() const              { return parser.is_open();     }

  /////////////////////////////////////////////////////////
  // Processing

  virtual bool process(Chunk &in, Chunk &out) { return parser.process(in, out); }
  virtual bool flush(Chunk &out)           { return parser.flush(out);           }
  virtual void reset()                      { parser.reset();                     }

  virtual bool new_stream() const           { return parser.new_stream();    }

  virtual Speakers get_input() const        { return parser.get_input();   }
  virtual Speakers get_output() const       { return parser.get_output();    }
};

#endif
