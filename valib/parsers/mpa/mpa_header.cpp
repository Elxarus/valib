#include "mpa_header.h"

/*
  Minimum and maximum sized for MPEG frames:

  Layer      min br max br min sr max sr slots  min fsize max fsize
  -----------------------------------------------------------------
  MPEG1
  Layer1      32000 448000  32000  48000     12        32       672
  Layer2      32000 384000  32000  48000    144        96      1728
  Layer3      32000 320000  32000  48000    144        96      1440

  MPEG2/LSF
  Layer1      32000 256000  16000  24000     12        64       768
  Layer2       8000 160000  16000  24000    144        48      1440
  Layer3       8000 160000  16000  24000    144        48      1440
*/


union RAWHeader
{
  RAWHeader()
  {}
  RAWHeader(uint32_t h)
  { raw = h; }

  uint32_t raw;
  struct
  {
    unsigned emphasis           : 2;
    unsigned original           : 1;
    unsigned copyright          : 1;
    unsigned mode_ext           : 2;
    unsigned mode               : 2;

    unsigned extension          : 1;
    unsigned padding            : 1;
    unsigned sampling_frequency : 2;
    unsigned bitrate_index      : 4;

    unsigned error_protection   : 1;
    unsigned layer              : 2;
    unsigned version            : 2;
    unsigned sync               : 11;
  };
};

static const int bitrate_tbl[4][3][15] =
{
  { // MPEG1
    { 0, 32, 64, 96, 128, 160, 192, 224, 256, 288, 320, 352, 384, 416, 448 },
    { 0, 32, 48, 56,  64,  80,  96, 112, 128, 160, 192, 224, 256, 320, 384 },
    { 0, 32, 40, 48,  56,  64,  80,  96, 112, 128, 160, 192, 224, 256, 320 }
  }, 
  { // MPEG2 LSF
    { 0, 32, 48, 56,  64,  80,  96, 112, 128, 144, 160, 176, 192, 224, 256 },
    { 0,  8, 16, 24,  32,  40,  48,  56,  64,  80,  96, 112, 128, 144, 160 },
    { 0,  8, 16, 24,  32,  40,  48,  56,  64,  80,  96, 112, 128, 144, 160 }
  },
  { // Reserved
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  },
  { // MPEG2.5 LSF
    { 0, 32, 48, 56,  64,  80,  96, 112, 128, 144, 160, 176, 192, 224, 256 },
    { 0,  8, 16, 24,  32,  40,  48,  56,  64,  80,  96, 112, 128, 144, 160 },
    { 0,  8, 16, 24,  32,  40,  48,  56,  64,  80,  96, 112, 128, 144, 160 }
  },
};

static const int freq_tbl[4][3] =
{
  { 44100, 48000, 32000 }, // MPEG1
  { 22050, 24000, 16000 }, // MPEG2 LSF
  {     0,     0,     0 }, // Reserved
  { 11025, 12000,  8000 }, // MPEG2.5 LSF
};

static const int slots_tbl[4][3] =
{
  { 12, 144, 144 },  // MPEG1
  { 12, 144,  72 },  // MPEG2 LSF
  {  0,   0,   0 },  // Reserved
  { 12, 144,  72 },  // MPEG2.5 LSF
};

// see codegen/makesync.cpp
const SyncTrie MPAFrameParser::sync_trie(
"iiii**ixiiiiiiii***xROxRO*xROxRO**xROxRO*xROxROxxiiiiiiii***xROxRO*xROxRO**"
"xROxRO*xROxRO*ixiiiiiiii***xROxRO*xROxRO**xROxRO*xROxRO*xiiiiiiii***xROxRO*"
"xROxRO**xROxRO*xROxRO*iiiiiiii***xROxRO*xROxRO**xROxRO*xROxROiii*o*ix***xRO"
"xRO*xROxRO**xROxRO*xROxROxx***xROxRO*xROxRO**xROxRO*xROxRO**ix***xROxRO*xRO"
"xRO**xROxRO*xROxROxx***xROxRO*xROxRO**xROxRO*xROxRO*ix***xROxRO*xROxRO**xRO"
"xRO*xROxRO*x***xROxRO*xROxRO**xROxRO*xROxRO****xROxRO*xROxRO**xROxRO*xROxRO"
"***xROxRO*xROxRO**xROxRO*xROxRO");

