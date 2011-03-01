/**************************************************************************//**
  \file spdif_parser.h
  \brief SPDIFParser: Converts SPDIF stream to raw AC3/MPA/DTS stream.
******************************************************************************/

#ifndef VALIB_SPDIF_PARSER_H
#define VALIB_SPDIF_PARSER_H

#include "../../filter.h"
#include "../../parser.h"

/**************************************************************************//**
  \class SPDIFParser
  \brief Converts SPDIF stream to raw AC3/MPA/DTS stream.


******************************************************************************/

class SPDIFParser : public SimpleFilter
{
public:
  SPDIFParser(bool big_endian = true);

  /////////////////////////////////////////////////////////
  // Own interface

  bool get_big_endian() const           { return big_endian;        }
  void set_big_endian(bool _big_endian) { big_endian = _big_endian; }

  HeaderInfo header_info() const        { return hinfo; }

  /////////////////////////////////////////////////////////
  // SimpleFilter overrides

  bool can_open(Speakers spk) const;
  bool init();

  void reset();
  bool process(Chunk &in, Chunk &out);

  bool new_stream() const
  { return new_stream_flag; }

  Speakers get_output() const
  { return hinfo.spk; }

  string info() const;

protected:
  bool big_endian;

  Rawdata     header;
  HeaderInfo  hinfo;
  bool new_stream_flag;

  struct spdif_header_s
  {
    uint16_t zero1;
    uint16_t zero2;
    uint16_t zero3;
    uint16_t zero4;

    uint16_t sync1;   // Pa sync word 1
    uint16_t sync2;   // Pb sync word 2
    uint16_t type;    // Pc data type
    uint16_t len;     // Pd length-code (bits)
  };

};

#endif
