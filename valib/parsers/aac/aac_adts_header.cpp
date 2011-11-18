#include "aac_adts_header.h"

// see codegen/makesync.cpp
const SyncTrie ADTSFrameParser::sync_trie
("iiiiiiiiiiiixooxxx***xxLLIxxLLI*xxLLIxxLLIo*xxLLIxxLLI");

static const int sample_rates[] =
{
  96000, 88200, 64000, 48000, 44100, 32000,
  24000, 22050, 16000, 12000, 11025, 8000, 7350
};

static const int modes[] =
{
  0,
  MODE_MONO,
  MODE_STEREO,
  MODE_3_0,
  MODE_3_1,
  MODE_3_2,
  MODE_3_2_LFE,
  MODE_5_2_LFE
};

bool
ADTSFrameParser::parse_header(const uint8_t *hdr, FrameInfo *finfo) const
{
  int bs_type;
  int profile;
  int sampling_frequency_index;
  int channel_configuration;
  int frame_length;

  if ((hdr[0] == 0xff)         && // sync
     ((hdr[1] & 0xf0) == 0xf0) && // sync
     ((hdr[1] & 0x06) == 0x00) && // layer
     ((hdr[2] & 0x3c) <  0x30) && // sample rate index
     ((((hdr[2] & 1) << 2) | (hdr[3] >> 6)) != 0)) // channel config
  {
    bs_type = BITSTREAM_8;
    profile = hdr[2] >> 6;
    sampling_frequency_index = (hdr[2] >> 2) & 0xf;
    channel_configuration = ((hdr[2] & 1) << 2) | (hdr[3] >> 6);
    frame_length = ((hdr[3] & 3) << 11) | (hdr[4] << 3) | (hdr[5] >> 5);

    if (!finfo) return true;
    finfo->spk = Speakers(FORMAT_AAC_ADTS, modes[channel_configuration], sample_rates[sampling_frequency_index]);
    finfo->frame_size = frame_length;
    finfo->nsamples = 1024;
    finfo->bs_type = bs_type;
    finfo->spdif_type = 0;
    return true;
  }

  return false;
}

bool
ADTSFrameParser::compare_headers(const uint8_t *hdr1, const uint8_t *hdr2) const
{
  // Compare first 28bits (adts_fixed_header())
  return ADTSFrameParser::parse_header(hdr1) &&
         (hdr1[0] == hdr2[0]) && (hdr1[1] == hdr2[1]) &&
         (hdr1[2] == hdr2[2]) && ((hdr1[3] & 0xfc) == (hdr2[3] & 0xfc));
}

SyncInfo
ADTSFrameParser::build_syncinfo(const uint8_t *frame, size_t size, const FrameInfo &finfo) const
{
  uint32_t adts_fixed_header = (frame[0] << 20) | (frame[1] << 12) | (frame[2] << 4) | (frame[3] >> 4);

  SyncInfo result = sync_info();
  result.sync_trie = SyncTrie(adts_fixed_header, 28);
  return result;
}


///////////////////////////////////////////////////////////////////////////////

const ADTSHeader adts_header;

static const SyncTrie ADTSTrie
("iiiiiiiiiiiixooxxx***xxLLIxxLLI*xxLLIxxLLIo*xxLLIxxLLI");

SyncTrie
ADTSHeader::sync_trie() const
{ return ADTSTrie; }

bool
ADTSHeader::parse_header(const uint8_t *hdr, HeaderInfo *hinfo) const
{
  int bs_type;
  int profile;
  int sampling_frequency_index;
  int channel_configuration;
  int frame_length;

  if ((hdr[0] == 0xff)         && // sync
     ((hdr[1] & 0xf0) == 0xf0) && // sync
     ((hdr[1] & 0x06) == 0x00) && // layer
     ((hdr[2] & 0x3c) <  0x30) && // sample rate index
     ((((hdr[2] & 1) << 2) | (hdr[3] >> 6)) != 0)) // channel config
  {
    bs_type = BITSTREAM_8;
    profile = hdr[2] >> 6;
    sampling_frequency_index = (hdr[2] >> 2) & 0xf;
    channel_configuration = ((hdr[2] & 1) << 2) | (hdr[3] >> 6);
    frame_length = ((hdr[3] & 3) << 11) | (hdr[4] << 3) | (hdr[5] >> 5);
  }
  else
    return false;

  if (!hinfo)
    return true;

  hinfo->spk = Speakers(FORMAT_AAC_ADTS, modes[channel_configuration], sample_rates[sampling_frequency_index]);
  hinfo->frame_size = frame_length;
  hinfo->scan_size = 0;
  hinfo->nsamples = 1024;
  hinfo->bs_type = bs_type;
  hinfo->spdif_type = 0;
  return true;
}

bool
ADTSHeader::compare_headers(const uint8_t *hdr1, const uint8_t *hdr2) const
{
  // Comapre first 28bits (adts_fixed_header())
  return (hdr1[0] == hdr2[0]) && (hdr1[1] == hdr2[1]) &&
         (hdr1[2] == hdr2[2]) && ((hdr1[3] & 0xfc) == (hdr2[3] & 0xfc));
}
