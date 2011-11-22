#include "spdif_header.h"

const SyncTrie SPDIFFrameParser::sync_trie =
  (SyncTrie(0, 32) + SyncTrie(0, 32) + SyncTrie(0x72f81f4e, 32)) |
  SyncTrie(0xfe7f0180, 32) | SyncTrie(0xff1f00e8, 32);

inline static const int is_spdif_bitstream(int bs_type)
{
  switch (bs_type)
  {
    case BITSTREAM_16LE:
    case BITSTREAM_14LE:
      return true;
  }
  return false;
}

bool
SPDIFFrameParser::parse_header(const uint8_t *hdr, FrameInfo *finfo) const
{
  // DTS
  const FrameParser *parser = 0;
  const uint8_t *payload = hdr;

  if ((hdr[0] == 0x00) && (hdr[1] == 0x00) && (hdr[2]  == 0x00) && (hdr[3]  == 0x00) &&
      (hdr[4] == 0x00) && (hdr[5] == 0x00) && (hdr[6]  == 0x00) && (hdr[7]  == 0x00) &&
      (hdr[8] == 0x72) && (hdr[9] == 0xf8) && (hdr[10] == 0x1f) && (hdr[11] == 0x4e))
  {
    // Parse SPDIF header
    const spdif_header_s *spdif_header = (spdif_header_s *)hdr;
    payload = hdr + sizeof(spdif_header_s);
    parser = spdifable_parser.find_parser(spdif_header->type);
    if (!parser)
      return false;
  }
  // DTS 16 bits low endian bitstream
  else if (hdr[0] == 0xfe && hdr[1] == 0x7f &&
           hdr[2] == 0x01 && hdr[3] == 0x80)
    parser = &spdifable_parser.dts;
  // DTS 14 bits low endian bitstream
  else if (hdr[0] == 0xff && hdr[1] == 0x1f &&
           hdr[2] == 0x00 && hdr[3] == 0xe8 &&
          (hdr[4] & 0xf0) == 0xf0 && hdr[5] == 0x07)
    parser = &spdifable_parser.dts;
  else
    return false;

  FrameInfo subinfo;
  if (!parser->parse_header(payload, &subinfo))
    return false;

  if (!is_spdif_bitstream(subinfo.bs_type))
    return false;

  // SPDIF frame size equals to number of samples in contained frame
  // multiplied by 4 (see Spdifer class for more details)

  if (finfo)
  {
    finfo->spk = subinfo.spk;
    finfo->spk.format = FORMAT_SPDIF;
    finfo->frame_size = subinfo.nsamples * 4;
    finfo->nsamples = subinfo.nsamples;
    finfo->bs_type = BITSTREAM_16LE;
    finfo->spdif_type = 0;
  }
  return true;
}

bool
SPDIFFrameParser::compare_headers(const uint8_t *hdr1, const uint8_t *hdr2) const
{
  if ((hdr1[0] == 0x00) && (hdr1[1] == 0x00) && (hdr1[2]  == 0x00) && (hdr1[3]  == 0x00) &&
      (hdr1[4] == 0x00) && (hdr1[5] == 0x00) && (hdr1[6]  == 0x00) && (hdr1[7]  == 0x00) &&
      (hdr1[8] == 0x72) && (hdr1[9] == 0xf8) && (hdr1[10] == 0x1f) && (hdr1[11] == 0x4e) &&
      (hdr2[0] == 0x00) && (hdr2[1] == 0x00) && (hdr2[2]  == 0x00) && (hdr2[3]  == 0x00) &&
      (hdr2[4] == 0x00) && (hdr2[5] == 0x00) && (hdr2[6]  == 0x00) && (hdr2[7]  == 0x00) &&
      (hdr2[8] == 0x72) && (hdr2[9] == 0xf8) && (hdr2[10] == 0x1f) && (hdr2[11] == 0x4e))
  {
    const FrameParser *parser1 = spdifable_parser.find_parser(((spdif_header_s *)hdr1)->type);
    const FrameParser *parser2 = spdifable_parser.find_parser(((spdif_header_s *)hdr2)->type);
    return parser1 && parser1 == parser2 && parser1->compare_headers(
      hdr1 + sizeof(spdif_header_s),
      hdr2 + sizeof(spdif_header_s));
  }
  // DTS 16 bits low endian bitstream
  else if (hdr1[0] == 0xfe && hdr1[1] == 0x7f &&
           hdr1[2] == 0x01 && hdr1[3] == 0x80 &&
           hdr2[0] == 0xfe && hdr2[1] == 0x7f &&
           hdr2[2] == 0x01 && hdr2[3] == 0x80)
    return spdifable_parser.dts.compare_headers(hdr1, hdr2);
  // DTS 14 bits low endian bitstream
  else if (hdr1[0] == 0xff && hdr1[1] == 0x1f &&
           hdr1[2] == 0x00 && hdr1[3] == 0xe8 &&
          (hdr1[4] & 0xf0) == 0xf0 && hdr1[5] == 0x07 &&
           hdr2[0] == 0xff && hdr2[1] == 0x1f &&
           hdr2[2] == 0x00 && hdr2[3] == 0xe8 &&
          (hdr2[4] & 0xf0) == 0xf0 && hdr2[5] == 0x07)
    return spdifable_parser.dts.compare_headers(hdr1, hdr2);
  else
    return false;
}
