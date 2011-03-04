/*
  SPDIF wrapper class
  Converts raw AC3/MPA/DTS stream to SPDIF stream.

  Does raw stream output if the stream given cannot be spdifed (high-bitrate
  DTS for example). Can convert DTS stream type to 14/16 bits. Supports padded
  and wrapped SPDIF stream types. Can re-wrap SPDIF stream with conversion
  between SPDIF stream types.
*/

#ifndef VALIB_SPDIF_WRAPPER_H
#define VALIB_SPDIF_WRAPPER_H

#include "../../filter.h"
#include "../../parser.h"

#define DTS_MODE_AUTO    0
#define DTS_MODE_WRAPPED 1
#define DTS_MODE_PADDED  2

#define DTS_CONV_NONE    0
#define DTS_CONV_16BIT   1
#define DTS_CONV_14BIT   2

class SPDIFWrapper : public SimpleFilter
{
public:
  int  dts_mode;
  int  dts_conv;

  SPDIFWrapper(int dts_mode = DTS_MODE_AUTO, int dts_conv = DTS_CONV_NONE);

  HeaderInfo header_info() const { return hinfo; }

  /////////////////////////////////////////////////////////
  // SimpleFilter overrides

  bool can_open(Speakers spk) const;
  bool init();

  void reset();
  bool process(Chunk &in, Chunk &out);

  bool new_stream() const
  { return new_stream_flag; }

  Speakers get_output() const
  { return out_spk; }

  string info() const;

protected:
  Rawdata     buf;          // output frame buffer
  Speakers    out_spk;      // output format
  bool        passthrough;  // passthrough mode
  bool        new_stream_flag;

  HeaderInfo  hinfo;        // input raw frame info
  Rawdata     header;

  bool use_header;          // use SPDIF header
  int spdif_bs;             // SPDIF bitstream type

  static const size_t header_size;
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

    inline void set(uint16_t _type, uint16_t _len_bits)
    {
      zero1 = 0;
      zero2 = 0;
      zero3 = 0;
      zero4 = 0;

      sync1 = 0xf872;
      sync2 = 0x4e1f;
      type  = _type;
      len   = _len_bits;
    }
  };

};

#endif
