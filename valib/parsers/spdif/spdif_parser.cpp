#include "spdif_parser.h"

#include "../../bitstream.h"
#include "../mpa/mpa_header.h"
#include "../ac3/ac3_header.h"
#include "../dts/dts_header.h"


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


SPDIFParser::SPDIFParser(bool big_endian_)
{
  size_t header_size[] = {
    ac3_header.header_size(),
    dts_header.header_size(),
    mpa_header.header_size()
  };

  big_endian = big_endian_;
  header.allocate(*std::max_element(header_size, header_size + array_size(header_size)));
  reset();
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
  hinfo.clear();
  header.zero();
  new_stream_flag = false;
}

bool
SPDIFParser::process(Chunk &in, Chunk &out)
{
  bool sync = in.sync;
  vtime_t time = in.time;
  in.set_sync(false, 0);
  if (in.size == 0)
    return false;

  uint8_t *frame = in.rawdata;
  size_t size = in.size;
  in.clear();

  if ((frame[0] != 0x00) || (frame[1] != 0x00) || (frame[2]  != 0x00) || (frame[3]  != 0x00) ||
      (frame[4] != 0x00) || (frame[5] != 0x00) || (frame[6]  != 0x00) || (frame[7]  != 0x00) ||
      (frame[8] != 0x72) || (frame[9] != 0xf8) || (frame[10] != 0x1f) || (frame[11] != 0x4e))
    return false;

  const spdif_header_s *spdif_header = (spdif_header_s *)frame;
  uint8_t *payload = frame + sizeof(spdif_header_s);
  size_t payload_size = (spdif_header->len + 7) / 8;
  if (size < sizeof(spdif_header_s) + payload_size)
    return false;

  const HeaderParser *parser = find_parser(spdif_header->type);
  if (parser && parser->parse_header(payload, &hinfo))
  {
    if (parser->compare_headers(payload, header))
      new_stream_flag = false;
    else
    {
      memcpy(header.begin(), payload, parser->header_size());
      new_stream_flag = true;
    }
      
    if (big_endian)
    {
      hinfo.bs_type = to_big_endian(hinfo.bs_type);
      bs_conv_swab16(payload, payload_size, payload);
    }
    out.set_rawdata(payload, payload_size, sync, time);
    return true;
  }

  reset();
  return false;
}

string
SPDIFParser::info() const 
{
  return string();
}
