#include <stdio.h>
#include "frame_splitter.h"

FrameSplitter::FrameSplitter()
{}

FrameSplitter::FrameSplitter(const HeaderParser *new_parser)
{
  set_parser(new_parser);
}

bool
FrameSplitter::set_parser(const HeaderParser *new_parser)
{
  reset();
  stream.set_parser(new_parser);
  return true;
}

const HeaderParser *
FrameSplitter::get_parser() const
{
  return stream.get_parser();
}

void
FrameSplitter::reset()
{
  stream.reset();
  sync.reset();
}

bool
FrameSplitter::can_open(Speakers spk) const
{
  if (!get_parser()) 
    return false;
  else if (spk.format == FORMAT_RAWDATA) 
    return true;
  else
    return get_parser()->can_parse(spk.format);
}

bool
FrameSplitter::process(Chunk &in, Chunk &out)
{
  if (in.sync)
  {
    sync.receive_sync(in, stream.get_buffer_size());
    in.sync = false;
    in.time = 0;
  }

  while (load_frame(in))
  {
    out.set_rawdata(stream.get_frame(), stream.get_frame_size());
    sync.send_frame_sync(out);
    return true;
  }

  // not enough data
  return false;
}

bool
FrameSplitter::load_frame(Chunk &in)
{
  size_t old_data_size = stream.get_buffer_size() + in.size;

  uint8_t *end = in.rawdata + in.size;
  bool result = stream.load_frame(&in.rawdata, end);
  in.size = end - in.rawdata;

  size_t new_data_size = stream.get_buffer_size() + in.size;
  sync.drop(old_data_size - new_data_size);
  return result;
}
