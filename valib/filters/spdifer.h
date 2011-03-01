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
#include "../parsers/spdif/spdif_header.h"
#include "../parsers/spdif/spdif_parser.h"
#include "../parsers/spdif/spdifable_header.h"



class Spdifer : public ParserFilter
{
protected:
  SPDIFWrapper spdif_wrapper;

public:
  Spdifer()
  {
    add(&spdifable_header, &spdif_wrapper);
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

class Despdifer : public ParserFilter
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
