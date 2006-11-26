/*
  Encapsulates encoded stream into SPDIF according to IEC 61937

  Input:     SPDIF, AC3, MPA, DTS, Unknown
  Output:    SPDIF, DTS
  OFDD:      yes
  Buffering: block
  Timing:    first syncpoint
  Parameters:
    -
*/

#ifndef SPDIFER_H
#define SPDIFER_H

#include "filters\parser_filter.h"
#include "parsers\spdif_wrapper.h"
#include "parsers\spdif_frame.h"



class Spdifer : public Filter
{
protected:
  ParserFilter parser;
  SPDIFWrapper spdif_wrapper;

public:
  Spdifer()
  {
    parser.set_parser(&spdif_wrapper);
  }

  int        get_frames()                    const { return parser.get_frames();       }
  int        get_errors()                    const { return parser.get_errors();       }

  size_t     get_info(char *buf, size_t len) const { return parser.get_info(buf, len); }
  HeaderInfo header_info()                   const { return parser.header_info();      }

  /////////////////////////////////////////////////////////
  // Filter interface

  virtual void reset()                     { parser.reset();                 }
  virtual bool is_ofdd() const             { return parser.is_ofdd();        }

  virtual bool query_input(Speakers spk) const { return parser.query_input(spk); }
  virtual bool set_input(Speakers spk)     { return parser.set_input(spk);   }
  virtual Speakers get_input() const       { return parser.get_input();      }
  virtual bool process(const Chunk *chunk) { return parser.process(chunk);   }

  virtual Speakers get_output() const      { return parser.get_output();     }
  virtual bool is_empty() const            { return parser.is_empty();       }
  virtual bool get_chunk(Chunk *chunk)     { return parser.get_chunk(chunk); }
};

class Despdifer : public Filter
{
protected:
  ParserFilter parser;
  SPDIFFrame   spdif_parser;

public:
  Despdifer()
  {
    parser.set_parser(&spdif_parser);
  }

  int        get_frames()                    const { return parser.get_frames();       }
  int        get_errors()                    const { return parser.get_errors();       }

  size_t     get_info(char *buf, size_t len) const { return parser.get_info(buf, len); }
  HeaderInfo header_info()                   const { return parser.header_info();      }

  /////////////////////////////////////////////////////////
  // Filter interface

  virtual void reset()                     { parser.reset();                 }
  virtual bool is_ofdd() const             { return parser.is_ofdd();        }

  virtual bool query_input(Speakers spk) const { return parser.query_input(spk); }
  virtual bool set_input(Speakers spk)     { return parser.set_input(spk);   }
  virtual Speakers get_input() const       { return parser.get_input();      }
  virtual bool process(const Chunk *chunk) { return parser.process(chunk);   }

  virtual Speakers get_output() const      { return parser.get_output();     }
  virtual bool is_empty() const            { return parser.is_empty();       }
  virtual bool get_chunk(Chunk *chunk)     { return parser.get_chunk(chunk); }
};

#endif
