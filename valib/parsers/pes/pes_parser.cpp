#include <sstream>
#include "pes_parser.h"
#include "pes_header.h"

///////////////////////////////////////////////////////////////////////////////
// PESParser

PESParser::PESParser()
{
  reset();
}

///////////////////////////////////////////////////////////////////////////////
// SimpleFilter overrides

bool
PESParser::can_open(Speakers spk) const
{
  return spk.format == FORMAT_PES;
}

bool
PESParser::init()
{
  reset();
  return true;
}

void 
PESParser::reset()
{
  out_spk = spk_unknown;
  new_stream_flag = false;
  stream = 0;
  substream = 0;
}

bool
PESParser::process(Chunk &in, Chunk &out)
{
  bool     sync  = in.sync;
  vtime_t  time  = in.time;
  uint8_t *frame = in.rawdata;
  size_t   size  = in.size;
  in.clear();

  if (size < 6)
    return false;

  PESHeader header;
  if (!header.parse(frame, size))
    return false;

  if (size < header.packet_size)
    return false;

  if (!stream ||
      (stream && stream != header.stream) || 
      (stream && substream && substream != header.substream))
  {
    // New stream
    out_spk = header.spk;
    new_stream_flag = true;
    stream = header.stream;
    substream = header.substream;
  }
  else
    new_stream_flag = false;

  out.set_rawdata(frame + header.payload_pos, header.payload_size, sync, time);
  out_spk = header.spk;
  return true;
}

string
PESParser::info() const 
{
  std::stringstream s;
  if (stream)
  {
    s << std::hex;
    s << "Stream: 0x" << stream;
    if (substream)
      s << "\nSubstream: 0x" << substream;
    s << "Format: " << spk.print();
  }
  else
    s << "No sync";
  return s.str();
}
