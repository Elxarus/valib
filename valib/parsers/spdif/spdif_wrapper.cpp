#include <sstream>
#include "spdif_header.h"
#include "spdif_wrapper.h"

#include "../../bitstream.h"
#include "spdifable_header.h"

static const size_t max_spdif_frame_size = 8192;


inline bool is_14bit(int bs_type)
{
  return (bs_type == BITSTREAM_14LE) || (bs_type == BITSTREAM_14BE);
}

// SPDIF header size
const size_t SPDIFWrapper::header_size = sizeof(SPDIFWrapper::spdif_header_s);


SPDIFWrapper::SPDIFWrapper(int _dts_mode, int _dts_conv)
:dts_mode(_dts_mode), dts_conv(_dts_conv)
{
  buf.allocate(max_spdif_frame_size);
}

///////////////////////////////////////////////////////////////////////////////
// SimpleFilter overrides

bool
SPDIFWrapper::can_open(Speakers spk) const
{
  return parser.can_parse(spk.format);
}

bool
SPDIFWrapper::init()
{
  reset();
  return true;
}

void 
SPDIFWrapper::reset()
{
  parser.reset();
  out_spk = spk_unknown;
  passthrough = false;
  new_stream_flag = false;
}

bool
SPDIFWrapper::process(Chunk &in, Chunk &out)
{
  out = in;
  in.clear();
  if (passthrough)
  {
    new_stream_flag = false;
    return !out.is_dummy();
  }

  uint8_t *frame = out.rawdata;
  size_t size = out.size;

  /////////////////////////////////////////////////////////
  // Determine frame's characteristics

  if (size < parser.header_size())
    return false;

  if (parser.in_sync())
  {
    if (parser.next_frame(frame, size))
      new_stream_flag = false;
    else
      parser.reset();
  }

  if (!parser.in_sync())
  {
    if (parser.first_frame(frame, size))
    {
      out_spk = parser.frame_info().spk;
      out_spk.format = FORMAT_SPDIF;
      new_stream_flag = true;
    }
    else
      return false;
  }

  finfo = parser.frame_info();
  if (finfo.spdif_type == 0)
  {
    // passthrough non-spdifable format
    passthrough = true;
    new_stream_flag = true;
    out_spk = finfo.spk;
    return true;
  }

  /////////////////////////////////////////////////////////
  // SPDIF frame size

  size_t spdif_frame_size = finfo.nsamples * 4;
  if (spdif_frame_size > max_spdif_frame_size)
  {
    // impossible to send over spdif
    // passthrough non-spdifable data
    passthrough = true;
    new_stream_flag = true;
    out_spk = finfo.spk;
    return true;
  }

  /////////////////////////////////////////////////////////
  // Determine output bitstream type and header usage

  if (finfo.spk.format == FORMAT_DTS)
  {
    // DTS frame may grow if conversion to 14bit stream format is used
    bool frame_grows = (dts_conv == DTS_CONV_14BIT) && !is_14bit(finfo.bs_type);
    // DTS frame may shrink if conversion to 16bit stream format is used
    bool frame_shrinks = (dts_conv == DTS_CONV_16BIT) && is_14bit(finfo.bs_type);

    switch (dts_mode)
    {
    case DTS_MODE_WRAPPED:
      use_header = true;
      if (frame_grows && (size * 8 / 7 <= spdif_frame_size - header_size))
        spdif_bs = BITSTREAM_14LE;
      else if (frame_shrinks && (size * 7 / 8 <= spdif_frame_size - header_size))
        spdif_bs = BITSTREAM_16LE;
      else if (size <= spdif_frame_size - header_size)
        spdif_bs = is_14bit(finfo.bs_type)? BITSTREAM_14LE: BITSTREAM_16LE;
      else
      {
        // impossible to wrap
        // passthrough non-spdifable data
        passthrough = true;
        new_stream_flag = true;
        out_spk = finfo.spk;
        return true;
      }
      break;

    case DTS_MODE_PADDED:
      use_header = false;
      if (frame_grows && (size * 8 / 7 <= spdif_frame_size))
        spdif_bs = BITSTREAM_14LE;
      else if (frame_shrinks && (size * 7 / 8 <= spdif_frame_size))
        spdif_bs = BITSTREAM_16LE;
      else if (size <= spdif_frame_size)
        spdif_bs = is_14bit(finfo.bs_type)? BITSTREAM_14LE: BITSTREAM_16LE;
      else
      {
        // impossible to send over spdif
        // passthrough non-spdifable data
        passthrough = true;
        new_stream_flag = true;
        out_spk = finfo.spk;
        return true;
      }
      break;

    case DTS_MODE_AUTO:
    default:
      if (frame_grows && (size * 8 / 7 <= spdif_frame_size - header_size))
      {
        use_header = true;
        spdif_bs = BITSTREAM_14LE;
      }
      else if (frame_grows && (size * 8 / 7 <= spdif_frame_size))
      {
        use_header = false;
        spdif_bs = BITSTREAM_14LE;
      }
      else if (frame_shrinks && (size * 7 / 8 <= spdif_frame_size - header_size))
      {
        use_header = true;
        spdif_bs = BITSTREAM_16LE;
      }
      else if (frame_shrinks && (size * 7 / 8 <= spdif_frame_size))
      {
        use_header = false;
        spdif_bs = BITSTREAM_16LE;
      }
      else if (size <= spdif_frame_size - header_size)
      {
        use_header = true;
        spdif_bs = is_14bit(finfo.bs_type)? BITSTREAM_14LE: BITSTREAM_16LE;
      }
      else if (size <= spdif_frame_size)
      {
        use_header = false;
        spdif_bs = is_14bit(finfo.bs_type)? BITSTREAM_14LE: BITSTREAM_16LE;
      }
      else
      {
        // impossible to send over spdif
        // passthrough non-spdifable data
        passthrough = true;
        new_stream_flag = true;
        out_spk = finfo.spk;
        return true;
      }
      break;
    }
  }
  else
  {
    if (size <= spdif_frame_size - header_size)
    {
      use_header = true;
      spdif_bs = BITSTREAM_16LE;
    }
    else
    {
      // impossible to wrap
      // passthrough non-spdifable data
      passthrough = true;
      new_stream_flag = true;
      out_spk = finfo.spk;
      return true;
    }
  }

  /////////////////////////////////////////////////////////
  // Fill payload, convert bitstream type and init header

  size_t payload_size;
  if (use_header)
  {
    payload_size = bs_convert(frame, size, finfo.bs_type, buf + header_size, spdif_bs);
    assert(payload_size < max_spdif_frame_size - header_size);
    memset(buf + header_size + payload_size, 0, spdif_frame_size - header_size - payload_size);

    // We must correct DTS synword when converting to 14bit
    if (spdif_bs == BITSTREAM_14LE)
      buf[header_size + 3] = 0xe8;

    spdif_header_s *header = (spdif_header_s *)buf.begin();
    header->set(finfo.spdif_type, (uint16_t)payload_size * 8);
  }
  else
  {
    payload_size = bs_convert(frame, size, finfo.bs_type, buf, spdif_bs);
    assert(payload_size < max_spdif_frame_size);
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

  out.set_rawdata(buf.begin(), spdif_frame_size, out.sync, out.time);
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

  std::stringstream result;
  result << "Output format: " << out_spk.print() << nl;
  result << "SPDIF format: " << (use_header? "wrapped": "padded") << nl;
  result << "Bitstream: " << bitstream << nl;
//  result << "Frame size: " << spdif_size << nl;
  return result.str();
}
