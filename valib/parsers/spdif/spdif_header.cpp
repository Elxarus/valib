#include "spdif_header.h"
#include "../mpa/mpa_header.h"
#include "../ac3/ac3_header.h"
#include "../dts/dts_header.h"

const SPDIFHeader spdif_header;

static const SyncTrie SPDIFTrie =
  (SyncTrie(0, 32) + SyncTrie(0, 32) + SyncTrie(0x72f81f4e, 32)) |
  SyncTrie(0xfe7f0180, 32) | SyncTrie(0xff1f00e8, 32);

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

SyncTrie
SPDIFHeader::sync_trie() const
{ return SPDIFTrie; }

bool
SPDIFHeader::parse_header(const uint8_t *hdr, HeaderInfo *hinfo) const
{
  // DTS
  const HeaderParser *parser = 0;
  const uint8_t *payload = hdr;

  if ((hdr[0] == 0x00) && (hdr[1] == 0x00) && (hdr[2]  == 0x00) && (hdr[3]  == 0x00) &&
      (hdr[4] == 0x00) && (hdr[5] == 0x00) && (hdr[6]  == 0x00) && (hdr[7]  == 0x00) &&
      (hdr[8] == 0x72) && (hdr[9] == 0xf8) && (hdr[10] == 0x1f) && (hdr[11] == 0x4e))
  {
    // Parse SPDIF header
    const spdif_header_s *spdif_header = (spdif_header_s *)hdr;
    payload = hdr + sizeof(spdif_header_s);
    parser = find_parser(spdif_header->type);
    if (!parser)
      return false;
  }
  // DTS 16 bits low endian bitstream
  else if (hdr[0] == 0xfe && hdr[1] == 0x7f &&
           hdr[2] == 0x01 && hdr[3] == 0x80)
    parser = &dts_header;
  // DTS 14 bits low endian bitstream
  else if (hdr[0] == 0xff && hdr[1] == 0x1f &&
           hdr[2] == 0x00 && hdr[3] == 0xe8 &&
          (hdr[4] & 0xf0) == 0xf0 && hdr[5] == 0x07)
    parser = &dts_header;
  else
    return false;

  HeaderInfo subinfo;
  if (!parser->parse_header(payload, &subinfo))
    return false;

  // SPDIF frame size equals to number of samples in contained frame
  // multiplied by 4 (see Spdifer class for more details)

  if (hinfo)
  {
    hinfo->bs_type = BITSTREAM_16LE;
    hinfo->spk = subinfo.spk;
    hinfo->spk.format = FORMAT_SPDIF;
    hinfo->frame_size = subinfo.nsamples * 4;
    hinfo->scan_size = subinfo.nsamples * 4;
    hinfo->nsamples = subinfo.nsamples;
    hinfo->spdif_type = 0;
  }
  return true;
}

bool
SPDIFHeader::compare_headers(const uint8_t *hdr1, const uint8_t *hdr2) const
{
  if ((hdr1[0] == 0x00) && (hdr1[1] == 0x00) && (hdr1[2]  == 0x00) && (hdr1[3]  == 0x00) &&
      (hdr1[4] == 0x00) && (hdr1[5] == 0x00) && (hdr1[6]  == 0x00) && (hdr1[7]  == 0x00) &&
      (hdr1[8] == 0x72) && (hdr1[9] == 0xf8) && (hdr1[10] == 0x1f) && (hdr1[11] == 0x4e) &&
      (hdr2[0] == 0x00) && (hdr2[1] == 0x00) && (hdr2[2]  == 0x00) && (hdr2[3]  == 0x00) &&
      (hdr2[4] == 0x00) && (hdr2[5] == 0x00) && (hdr2[6]  == 0x00) && (hdr2[7]  == 0x00) &&
      (hdr2[8] == 0x72) && (hdr2[9] == 0xf8) && (hdr2[10] == 0x1f) && (hdr2[11] == 0x4e))
  {
    const spdif_header_s *header = (spdif_header_s *)hdr1;
    const HeaderParser *parser = find_parser(header->type);
    return parser && parser->compare_headers(
      hdr1 + sizeof(spdif_header_s),
      hdr2 + sizeof(spdif_header_s));
  }
  // DTS 16 bits low endian bitstream
  else if (hdr1[0] == 0xfe && hdr1[1] == 0x7f &&
           hdr1[2] == 0x01 && hdr1[3] == 0x80 &&
           hdr2[0] == 0xfe && hdr2[1] == 0x7f &&
           hdr2[2] == 0x01 && hdr2[3] == 0x80)
    return dts_header.compare_headers(hdr1, hdr2);
  // DTS 14 bits low endian bitstream
  else if (hdr1[0] == 0xff && hdr1[1] == 0x1f &&
           hdr1[2] == 0x00 && hdr1[3] == 0xe8 &&
          (hdr1[4] & 0xf0) == 0xf0 && hdr1[5] == 0x07 &&
           hdr2[0] == 0xff && hdr2[1] == 0x1f &&
           hdr2[2] == 0x00 && hdr2[3] == 0xe8 &&
          (hdr2[4] & 0xf0) == 0xf0 && hdr2[5] == 0x07)
    return dts_header.compare_headers(hdr1, hdr2);
  else
    return false;
}

string
SPDIFHeader::header_info(const uint8_t *hdr) const
{
  if ((hdr[0] == 0x00) && (hdr[1] == 0x00) && (hdr[2]  == 0x00) && (hdr[3]  == 0x00) &&
      (hdr[4] == 0x00) && (hdr[5] == 0x00) && (hdr[6]  == 0x00) && (hdr[7]  == 0x00) &&
      (hdr[8] == 0x72) && (hdr[9] == 0xf8) && (hdr[10] == 0x1f) && (hdr[11] == 0x4e))
  {
    spdif_header_s *header = (spdif_header_s *)hdr;
    const uint8_t *subheader = hdr + sizeof(spdif_header_s);
    HeaderInfo subinfo;

    const HeaderParser *parser = find_parser(header->type);
    if (!parser)
      return "Unknown stream type\n";

    if (!parser->parse_header(subheader, &subinfo))
      return "Cannot parse substream header\n";

    string result;
    result += string("Stream format: SPDIF/") + subinfo.spk.print() + string("\n");
    result += parser->header_info(subheader);
    return result;
  }
  else if ((hdr[0] == 0xfe && hdr[1] == 0x7f &&
            hdr[2] == 0x01 && hdr[3] == 0x80) ||
           (hdr[0] == 0xff && hdr[1] == 0x1f &&
            hdr[2] == 0x00 && hdr[3] == 0xe8 &&
           (hdr[4] & 0xf0) == 0xf0 && hdr[5] == 0x07))
  {
    HeaderInfo hinfo;
    if (!dts_header.parse_header(hdr, &hinfo))
      return "Cannot parse substream header\n";

    string result;
    result += string("Stream format: Padded DTS");
    result += dts_header.header_info(hdr);
    return result;
  }
  else
    return "No SPDIF header found";
}
