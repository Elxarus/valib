#include "frame_splitter.h"

FrameSplitter::FrameSplitter()
{}

FrameSplitter::FrameSplitter(FrameParser *new_parser)
{
  set_parser(new_parser);
}

void
FrameSplitter::set_parser(FrameParser *new_parser)
{
  reset();
  stream.set_parser(new_parser);
}

const FrameParser *
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
  sync.receive_sync(in);
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
FrameSplitter::flush(Chunk &out)
{
  while (stream.flush())
    if (stream.has_frame())
    {
      out.set_rawdata(stream.get_frame(), stream.get_frame_size());
      sync.send_frame_sync(out);
      return true;
    }
  return false;
}

bool
FrameSplitter::load_frame(Chunk &in)
{
  uint8_t *buf = in.rawdata;
  uint8_t *end = buf + in.size;
  size_t old_data_size = stream.get_buffer_size();

  bool result = stream.load_frame(&buf, end);
  size_t gone = buf - in.rawdata;
  in.drop_rawdata(gone);

  sync.put(gone);
  sync.drop(old_data_size + gone - stream.get_buffer_size());
  return result;
}
