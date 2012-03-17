#include "../../mpeg_demux.h"
#include "pes_frame_parser.h"

const SyncTrie PESFrameParser::sync_trie("oooooooooooooooooooooooiiLiiiLLI");

#define PACK_HEADER_CODE 0xba
#define SYSTEM_HEADER_CODE 0xbb

///////////////////////////////////////////////////////////////////////////////
// PESFrameParser

bool
PESFrameParser::parse_header(const uint8_t *hdr, FrameInfo *finfo) const
{
  uint32_t sync = be2uint32(*(uint32_t *)hdr);
  if (sync > 0x000001ff || sync < 0x000001b9)
    return false;

  uint8_t stream = hdr[3];
  uint16_t packet_size = 0;
  if (stream != PACK_HEADER_CODE)
    packet_size = ((hdr[4] << 8) | hdr[5]) + 6;

  if (finfo)
  {
    finfo->spk = Speakers(FORMAT_PES, 0, 0);
    finfo->frame_size = packet_size;
    finfo->nsamples = 0;
    finfo->bs_type = BITSTREAM_8;
    finfo->spdif_type = 0;
  }

  return true;
}

bool
PESFrameParser::compare_headers(const uint8_t *hdr1, const uint8_t *hdr2) const
{
  uint32_t sync1 = be2uint32(*(uint32_t *)hdr1);
  uint32_t sync2 = be2uint32(*(uint32_t *)hdr2);
  if (sync1 > 0x000001ff || sync1 < 0x000001b9 ||
      sync2 > 0x000001ff || sync2 < 0x000001b9)
    return false;

  uint8_t id1 = hdr1[3];
  uint8_t id2 = hdr2[3];
  if (id1 == PACK_HEADER_CODE || id2 == PACK_HEADER_CODE ||
      id1 == SYSTEM_HEADER_CODE || id2 == SYSTEM_HEADER_CODE)
    return true;

  return hdr1[3] == hdr2[3];
}

SyncInfo
PESFrameParser::build_syncinfo(const uint8_t *frame, size_t size, const FrameInfo &finfo) const
{
  // include stream number into syncword
  uint32_t sync = be2uint32(*(uint32_t *)frame);

  SyncInfo result = sync_info();
  result.sync_trie = SyncTrie(sync, 32);
  return result;
}