bool
MPAFrameParser::parse_header(const uint8_t *hdr, FrameInfo *finfo) const
{
  RAWHeader h;
  int bs_type;

  // MPA big and low endians have ambigous headers
  // so first we check big endian as most used and only
  // then try low endian

  // 8 bit or 16 bit big endian stream sync
  // MPEG2.5 supported
  if ((hdr[0] == 0xff)         && // sync
     ((hdr[1] & 0xe0) == 0xe0) && // sync
     ((hdr[1] & 0x18) != 0x08) && // version
     ((hdr[1] & 0x06) != 0x00) && // layer
     ((hdr[2] & 0xf0) != 0xf0) && // bitrate
     ((hdr[2] & 0x0c) != 0x0c))   // sample rate
  {
    uint32_t header = *(uint32_t *)hdr;
    h = swab_u32(header);
    bs_type = BITSTREAM_8;
  }
  else
  // 16 bit low endian stream sync
  // MPEG2.5 is not supported
  if ((hdr[1] == 0xff)         && // sync
     ((hdr[0] & 0xf0) == 0xf0) && // sync
     ((hdr[0] & 0x06) != 0x00) && // layer
     ((hdr[3] & 0xf0) != 0xf0) && // bitrate
     ((hdr[3] & 0x0c) != 0x0c))   // sample rate
  {
    uint32_t header = *(uint32_t *)hdr;
    h = (header >> 16) | (header << 16);
    bs_type = BITSTREAM_16LE;
  }
  else
    return false;

  if (!finfo)
    return true;

  // common information
  int ver   = 3 - h.version;
  int layer = 3 - h.layer;
  int bitrate = bitrate_tbl[ver][layer][h.bitrate_index] * 1000;
  int sample_rate = freq_tbl[ver][h.sampling_frequency];

  finfo->spk = Speakers(FORMAT_MPA, (h.mode == 3)? MODE_MONO: MODE_STEREO, sample_rate);

  if (bitrate)
    finfo->frame_size = bitrate * slots_tbl[ver][layer] / sample_rate + h.padding;
  else
    finfo->frame_size = 0;

  if (layer == 0) // MPA_LAYER_I
    finfo->frame_size *= 4;

  finfo->nsamples = layer == 0? 384: 1152;
  finfo->bs_type = bs_type;

  switch (ver)
  {
  case 0:
    // MPEG1
    if (layer == 0)
      finfo->spdif_type = 0x0004; // Pc burst-info (data type = MPEG1 Layer I) 
    else
      finfo->spdif_type = 0x0005; // Pc burst-info (data type = MPEG1/2 Layer II/III) 
    break;

  case 1:
    // MPEG2 LSF
    if (layer == 0)
      finfo->spdif_type = 0x0008; // Pc burst-info (data type = MPEG2 Layer I LSF) 
    else
      finfo->spdif_type = 0x0009; // Pc burst-info (data type = MPEG2 Layer II/III LSF) 

  default:
    // MPEG2.5 is non-standard.
    finfo->spdif_type = 0;
  }

  return true;
}

bool
MPAFrameParser::compare_headers(const uint8_t *hdr1, const uint8_t *hdr2) const
{
  // Compare following fields:
  // * layer
  // * protection bit
  // * sample rate
  // * mode must indicate the same number of channles (mono or stereo)
  static const int nch[4] = { 2, 2, 2, 1 };

  // 8 bit or 16 bit big endian stream sync
  if ((hdr1[0] == 0xff)         && // sync
     ((hdr1[1] & 0xe0) == 0xe0) && // sync
     ((hdr1[1] & 0x18) != 0x08) && // version
     ((hdr1[1] & 0x06) != 0x00) && // layer
     ((hdr1[2] & 0xf0) != 0xf0) && // bitrate
     ((hdr2[2] & 0xf0) != 0xf0) && // bitrate
     ((hdr1[2] & 0x0c) != 0x0c))   // sample rate
  {
    return 
      hdr1[0] == hdr2[0] && 
      hdr1[1] == hdr2[1] &&
      (hdr1[2] & 0x0c) == (hdr2[2] & 0x0c) && // sample rate
      nch[hdr1[3] >> 6] == nch[hdr2[3] >> 6]; // number of channels
  }
  else
  // 16 bit low endian stream sync
  if ((hdr1[1] == 0xff)         && // sync
     ((hdr1[0] & 0xf0) == 0xf0) && // sync
     ((hdr1[0] & 0x06) != 0x00) && // layer
     ((hdr1[3] & 0xf0) != 0xf0) && // bitrate
     ((hdr2[3] & 0xf0) != 0xf0) && // bitrate
     ((hdr1[3] & 0x0c) != 0x0c))   // sample rate
  {
    return
      hdr1[1] == hdr2[1] && 
      hdr1[0] == hdr2[0] &&
      (hdr1[3] & 0x0c) == (hdr2[3] & 0x0c) && // sample rate
      nch[hdr1[2] >> 6] == nch[hdr2[2] >> 6]; // number of channels
  }
  else
    return false;
}

SyncInfo
MPAFrameParser::build_syncinfo(const uint8_t *frame, size_t size, const FrameInfo &finfo) const
{
  uint32_t mpa_sync = (frame[0] << 8) | frame[1];

  SyncInfo result = MPAFrameParser::sync_info();
  if (finfo.frame_size == 0)
  {
    // Constant bitrate!
    // It's very important, otherwise any frame size
    // is allowed and false sync is *highly* posible.
    // Finds 3-point sync even at usual DTS stream.
    result.min_frame_size = size;
    result.max_frame_size = size;
  }

  result.sync_trie = SyncTrie(mpa_sync, 16);
  return result;
}
