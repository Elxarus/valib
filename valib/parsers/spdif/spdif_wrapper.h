/**************************************************************************//**
  \file spdif_wrapper.h
  \brief SPDIFWrapper: Converts raw AC3/MPA/DTS stream to SPDIF stream.
******************************************************************************/

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

/**************************************************************************//**
  \class SPDIFWrapper
  \brief Converts raw AC3/MPA/DTS stream to SPDIF stream.

  Accepts only chunks with exactly one frame.

  Does raw stream output if the stream given cannot be converted (high-bitrate
  DTS for example). Can convert DTS stream type to 14/16 bits. Supports wrapped
  SPDIF and padded-DTS output stream types.

  Some data cannot be converted because it does not fit SPDIF bitrate
  requirements. In this case filter switches to passthrough mode and output
  the stream unchanged. We cannot determine the possibility of the conversion
  in advance, and have to receive some data to decide. Passthrough mode allows
  the client to handle the stream that cannot be converted (decode it for
  example).

  In conversion mode output format is FORMAT_SPDIF. In passthrough mode output
  format equals to the format of the raw stream.

  DTS may be handled in two ways:
  \li It can be wrapped into SPDIF packets.
  \li It can be padded with zeros to match SPDIF bitrate. No SPDIF header added.
    This allows to pass a little higher bitrate DTS and such DTS streams are
    widely used.

  \c dts_mode option selects DTS output mode:
  \li \c DTS_MODE_WRAPPED Wrap DTS into SPDIF packets. If DTS frame does not
    fit SPDIF packet, filter switches to passthrough mode.
  \li \c DTS_MODE_PADDED Pad each DTS frame with zeros to match SPDIF bitrate.
    No SPDIF header added. If DTS does not fit SPDIF bitrate requirements,
    filter switches to passthrough mode.
  \li \c DTS_MODE_AUTO Wrap DTS into SPDIF packets when DTS frame fits SPDIF
    packet. Pad DTS frame with zeros if it fits SPDIF bitrate requirements.
    Switch to passthrough mode otherwise.

  DTS frame size requirements
  \li Wrapped mode: dts_frame_size <= spdif_frame_size - spdif_header_size
  \li Padded mode dts_frame_size <= spdif_frame_size
  Where spdif_frame_size = dts_frame_samples * 4

  DTS stream format may be changed to 14bit or 16bit. \c dts_conv option
  selects the conversion mode:
  \li \c DTS_CONV_NONE Do not convert bitstream type.
  \li \c DTS_CONV_16BIT Convert any bitstream type to 16bit.
  \li \c DTS_CONV_14BIT Convert any bitstream type to 14bit. Note, that frame
    size may grow and exceed the frame size requirements. Conversion is not
    done in this case.

******************************************************************************/

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
