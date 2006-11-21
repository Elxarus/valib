#include <stdio.h>

#include "spdif_header.h"
#include "mpa\mpa_header.h"
#include "ac3\ac3_header.h"
#include "dts\dts_header.h"

const SPDIFHeader spdif_header;

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

bool
SPDIFHeader::parse_header(const uint8_t *hdr, HeaderInfo *hinfo) const
{
  if ((hdr[0] != 0x72) || (hdr[1] != 0xf8) || (hdr[2] != 0x1f) || (hdr[3] != 0x4e))
    return false;

  const spdif_header_s *header = (spdif_header_s *)hdr;
  const uint8_t *subheader = hdr + sizeof(spdif_header_s);
  HeaderInfo subinfo;

  const HeaderParser *parser = find_parser(header->type);

  if (!parser)
    return false;

  if (!parser->parse_header(subheader, &subinfo))
    return false;

  // SPDIF frame size equals to number of samples in contained frame
  // multiplied by 4 (see Spdifer class for more details)

  // SPDIF syncword contains 4 16bit zeros (8 bytes) before other fields. But
  // it is possible that stream starts without theese zeros (some utilities
  // generate such files. Therefore we can sync on stream without leading
  // zeros, but must specify SPDIF frame size smaller than actual size
  // (8 bytes smaller).

  if (hinfo)
  {
    hinfo->bs_type = BITSTREAM_16LE;
    hinfo->spk = subinfo.spk;
    hinfo->spk.format = FORMAT_SPDIF;
    hinfo->frame_size = subinfo.nsamples * 4 - 8;
    hinfo->scan_size = subinfo.nsamples * 4;
    hinfo->nsamples = subinfo.nsamples;
    hinfo->spdif_type = header->type;
  }
  return true;
}

bool
SPDIFHeader::compare_headers(const uint8_t *hdr1, const uint8_t *hdr2) const
{
  if ((hdr1[0] != 0x72) || (hdr1[1] != 0xf8) || (hdr1[2] != 0x1f) || (hdr1[3] != 0x4e) ||
      (hdr2[0] != 0x72) || (hdr2[1] != 0xf8) || (hdr2[2] != 0x1f) || (hdr2[3] != 0x4e))
    return false;

  const spdif_header_s *header = (spdif_header_s *)hdr1;
  const HeaderParser *parser = find_parser(header->type);
  if (parser)
    return parser->compare_headers(hdr1 + sizeof(spdif_header_s), hdr2 + sizeof(spdif_header_s));
  else
    return false;
}

size_t
SPDIFHeader::header_info(const uint8_t *hdr, char *buf, size_t size) const
{
  char info[1024];
  size_t info_size = 0;

  if ((hdr[0] == 0x72) && (hdr[1] == 0xf8) && (hdr[2] == 0x1f) && (hdr[3] == 0x4e))
  {
    spdif_header_s *header = (spdif_header_s *)hdr;
    const uint8_t *subheader = hdr + sizeof(spdif_header_s);
    HeaderInfo subinfo;

    const HeaderParser *parser = find_parser(header->type);
    if (parser)
    {
      if (parser->parse_header(subheader, &subinfo))
      {
        info_size += sprintf(info + info_size, "Stream format: SPDIF/%s %s %iHz\n", subinfo.spk.format_text(), subinfo.spk.mode_text(), subinfo.spk.sample_rate);
        info_size += parser->header_info(subheader, info + info_size, sizeof(info) - info_size);
      }
      else
        info_size += sprintf(info + info_size, "Cannot parse substream header\n");
    }
    else
      info_size += sprintf(info + info_size, "Unknown substream\n");
  }
  else
    info_size += sprintf(info + info_size, "No SPDIF header found\n");

  if (info_size > size) info_size = size;
  memcpy(buf, info, info_size);
  buf[info_size] = 0;
  return info_size;
};
