/*
  Wrap/unwrap encoded stream to/from SPDIF according to IEC 61937

  Input:     SPDIF, AC3, MPA, DTS, Unknown
  Output:    SPDIF, DTS
  OFDD:      yes
  Buffering: block
  Timing:    first syncpoint
  Parameters:
    -
*/

#ifndef VALIB_SPDIFER_H
#define VALIB_SPDIFER_H

#include "parser_filter.h"
#include "../parsers/spdif/spdif_wrapper.h"
#include "../parsers/spdif/spdif_parser.h"



class Spdifer : public Filter2
{
protected:
  ParserFilter parser;
  SPDIFWrapper spdif_wrapper;

public:
  Spdifer()
  {
    parser.set_parser(&spdif_wrapper);
  }

  /////////////////////////////////////////////////////////
  // Spdifer interface

  int        get_dts_mode()                  const { return spdif_wrapper.dts_mode;       }
  void       set_dts_mode(int dts_mode)            { spdif_wrapper.dts_mode = dts_mode;   }

  int        get_dts_conv()                  const { return spdif_wrapper.dts_conv;       }
  void       set_dts_conv(int dts_conv)            { spdif_wrapper.dts_conv = dts_conv;   }

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
  virtual bool is_ofdd() const              { return parser.is_ofdd();     }
  virtual bool is_inplace() const           { return parser.is_inplace();  }
  virtual Speakers get_input() const        { return parser.get_input();   }

  /////////////////////////////////////////////////////////
  // Processing

  virtual bool process(Chunk2 &in, Chunk2 &out) { return parser.process(in, out); }
  virtual bool flush(Chunk2 &out)           { return parser.flush(out);           }
  virtual void reset()                      { parser.reset();                     }

  virtual bool new_stream() const           { return parser.new_stream();    }
  virtual bool need_flushing() const        { return parser.need_flushing(); }
  virtual Speakers get_output() const       { return parser.get_output();    }
};

class Despdifer : public Filter2
{
protected:
  ParserFilter parser;
  SPDIFParser  spdif_parser;

public:
  Despdifer(): spdif_parser(true)
  {
    parser.set_parser(&spdif_parser);
  }

  bool get_big_endian() const           { return spdif_parser.get_big_endian();     }
  void set_big_endian(bool _big_endian) { spdif_parser.set_big_endian(_big_endian); }

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
  virtual bool is_ofdd() const              { return parser.is_ofdd();     }
  virtual bool is_inplace() const           { return parser.is_inplace();  }
  virtual Speakers get_input() const        { return parser.get_input();   }

  /////////////////////////////////////////////////////////
  // Processing

  virtual bool process(Chunk2 &in, Chunk2 &out) { return parser.process(in, out); }
  virtual bool flush(Chunk2 &out)           { return parser.flush(out);           }
  virtual void reset()                      { parser.reset();                     }

  virtual bool new_stream() const           { return parser.new_stream();    }
  virtual bool need_flushing() const        { return parser.need_flushing(); }
  virtual Speakers get_output() const       { return parser.get_output();    }
};

#endif
