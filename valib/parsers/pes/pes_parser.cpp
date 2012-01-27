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
}

bool
PESParser::process(Chunk &in, Chunk &out)
{
  bool     sync  = in.sync;
  vtime_t  time  = in.time;
  uint8_t *frame = in.rawdata;
  size_t   size  = in.size;
  in.clear();

  if (size < frame_parser.header_size())
    return false;

  if (frame_parser.in_sync())
  {
    if (frame_parser.next_frame(frame, size))
      new_stream_flag = false;
    else
      reset();
  }

  if (!frame_parser.in_sync())
  {
    if (frame_parser.first_frame(frame, size))
      new_stream_flag = true;
    else
      return false;
  }

  PESHeader header;
  if (!header.parse(frame))
    return false;

  out.set_rawdata(frame + header.payload_pos, header.payload_size, sync, time);
  out_spk = header.spk;

  return true;

}

string
PESParser::info() const 
{
  return string();
}
