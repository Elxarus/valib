#include "../../bitstream.h"
#include "mlp_header.h"

static const uint32_t mlp_sync = 0xf8726fbb;
static const uint32_t truehd_sync = 0xf8726fba;

const SyncTrie MlpFrameParser::sync_trie = SyncTrie(32) + SyncTrie(mlp_sync, 32);
const SyncTrie TruehdFrameParser::sync_trie = SyncTrie(32) + SyncTrie(truehd_sync, 32);

static int mlp_rate_tbl[16] = 
{
  48000, 96000, 192000, 0, 0, 0, 0, 0,
  44100, 88200, 176400, 0, 0, 0, 0, 0,
};

static const int mlp_quant_tbl[16] =
{
  16, 20, 24, 0, 0, 0, 0, 0,
   0,  0,  0, 0, 0, 0, 0, 0,
};

static int mlp_nsamples_tbl[16] =
{
  40, 80, 160, 0, 0, 0, 0, 0,
  40, 80, 160, 0, 0, 0, 0, 0,
};

static int mlp_mask_tbl[32] =
{
  MODE_1_0,

  MODE_2_0,
  MODE_2_1,
  MODE_2_0_2,
  MODE_2_0_LFE,
  MODE_2_1_LFE,
  MODE_2_0_2_LFE,

  MODE_3_0,
  MODE_3_1,
  MODE_3_0_2,
  MODE_3_0_LFE,
  MODE_3_1_LFE,
  MODE_3_0_2_LFE,

  MODE_3_1,
  MODE_3_0_2,
  MODE_3_0_LFE,
  MODE_3_1_LFE,

  MODE_3_0_2_LFE,
  MODE_2_0_2_LFE,

  MODE_3_0_2,
  MODE_3_0_2_LFE,

  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

static int truehd_mask_tbl[13] =
{
  CH_MASK_L | CH_MASK_R,
  CH_MASK_C,
  CH_MASK_LFE,
  CH_MASK_SL | CH_MASK_SR,
  0, // top left & top right
  CH_MASK_CL | CH_MASK_CR,
  CH_MASK_BL | CH_MASK_BR,
  CH_MASK_BC,
  0, // top center
  CH_SL | CH_SR,
  CH_MASK_CL | CH_MASK_CR,
  0, // top front center
  CH_MASK_LFE, // LFE2
};

static int truehd_mask(int channels)
{
  int mask = 0;
  for (int i = 0, j = 1; i < array_size(truehd_mask_tbl); i++, j << 1)
    if (channels & j)
      mask |= truehd_mask_tbl[i];
  return mask;
}

///////////////////////////////////////////////////////////////////////////////

SyncInfo
MlpBaseFrameParser::build_syncinfo(const uint8_t *frame, size_t size, const FrameInfo &finfo) const
{
  uint32_t sync = be2uint32(*(uint32_t *)frame + 4);

  SyncInfo result = sync_info();
  result.sync_trie = SyncTrie(32) + SyncTrie(sync, 32);
  return result;
}

bool
MlpBaseFrameParser::first_frame(const uint8_t *frame, size_t size)
{
  if (!BasicFrameParser::first_frame(frame, size))
    return false;
  return check_frame_sequence(frame, size);
}

bool
MlpBaseFrameParser::next_frame(const uint8_t *frame, size_t size)
{
  if (!BasicFrameParser::next_frame(frame, size))
    return false;
  return check_frame_sequence(frame, size);
}

bool
MlpBaseFrameParser::check_frame_sequence(const uint8_t *frame, size_t size) const
{
  if (size < 4) return false;

  size_t pos = 0;
  int frames = 0;
  while (pos + 2 < size)
  {
    size_t frame_size = (0xfff & be2uint16(*(uint16_t *)(frame + pos))) * 2;
    // Todo: check parity
    pos += frame_size;
    frames++;
  }

  if (pos > size)
    return false;

  return true;
}

///////////////////////////////////////////////////////////////////////////////

bool
MlpFrameParser::parse_header(const uint8_t *hdr, FrameInfo *finfo) const
{
  uint32_t sync = be2uint32(*(uint32_t *)(hdr + 4));
  if (sync != mlp_sync)
    return false;

  ReadBS bs(hdr, 64, 96);
  int group1_bits = bs.get(4);
  int group2_bits = bs.get(4);
  int rate1_code = bs.get(4);
  int rate2_code = bs.get(4);
  bs.get(11);
  int channels = bs.get(5);

  if (mlp_quant_tbl[group1_bits] == 0 ||
      mlp_rate_tbl[rate1_code] == 0 ||
      group2_bits != 0xf && mlp_quant_tbl[group2_bits] == 0 ||
      rate2_code != 0xf && mlp_rate_tbl[rate2_code] == 0 ||
      mlp_mask_tbl[channels] == 0)
    return false;

  Speakers spk = Speakers(FORMAT_MLP, mlp_mask_tbl[channels], mlp_rate_tbl[rate1_code]);

  if (finfo)
  {
    finfo->spk = spk;
    finfo->frame_size = 0; // Unknown frame size
    finfo->nsamples = 0;
    finfo->bs_type = BITSTREAM_8;
    finfo->spdif_type = 0;
  }
  return true;
}

bool
MlpFrameParser::compare_headers(const uint8_t *hdr1, const uint8_t *hdr2) const
{
  if (!MlpFrameParser::parse_header(hdr1))
    return false;

  return
    hdr1[8] == hdr2[8] && // equal bits
    hdr1[9] == hdr2[9] && // equal sample rates
    (hdr1[11] & 0x1f) == (hdr2[11] & 0x1f); // equal channel config
}

///////////////////////////////////////////////////////////////////////////////

bool
TruehdFrameParser::parse_header(const uint8_t *hdr, FrameInfo *finfo) const
{
  uint32_t sync = be2uint32(*(uint32_t *)(hdr + 4));
  if (sync != truehd_sync)
    return false;

  ReadBS bs(hdr, 64, 96);
  int rate_code = bs.get(4);
  bs.get(8);
  int channels1 = bs.get(5);
  bs.get(2);
  int channels2 = bs.get(13);
  if (mlp_rate_tbl[rate_code] == 0)
    return false;

  Speakers spk;
  if (channels2)
    spk = Speakers(FORMAT_TRUEHD, truehd_mask(channels2), mlp_rate_tbl[rate_code]);
  else
    spk = Speakers(FORMAT_TRUEHD, truehd_mask(channels1), mlp_rate_tbl[rate_code]);

  if (finfo)
  {
    finfo->spk = spk;
    finfo->frame_size = 0;
    finfo->nsamples = 0;
    finfo->bs_type = BITSTREAM_8;
    finfo->spdif_type = 0;
  }
  return true;
}

bool
TruehdFrameParser::compare_headers(const uint8_t *hdr1, const uint8_t *hdr2) const
{
  if (!TruehdFrameParser::parse_header(hdr1))
    return false;

  return
    ((hdr1[8] & 0xf0) == (hdr2[8] & 0xf0)) && // equal sample rates
    ((hdr1[9] & 0x0f) == (hdr2[9] & 0x0f)) && // equal channel config 1
    ((hdr1[10] & 0x9f) == (hdr2[10] & 0x9f)) && // equal channel config 1 & channel config 2
    (hdr1[11] == hdr2[11]); // equal channel config 2
}
