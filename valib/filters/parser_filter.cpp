#include <stdio.h>
#include "parser_filter.h"

ParserFilter::ParserFilter()
{
  parser = 0;
  errors = 0;

  out_spk = spk_unknown;
  is_new_stream = false;
}

ParserFilter::ParserFilter(FrameParser *_parser)
{
  parser = 0;
  errors = 0;

  out_spk = spk_unknown;
  is_new_stream = false;

  set_parser(_parser);
}

ParserFilter::~ParserFilter()
{}

bool
ParserFilter::set_parser(FrameParser *_parser)
{
  reset();
  parser = 0;

  if (!_parser)
    return true;

  const HeaderParser *header_parser = _parser->header_parser();
  if (!stream.set_parser(header_parser))
    return false;

  parser = _parser;
  return true;
}

const FrameParser *
ParserFilter::get_parser() const
{
  return parser;
}

string
ParserFilter::get_info() const
{
  string result;
  result += stream.stream_info();
  if (parser)
    result += parser->stream_info();
  return result;
}



void
ParserFilter::reset()
{
  out_spk = spk_unknown;
  is_new_stream = false;

  if (parser)
    parser->reset();
  stream.reset();
  sync.reset();
}

bool
ParserFilter::can_open(Speakers spk) const
{
  if (!parser) 
    return false;
  else if (spk.format == FORMAT_RAWDATA) 
    return true;
  else
    return parser->header_parser()->can_parse(spk.format);
}

bool
ParserFilter::process(Chunk &in, Chunk &out)
{
  if (in.sync)
  {
    sync.receive_sync(in, stream.get_buffer_size());
    in.sync = false;
    in.time = 0;
  }

  is_new_stream = false;
  while (load_frame(in))
  {
    if (stream.is_new_stream())
      is_new_stream = true;

    if (parser->process(stream.get_frame(), stream.get_frame_size()))
    {
      if (out_spk != parser->get_output())
        is_new_stream = true;
      out_spk = parser->get_output();

      if (out_spk.is_linear())
        out.set_linear(parser->get_samples(), parser->get_nsamples());
      else
        out.set_rawdata(parser->get_rawdata(), parser->get_rawsize());
      sync.send_frame_sync(out);
      return true;
    }
    else
      errors++;
  }

  // not enough data
  return false;
}

bool
ParserFilter::load_frame(Chunk &in)
{
  size_t old_data_size = stream.get_buffer_size() + in.size;

  uint8_t *end = in.rawdata + in.size;
  bool result = stream.load_frame(&in.rawdata, end);
  in.size = end - in.rawdata;

  size_t new_data_size = stream.get_buffer_size() + in.size;
  sync.drop(old_data_size - new_data_size);
  return result;
}
