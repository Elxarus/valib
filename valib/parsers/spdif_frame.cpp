#include "spdif_header.h"
#include "spdif_frame.h"

#include "mpa\mpa_header.h"
#include "ac3\ac3_header.h"
#include "dts\dts_header.h"

#include "bitstream.h"

inline static const HeaderParser *find_parser(int spdif_type)
{
  switch (spdif_type)
  {
    // AC3
    case 1: return &ac3_header;
    // MPA
    case 4:
    case 5:
    case 8:
    case 9: return &mpa_header;
    // DTS
    case 11:
    case 12:
    case 13: return &dts_header;
    // Unknown
    default: return 0;
  }
}

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


SPDIFFrame::SPDIFFrame()
{
  big_endian = true;
  reset();
}

SPDIFFrame::~SPDIFFrame()
{}

///////////////////////////////////////////////////////////////////////////////
// FrameParser overrides

const HeaderParser *
SPDIFFrame::header_parser() const
{
  return &spdif_header;
}

void 
SPDIFFrame::reset()
{
  data = 0;
  data_size = 0;

  hdr.drop();
}

bool
SPDIFFrame::parse_frame(uint8_t *frame, size_t size)
{
  if ((frame[0] != 0x72) || (frame[1] != 0xf8) || (frame[2] != 0x1f) || (frame[3] != 0x4e))
    return false;

  const spdif_header_s *header = (spdif_header_s *)frame;
  uint8_t *subheader = frame + sizeof(spdif_header_s);

  const HeaderParser *parser = find_parser(header->type);
  if (parser)
    if (parser->parse_header(subheader, &hdr))
    {
      data = subheader;
      data_size = header->len / 8;
      if (big_endian)
      {
        hdr.bs_type = to_big_endian(hdr.bs_type);
        bs_conv_swab16(data, data_size, data);
      }
      return true;
    }

  hdr.drop();
  return false;
}

size_t
SPDIFFrame::stream_info(char *buf, size_t size) const 
{
  char info[1024];
  size_t len = 0;

  if (len + 1 > size) len = size - 1;
  memcpy(buf, info, len + 1);
  buf[len] = 0;
  return len;
}

size_t
SPDIFFrame::frame_info(char *buf, size_t size) const 
{
  if (buf && size) buf[0] = 0;
  return 0;
}
