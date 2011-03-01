#include <sstream>
#include "spdif_header.h"
#include "spdif_wrapper.h"

#include "../../bitstream.h"
#include "../mpa/mpa_header.h"
#include "../ac3/ac3_header.h"
#include "../dts/dts_header.h"

#define MAX_SPDIF_FRAME_SIZE 8192


inline bool is_14bit(int bs_type)
{
  return (bs_type == BITSTREAM_14LE) || (bs_type == BITSTREAM_14BE);
}


SPDIFWrapper::SPDIFWrapper(int _dts_mode, int _dts_conv)
:dts_mode(_dts_mode), dts_conv(_dts_conv)
{
  const HeaderParser *parsers[] = { &spdif_header, &mpa_header, &ac3_header, &dts_header };
  spdifable.set_parsers(parsers, array_size(parsers));
  buf = new uint8_t[MAX_SPDIF_FRAME_SIZE];
}

SPDIFWrapper::~SPDIFWrapper()
{
  safe_delete(buf);
}

///////////////////////////////////////////////////////////////////////////////
// FrameParser overrides

const HeaderParser *
SPDIFWrapper::header_parser() const
{
  return &spdifable;
}

void 
SPDIFWrapper::reset()
{
  hdr.clear();

  spk = spk_unknown;
  spdif_frame = 0;
  spdif_size = 0;
}

