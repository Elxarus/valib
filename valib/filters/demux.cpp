#include <string.h>
#include "demux.h"

Demux::Demux()
{
  reset();
}

void 
Demux::reset()
{
  NullFilter::reset();

  pes_size  = 0;
  stream    = 0;
  substream = 0;
  stream_spk = spk_unknown;
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

  uint8_t *read_buf  = rawdata;
  uint8_t *write_buf = rawdata;
  int read_len = size;
  int write_len = 0;

  while (read_len)
  {
    if (pes_size)
    {
      int l = MIN(read_len, pes_size);

      if (!pes.is_audio())
      {
        // drop all non-audio packets
        pes_size -= l;
        read_buf += l;
        read_len -= l;
        continue;
      }

      // if there're something at output buffer and next packet belongs to other stream
      // return currently decoded buffer and switch to other stream on next call
      if ((stream && stream != pes.stream) ||
          (stream && substream && substream != pes.substream))
        if (write_len)
          break;

      // update stream info
      stream = pes.stream;
      substream = pes.substream;
      stream_spk = pes.spk();

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

  size    = write_len;
  return true;
}

Speakers 
Demux::get_output() const
{
  return stream_spk;
}
