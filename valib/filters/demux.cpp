#include "demux.h"

// todo: detect encoded stream in PCM input
// todo: demux spdif
// todo: syncronization

Demux::Demux()
{}

Speakers 
Demux::lpcm_spk()
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

  // parse LPCM header
  int format, mask, sample_rate;
  sample_t level;

  switch (pes.subheader[4] >> 6)
  {
    case 0: format = FORMAT_PCM16_LE; level = 32767;    break;
    case 2: format = FORMAT_PCM24_LE; level = 8388607;  break;
    default: return Speakers();
  }

  mask = nch2mask[pes.subheader[4] & 7];
  if (!mask) return Speakers();

  switch ((pes.subheader[4] >> 4) & 3)
  {
    case 0: sample_rate = 48000; break;
    case 1: sample_rate = 96000; break;
    default: return Speakers();
  }

  return Speakers(format, mask, sample_rate, level);
}


void 
Demux::reset()
{
  NullFilter::reset();

  pes_size  = 0;
  stream    = 0;
  substream = 0;
  pes.reset();
}

bool 
Demux::query_input(Speakers _spk) const
{
  return _spk.format == FORMAT_PES;
}

bool 
Demux::process(const Chunk *_chunk)
{
  if (!NullFilter::receive_chunk(_chunk))
    return false;

  uint8_t *read_buf  = buf;
  uint8_t *write_buf = buf;
  int read_len = size;
  int write_len = 0;

  while (read_len)
  {
    if (pes_size)
    {
      int l = MIN(read_len, pes_size);

      if (((pes.stream    & 0xe0) != 0xc0) &&   // MPEG audio stream
          ((pes.substream & 0xf8) != 0x80) &&   // AC3 audio substream
          ((pes.substream & 0xf8) != 0x88) &&   // DTS audio substream
          ((pes.substream & 0xf8) != 0xA0))     // LPCM audio substream
      {
        // drop all non-audio packets
        pes_size -= l;
        read_buf += l;
        read_len -= l;
        continue;
      }

      // on stream switch return currently decoded buffer and switch to other stream on next call
      if ((stream && stream != pes.stream) ||
          (stream && substream && substream != pes.substream))
        if (write_len)
          break;

      // update stream info
      stream = pes.stream;
      substream = pes.substream;
      stream_spk = spk;

      if      ((pes.stream    & 0xe0) == 0xc0) stream_spk.format = FORMAT_MPA;   // MPEG audio stream
      else if ((pes.substream & 0xf8) == 0x80) stream_spk.format = FORMAT_AC3;   // AC3 audio substream
      else if ((pes.substream & 0xf8) == 0x88) stream_spk.format = FORMAT_DTS;   // DTS audio substream
      else if ((pes.substream & 0xf8) == 0xA0) stream_spk = lpcm_spk();          // LPCM audio substream

      // demux
      memmove(write_buf, read_buf, l);
      read_buf  += l;
      read_len  -= l;
      write_buf += l;
      write_len += l;
      pes_size  -= l;
    }
    else
    {
      // load PES packet
      int gone;
      pes_size = pes.packet(read_buf, read_len, &gone);
      read_buf += gone;
      read_len -= gone;
    }
  }

  spk  = stream_spk; // get_output() is always right
  buf  = read_buf;
  size = write_len;
  return true;
}
