/**************************************************************************//**
  \file spdif_parser.h
  \brief SPDIFParser: Converts SPDIF stream to raw AC3/MPA/DTS stream.
******************************************************************************/

#ifndef VALIB_SPDIF_PARSER_H
#define VALIB_SPDIF_PARSER_H

#include "../../filter.h"
#include "../../parser.h"
#include "spdifable_header.h"

/**************************************************************************//**
  \class SPDIFParser
  \brief Converts SPDIF stream to raw AC3/MPA/DTS stream.

  Accepts only chunks with exactly one frame.

  By default, it converts the stream from little endian format (SPDIF stream
  always contains) to big endian. You can disable this behavior and keep little
  enddian stream unchanged.

  SPDIFWrapper produces two types of SPDIF stream: wrapped and padded DTS. This
  class supports both.

  Normally, the size of original frame is saved at SPDIF header, so the original
  stream may be reconstructed prcisely. But padded DTS does not have SPDIF
  header and frame size at the DTS header may be incorrect. Therefore, it is
  impossible to determine the correct frame size and it remains unchanged.
  I.e. SPDIFParser just passes the padded DTS without precise reconstruction of
  an original stream.

  The change of the contained stream is detected and output stream change
  produced.

  \fn bool SPDIFParser::get_big_endian() const
    Returns big_endian option.

  \fn void SPDIFParser::set_big_endian(bool big_endian)
    \param big_endian Convert to big endian option.

    Sets big_endian option. When set to true SPDIFParser converts the contained
    stream to big endian format. Otherwise it keeps the little endian format
    unchanged.

  \fn FrameInfo SPDIFParser::frame_info() const
    Returns the info about the last frame.

******************************************************************************/

class SPDIFParser : public SimpleFilter
{
public:
  SPDIFParser(bool big_endian = true);

  /////////////////////////////////////////////////////////
  // Own interface

  bool get_big_endian() const           { return big_endian;        }
  void set_big_endian(bool _big_endian) { big_endian = _big_endian; }

  FrameInfo frame_info() const          { return finfo; }

  /////////////////////////////////////////////////////////
  // SimpleFilter overrides

  bool can_open(Speakers spk) const;
  bool init();

  void reset();
  bool process(Chunk &in, Chunk &out);

  bool new_stream() const
  { return new_stream_flag; }

  Speakers get_output() const
  { return finfo.spk; }

  string info() const;

protected:
  SpdifableFrameParser spdifable_parser;

  bool big_endian;
  FrameInfo finfo;
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
