#include "pes_header.h"

#define PRIVATE_STREAM_1 0xbd
#define PRIVATE_STREAM_2 0xbf
#define MPEG1_MAX_STUFFING 24

///////////////////////////////////////////////////////////////////////////////
// Helper functions

static Speakers 
get_format(int stream, int substream, const uint8_t *subheader)
{
  // convert LPCM number of channels to channel mask
  static const int nch2mask[8] = 
  {
    MODE_MONO, 
    MODE_STEREO,
    MODE_3_1,
    MODE_QUADRO,
    MODE_3_2, 
    MODE_5_1,
    0, 0
  };

  if      ((stream    & 0xe0) == 0xc0) return Speakers(FORMAT_MPA, 0, 0);   // MPEG audio stream
  else if ((substream & 0xf8) == 0x80) return Speakers(FORMAT_AC3, 0, 0);   // AC3 audio substream
  else if ((substream & 0xf8) == 0x88) return Speakers(FORMAT_DTS, 0, 0);   // DTS audio substream
  else if ((substream & 0xf8) == 0xA0)                                 // LPCM audio substream
  {
    // parse LPCM header
    // note that MPEG LPCM uses big-endian format
    int format, mask, sample_rate;

    switch (subheader[4] >> 6)
    {
      case 0: format = FORMAT_PCM16_BE; break;
      case 1: format = FORMAT_LPCM20;   break;
      case 2: format = FORMAT_LPCM24;   break;
      default: return spk_unknown;
    }

    mask = nch2mask[subheader[4] & 7];
    if (!mask)
      return spk_unknown;

    switch ((subheader[4] >> 4) & 3)
    {
      case 0: sample_rate = 48000; break;
      case 1: sample_rate = 96000; break;
      default: return spk_unknown;
    }

    return Speakers(format, mask, sample_rate);
  }
  else
    // not an audio format
    return spk_unknown;
}

///////////////////////////////////////////////////////////////////////////////
// PESHeader

bool
PESHeader::is_audio_stream(int stream)
{
  if ((stream & 0xe0) == 0xc0) return true; // MPEG audio stream
  else if (stream == PRIVATE_STREAM_1) return true; // Private stream may contain audio
  return false;
}

bool
PESHeader::parse(const uint8_t *hdr, size_t size)
{
  uint32_t sync = be2uint32(*(uint32_t *)hdr);
  if (sync > 0x000001ff || sync < 0x000001b9)
    return false;

  stream = hdr[3];
  packet_size = ((hdr[4] << 8) | hdr[5]) + 6;

  size_t pos = 6;
  size_t substream_header_pos = 0;
  if (stream != PRIVATE_STREAM_2)
  {
    if ((hdr[pos] & 0xc0) == 0x80)
    {
      // MPEG2
      pos = hdr[8] + 9;
    } 
    else 
    {
      // MPEG1
      while (hdr[pos] == 0xff && pos < MPEG1_MAX_STUFFING && pos < size)
        pos++;

      if (pos >= MPEG1_MAX_STUFFING)
        return false;

      if (pos >= size) return false;
      if ((hdr[pos] & 0xc0) == 0x40)
        pos += 2;

      if (pos >= size) return false;
      if ((hdr[pos] & 0xf0) == 0x20)
        pos += 5;
      else if ((hdr[pos] & 0xf0) == 0x30)
        pos += 10;
      else if (hdr[pos] == 0x0f)
        pos++;
      else
        return false;
    }
  }

  if (stream == PRIVATE_STREAM_1)
  {
    if (pos >= size) return false;
    substream = hdr[pos++];
    substream_header_pos = pos;
    pos += 3; // skip substream header
  }
  else
  {
    substream = 0;
    substream_header_pos = 0;
  }

  if (pos + 3 >= size) return false;
  spk = get_format(stream, substream, hdr + substream_header_pos);
  if (spk.is_unknown())
    return false;

  if (spk.format == FORMAT_PCM16_BE || spk.format == FORMAT_LPCM20 || spk.format == FORMAT_LPCM24)
    // Skip LPCM header
    pos += 3;

  if (pos > size) return false;
  if (pos >= packet_size) return false;
  payload_pos = pos;
  payload_size = packet_size - pos;
  return true;
}

bool
PESHeader::operator == (const PESHeader &other) const
{
  return
    stream == other.stream &&
    substream == other.substream &&
    spk == other.spk;
}

bool
PESHeader::operator != (const PESHeader &other) const
{
  return !(*this == other);
}
