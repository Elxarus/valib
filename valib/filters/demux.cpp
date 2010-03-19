#include <string.h>
#include "demux.h"

Demux::Demux()
{
  reset();
}

bool
Demux::can_open(Speakers new_spk) const
{
  return new_spk.format == FORMAT_PES;
}

bool 
Demux::process(Chunk2 &in, Chunk2 &out)
{
  out.set_rawdata(in.rawdata, 0, in.sync, in.time);
  in.sync = false;
  in.time = 0;

  is_new_stream = false;
  // update stream info after stream change
  if ((stream && stream != ps.stream) ||
      (stream && substream && substream != ps.substream))
  {
    if (!out_spk.is_unknown())
      is_new_stream = true;
    stream     = ps.stream;
    substream  = ps.substream;
    out_spk    = spk_unknown;
  }

  while (in.size)
  {
    if (ps.payload_size)
    {
      size_t len = MIN(in.size, ps.payload_size);

      // drop all non-audio packets
      if (!ps.is_audio())
      {
        ps.payload_size -= len;
        in.drop_rawdata(len);
        continue;
      }

      // stream change
      if ((stream && stream != ps.stream) ||
          (stream && substream && substream != ps.substream))
        break;

      stream     = ps.stream;
      substream  = ps.substream;

      // If sample rate is specified at input we will trust
      // it and pass to downstream. 
      // It is requred to determine SPDIF passthrough
      // possibility just after demuxer. In future it will
      // be a special filter to determine parameters of
      // comressed spdifable stream, but now we can only
      // trust upstream.
      out_spk = ps.spk();
      if (out_spk.sample_rate == 0)
        out_spk.sample_rate = spk.sample_rate;

      // demux
      memmove(out.rawdata + out.size, in.rawdata, len);
      ps.payload_size -= len;
      in.drop_rawdata(len);
      out.size += len;
    }
    else
    {
      // load PES packet
      uint8_t *end = in.rawdata + in.size;
      ps.parse(&in.rawdata, end);
      in.size = end - in.rawdata;
    }
  }
  return true;
}

void 
Demux::reset()
{
  stream    = 0;
  substream = 0;
  out_spk   = spk_unknown;
  is_new_stream = false;

  ps.reset();
}
