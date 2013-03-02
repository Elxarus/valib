#include "eac3_header.h"

// 0x0b77 | 0x770b
const SyncTrie EAC3FrameParser::sync_trie = SyncTrie(0x0b77, 16) | SyncTrie(0x770b, 16);

static const int srate_tbl[] =
{
  48000, 48000, 48000, 48000, 
  44100, 44100, 44100, 44100, 
  32000, 32000, 32000, 32000, 
  24000, 22050, 16000, 0
};

static const int nsamples_tbl[] =
{
  256, 512, 768, 1536,
  256, 512, 768, 1536,
  256, 512, 768, 1536,
  1536, 1536, 1536, 1536
};

static const int mask_tbl[] = 
{
  MODE_2_0,
  MODE_2_0 | CH_MASK_LFE,
  MODE_1_0, 
  MODE_1_0 | CH_MASK_LFE, 
  MODE_2_0,
  MODE_2_0 | CH_MASK_LFE,
  MODE_3_0,
  MODE_3_0 | CH_MASK_LFE,
  MODE_2_1,
  MODE_2_1 | CH_MASK_LFE,
  MODE_3_1,
  MODE_3_1 | CH_MASK_LFE,
  MODE_2_2,
  MODE_2_2 | CH_MASK_LFE,
  MODE_3_2,
  MODE_3_2 | CH_MASK_LFE,
};

bool
EAC3FrameParser::parse_header(const uint8_t *hdr, FrameInfo *finfo) const
{
  int bsid, frame_size, sample_rate, nsamples, mode, bs_type;

  /////////////////////////////////////////////////////////
  // 8 bit or 16 bit big endian stream sync
  if ((hdr[0] == 0x0b) && (hdr[1] == 0x77))
  {
    if ((hdr[4] >> 4) == 0xf)
      return false;

    bsid = hdr[5] >> 3;
    if (bsid <= 10 || bsid > 16)
      return false;

    if (!finfo)
      return true;

    frame_size = (((hdr[2] & 7) << 8) | hdr[3]) * 2 + 2;
    sample_rate = srate_tbl[hdr[4] >> 4];
    nsamples = nsamples_tbl[hdr[4] >> 4];
    mode = mask_tbl[hdr[4] & 0xf];
    bs_type = BITSTREAM_8;
  }
  /////////////////////////////////////////////////////////
  // 16 bit low endian stream sync
  else if ((hdr[1] == 0x0b) && (hdr[0] == 0x77))
  {
    if ((hdr[5] >> 4) == 0xf)
      return false;

    bsid = hdr[4] >> 3;
    if (bsid <= 10 || bsid > 16)
      return false;

    if (!finfo)
      return true;

    frame_size = (((hdr[3] & 7) << 8) | hdr[2]) * 2 + 2;
    sample_rate = srate_tbl[hdr[5] >> 4];
    nsamples = nsamples_tbl[hdr[5] >> 4];
    mode = mask_tbl[hdr[5] & 0xf];
    bs_type = BITSTREAM_16LE;
  }
  else
    return false;

  finfo->spk = Speakers(FORMAT_EAC3, mode, sample_rate);
  finfo->frame_size = frame_size;
  finfo->nsamples = nsamples;
  finfo->bs_type = bs_type;
  finfo->spdif_type = 0; // 21; // SPDIF Pc burst-info (data type = EAC3) 
  return true;
}

bool
EAC3FrameParser::compare_headers(const uint8_t *hdr1, const uint8_t *hdr2) const
{
  /////////////////////////////////////////////////////////
  // 8 bit or 16 bit big endian
  if ((hdr1[0] == 0x0b) && (hdr1[1] == 0x77))
  {
    if ((hdr1[4] >> 4) == 0xf)
      return false;

    int bsid = hdr1[5] >> 3;
    if (bsid <= 10 || bsid > 16)
      return false;

    return hdr1[0] == hdr2[0] && hdr1[1] == hdr2[1] &&
           hdr1[2] == hdr2[2] && hdr1[3] == hdr2[3] &&
           hdr1[4] == hdr2[4] && (hdr1[5] & 0xf8) == (hdr2[5] & 0xf8);
  }
  /////////////////////////////////////////////////////////
  // 16 bit low endian
  else if ((hdr1[1] == 0x0b) && (hdr1[0] == 0x77))
  {
    if ((hdr1[5] >> 4) == 0xf)
      return false;

    int bsid = hdr1[4] >> 3;
    if (bsid <= 10 || bsid > 16)
      return false;

    return hdr1[1] == hdr2[1] && hdr1[0] == hdr2[0] &&
           hdr1[3] == hdr2[3] && hdr1[2] == hdr2[2] &&
           hdr1[5] == hdr2[5] && (hdr1[4] & 0xf8) == (hdr2[4] & 0xf8);
  }
  return false;
}

SyncInfo
EAC3FrameParser::build_syncinfo(const uint8_t *frame, size_t size, const FrameInfo &finfo) const
{
  uint32_t eac3_sync = (frame[0] << 8) | frame[1];

  SyncInfo result = sync_info();
  result.sync_trie = SyncTrie(eac3_sync, 16);
  return result;
}
