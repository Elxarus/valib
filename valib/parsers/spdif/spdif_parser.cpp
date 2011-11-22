#include "spdif_parser.h"

#include "../../bitstream.h"

inline static const int to_big_endian(int bs_type)
{
  switch (bs_type)
  {
    case BITSTREAM_16LE: return BITSTREAM_16BE;
    case BITSTREAM_32LE: return BITSTREAM_32BE;
    case BITSTREAM_14LE: return BITSTREAM_14BE;
    default: return bs_type;
  }
}

SPDIFParser::SPDIFParser(bool big_endian_)
{
  big_endian = big_endian_;
  reset();
}

FrameParser *
SPDIFParser::find_parser(int spdif_type)
{
  switch (spdif_type)
  {
    // AC3
    case 1: return &ac3_parser;
    // MPA
    case 4:
    case 5:
    case 8:
    case 9: return &mpa_parser;
    // DTS
    case 11:
    case 12:
    case 13: return &dts_parser;
    // Unknown
    default: return 0;
  }
}

///////////////////////////////////////////////////////////////////////////////
// SimpleFilter overrides

bool
SPDIFParser::can_open(Speakers spk) const
{
  return spk.format == FORMAT_SPDIF;
}

bool
SPDIFParser::init()
{
  reset();
  return true;
}

void 
SPDIFParser::reset()
{
  finfo.clear();
  new_stream_flag = false;
}

bool
SPDIFParser::process(Chunk &in, Chunk &out)
{
  bool     sync  = in.sync;
  vtime_t  time  = in.time;
  uint8_t *frame = in.rawdata;
  size_t   size  = in.size;
  in.clear();

  // DTS
  FrameParser *parser = &dts_parser;
  uint8_t *payload = frame;
  size_t payload_size = size;

  if (size < sizeof(spdif_header_s))
    return false;

  if ((frame[0] == 0x00) && (frame[1] == 0x00) && (frame[2]  == 0x00) && (frame[3]  == 0x00) &&
      (frame[4] == 0x00) && (frame[5] == 0x00) && (frame[6]  == 0x00) && (frame[7]  == 0x00) &&
      (frame[8] == 0x72) && (frame[9] == 0xf8) && (frame[10] == 0x1f) && (frame[11] == 0x4e))
  {
    // Parse SPDIF header
    const spdif_header_s *spdif_header = (spdif_header_s *)frame;
    payload = frame + sizeof(spdif_header_s);
    payload_size = (spdif_header->len + 7) / 8;
    parser = find_parser(spdif_header->type);
    if (!parser || size < sizeof(spdif_header_s) + payload_size)
      return false;
  }

  if (payload_size < parser->header_size())
    return false;

  if (parser->in_sync())
  {
    if (parser->next_frame(payload, payload_size))
      new_stream_flag = false;
    else
      parser->reset();
  }

  if (!parser->in_sync())
  {
    if (parser->first_frame(payload, payload_size))
      new_stream_flag = true;
    else
      return false;
  }

  finfo = parser->frame_info();
  if (big_endian)
  {
    bs_convert(payload, payload_size, finfo.bs_type, payload, to_big_endian(finfo.bs_type));
    finfo.bs_type = to_big_endian(finfo.bs_type);
  }

  out.set_rawdata(payload, payload_size, sync, time);
  return true;
}

string
SPDIFParser::info() const 
{
  return string();
}