bool
SPDIFWrapper::process(uint8_t *frame, size_t size)
{
  spdif_frame = 0;
  spdif_size = 0;

  /////////////////////////////////////////////////////////
  // Determine frame's characteristics

  if (!spdifable.parse_header(frame, &hdr))
    // unknown format
    return false;

  if (!hdr.spdif_type)
  {
    // passthrough non-spdifable format
    spk = hdr.spk;
    spdif_frame = frame;
    spdif_size = size;
    return true;
  }

  /////////////////////////////////////////////////////////
  // Parse spdif input if nessesary

  uint8_t *raw_frame = frame;
  size_t raw_size = size;

  /////////////////////////////////////////////////////////
  // SPDIF frame size

  size_t spdif_frame_size = hdr.nsamples * 4;
  if (spdif_frame_size > MAX_SPDIF_FRAME_SIZE)
  {
    // impossible to send over spdif
    // passthrough non-spdifable data
    spk = hdr.spk;
    spdif_frame = raw_frame;
    spdif_size = raw_size;
    return true;
  }

  /////////////////////////////////////////////////////////
  // Determine output bitstream type and header usage

  if (hdr.spk.format == FORMAT_DTS)
  {
    // DTS frame may grow if conversion to 14bit stream format is used
    bool frame_grows = (dts_conv == DTS_CONV_14BIT) && !is_14bit(hdr.bs_type);
    // DTS frame may shrink if conversion to 16bit stream format is used
    bool frame_shrinks = (dts_conv == DTS_CONV_16BIT) && is_14bit(hdr.bs_type);

    switch (dts_mode)
    {
    case DTS_MODE_WRAPPED:
      use_header = true;
      if (frame_grows && (raw_size * 8 / 7 <= spdif_frame_size - sizeof(spdif_header_s)))
        spdif_bs = BITSTREAM_14LE;
      else if (frame_shrinks && (raw_size * 7 / 8 <= spdif_frame_size - sizeof(spdif_header_s)))
        spdif_bs = BITSTREAM_16LE;
      else if (raw_size <= spdif_frame_size - sizeof(spdif_header_s))
        spdif_bs = is_14bit(hdr.bs_type)? BITSTREAM_14LE: BITSTREAM_16LE;
      else
      {
        // impossible to wrap
        // passthrough non-spdifable data
        spk = hdr.spk;
        spdif_frame = raw_frame;
        spdif_size = raw_size;
        return true;
      }
      break;

    case DTS_MODE_PADDED:
      use_header = false;
      if (frame_grows && (raw_size * 8 / 7 <= spdif_frame_size))
        spdif_bs = BITSTREAM_14LE;
      else if (frame_shrinks && (raw_size * 7 / 8 <= spdif_frame_size))
        spdif_bs = BITSTREAM_16LE;
      else if (raw_size <= spdif_frame_size)
        spdif_bs = is_14bit(hdr.bs_type)? BITSTREAM_14LE: BITSTREAM_16LE;
      else
      {
        // impossible to send over spdif
        // passthrough non-spdifable data
        spk = hdr.spk;
        spdif_frame = raw_frame;
        spdif_size = raw_size;
        return true;
      }
      break;

    case DTS_MODE_AUTO:
    default:
      if (frame_grows && (raw_size * 8 / 7 <= spdif_frame_size - sizeof(spdif_header_s)))
      {
        use_header = true;
        spdif_bs = BITSTREAM_14LE;
      }
      else if (frame_grows && (raw_size * 8 / 7 <= spdif_frame_size))
      {
        use_header = false;
        spdif_bs = BITSTREAM_14LE;
      }
      if (frame_shrinks && (raw_size * 7 / 8 <= spdif_frame_size - sizeof(spdif_header_s)))
      {
        use_header = true;
        spdif_bs = BITSTREAM_14LE;
      }
      else if (frame_shrinks && (raw_size * 7 / 8 <= spdif_frame_size))
      {
        use_header = false;
        spdif_bs = BITSTREAM_14LE;
      }
      else if (raw_size <= spdif_frame_size - sizeof(spdif_header_s))
      {
        use_header = true;
        spdif_bs = is_14bit(hdr.bs_type)? BITSTREAM_14LE: BITSTREAM_16LE;
      }
      else if (raw_size <= spdif_frame_size)
      {
        use_header = false;
        spdif_bs = is_14bit(hdr.bs_type)? BITSTREAM_14LE: BITSTREAM_16LE;
      }
      else
      {
        // impossible to send over spdif
        // passthrough non-spdifable data
        spk = hdr.spk;
        spdif_frame = raw_frame;
        spdif_size = raw_size;
        return true;
      }
      break;
    }
  }
  else
  {
    if (raw_size <= spdif_frame_size - sizeof(spdif_header_s))
    {
      use_header = true;
      spdif_bs = BITSTREAM_16LE;
    }
    else
    {
      // impossible to wrap
      // passthrough non-spdifable data
      spk = hdr.spk;
      spdif_frame = raw_frame;
      spdif_size = raw_size;
      return true;
    }
  }

  /////////////////////////////////////////////////////////
  // Fill payload, convert bitstream type and init header

  size_t payload_size;
  if (use_header)
  {
    payload_size = bs_convert(raw_frame, raw_size, hdr.bs_type, buf + sizeof(spdif_header_s), spdif_bs);
    assert(payload_size < MAX_SPDIF_FRAME_SIZE - sizeof(spdif_header_s));
    memset(buf + sizeof(spdif_header_s) + payload_size, 0, spdif_frame_size - sizeof(spdif_header_s) - payload_size);

    // We must correct DTS synword when converting to 14bit
    if (spdif_bs == BITSTREAM_14LE)
      buf[sizeof(spdif_header_s) + 3] = 0xe8;

    spdif_header_s *header = (spdif_header_s *)buf;
    header->set(hdr.spdif_type, (uint16_t)payload_size * 8);
  }
  else
  {
    payload_size = bs_convert(raw_frame, raw_size, hdr.bs_type, buf, spdif_bs);
    assert(payload_size < MAX_SPDIF_FRAME_SIZE);
    memset(buf + payload_size, 0, spdif_frame_size - payload_size);

    // We must correct DTS synword when converting to 14bit
    if (spdif_bs == BITSTREAM_14LE)
      buf[3] = 0xe8;
  }

  if (!payload_size)
    // cannot convert bitstream
    return false;

  /////////////////////////////////////////////////////////
  // Send spdif frame

  spk = hdr.spk;
  spk.format = FORMAT_SPDIF;
  spdif_frame = buf;
  spdif_size = spdif_frame_size;

  return true;
}

string
SPDIFWrapper::info() const 
{
  char *bitstream = "???";
  switch (spdif_bs)
  {
    case BITSTREAM_16BE: bitstream = "16bit BE"; break;
    case BITSTREAM_16LE: bitstream = "16bit LE"; break;
    case BITSTREAM_14BE: bitstream = "14bit BE"; break;
    case BITSTREAM_14LE: bitstream = "14bit LE"; break;
  }

  using std::endl;
  std::stringstream result;
  result << "Output format: " << spk.print() << endl;
  result << "SPDIF format: " << (use_header? "wrapped": "padded") << endl;
  result << "Bitstream: " << bitstream << endl;
  result << "Frame size: " << spdif_size << endl;
  return result.str();
}
