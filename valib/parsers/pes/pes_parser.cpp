#include <sstream>
#include "../../log.h"
#include "pes_parser.h"
#include "pes_header.h"

#define PACK_HEADER_CODE 0xba
#define SYSTEM_HEADER_CODE 0xbb
static const string log_module = "PESParser";

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
  sync = false;
  time = 0;
  out_spk = spk_unknown;
  new_stream_flag = false;
  stream = 0;
  substream = 0;
}

bool
PESParser::process(Chunk &in, Chunk &out)
{
  if (in.sync)
  {
    sync = true;
    time = in.time;
  }
  uint8_t *frame = in.rawdata;
  size_t   size  = in.size;
  in.clear();

  if (size < 6)
    return false;

  PESHeader header;
  if (!header.parse(frame, size))
  {
    valib_log(log_error, log_module, "Cannot parse");
    return false;
  }

  if (size < header.packet_size)
  {
    valib_log(log_error, log_module, "Packet size (%i) > chunk size (%i)", header.packet_size, size);
    return false;
  }

  // Skip pack header and system header (not an error)
  if (header.stream == PACK_HEADER_CODE || header.stream == SYSTEM_HEADER_CODE)
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
  sync = false;
  time = 0;
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
    s << "\nFormat: " << out_spk.print();
  }
  else
    s << "No sync";
  return s.str();
}
