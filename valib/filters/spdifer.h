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

#include "parser_filter2.h"
#include "../parsers/spdif/spdif_wrapper.h"
#include "../parsers/spdif/spdif_header.h"
#include "../parsers/spdif/spdif_parser.h"



class Spdifer : public ParserFilter2
{
protected:
  SPDIFWrapper spdif_wrapper;
  MultiHeader spdifable;

public:
  Spdifer()
  {
    const HeaderParser *parsers[] = { &mpa_header, &ac3_header, &dts_header };
    spdifable.set_parsers(parsers, array_size(parsers));
    add(&spdifable, &spdif_wrapper);
  }

  /////////////////////////////////////////////////////////
  // Spdifer interface

  int        get_dts_mode()                  const { return spdif_wrapper.dts_mode;       }
  void       set_dts_mode(int dts_mode)            { spdif_wrapper.dts_mode = dts_mode;   }

  int        get_dts_conv()                  const { return spdif_wrapper.dts_conv;       }
  void       set_dts_conv(int dts_conv)            { spdif_wrapper.dts_conv = dts_conv;   }

  string     info()                          const { return spdif_wrapper.info();         }
  HeaderInfo header_info()                   const { return spdif_wrapper.header_info();  }
};

class Despdifer : public ParserFilter2
{
protected:
  SPDIFParser  spdif_parser;

public:
  Despdifer(): spdif_parser(true)
  {
    add(&spdif_header, &spdif_parser);
  }

  bool get_big_endian() const           { return spdif_parser.get_big_endian();     }
  void set_big_endian(bool _big_endian) { spdif_parser.set_big_endian(_big_endian); }
};

#endif
