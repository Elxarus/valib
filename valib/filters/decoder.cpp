#include <stdio.h>
#include "decoder.h"


Decoder::Decoder()
:NullFilter(FORMAT_MASK_RAWDATA)
{
  parser = 0;

  out_spk = spk_unknown;
  state = state_transition;
  new_stream = false;
}

Decoder::Decoder(FrameParser *_parser)
:NullFilter(FORMAT_MASK_RAWDATA)
{
  parser = 0;

  out_spk = spk_unknown;
  state = state_transition;
  new_stream = false;

  set_parser(_parser);
}

Decoder::~Decoder()
{
}

bool
Decoder::set_parser(FrameParser *_parser)
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
Decoder::get_parser() const
{
  return parser;
}



void
Decoder::reset()
{
  NullFilter::reset();

  out_spk = spk_unknown;
  state = state_transition;
  new_stream = false;

  if (parser)
    parser->reset();
  stream.reset();
  sync_helper.reset();
}

bool
Decoder::is_ofdd() const
{
  return true;
}

bool
Decoder::process(const Chunk *_chunk)
{
  if (!parser)
    return false;

  // we must ignore dummy chunks
  if (_chunk->is_dummy())
    return true;

  // receive the chunk
  FILTER_SAFE(receive_chunk(_chunk));
  sync_helper.receive_sync(sync, time);
  sync = false;


  switch (state)
  {
    case state_transition:
    {
      if (load_decode_frame())
      {
        out_spk = parser->get_spk();
        state = state_frame_decoded;
      }
      else
      {
        // if we did not start a stream we must forget about current stream on 
        // flushing and drop data currently buffered (flushing state is also
        // dropped so we do not pass eos event in this case)
        if (flushing)
          reset();
      }
      return true;
    }

    case state_no_data:
    {
      if (load_decode_frame())
        state = new_stream? state_format_change: state_frame_decoded;
      else 
        state = flushing? state_flushing: state_no_data;
      return true;
    }

    default: 
      return false;
  }
}

Speakers
Decoder::get_output() const
{
  return out_spk;
}

bool
Decoder::is_empty() const
{
  return state == state_transition || state == state_no_data;
}

bool 
Decoder::get_chunk(Chunk *_chunk)
{
  if (!parser) 
    return false;

  switch (state)
  {
    case state_frame_decoded:
      // send the decoded frame
      _chunk->set_linear(out_spk, parser->get_samples(), parser->get_nsamples());
      sync_helper.send_sync(_chunk);

      // load next frame
      if (load_frame())
        state = new_stream? state_format_change: state_frame_loaded;
      else
        state = flushing? state_flushing: state_no_data;
      return true;

    case state_frame_loaded:
      // decode and send the frame
      if (!decode_frame())
        _chunk->set_empty(out_spk);
      else
       _chunk->set_linear(out_spk, parser->get_samples(), parser->get_nsamples());
      sync_helper.send_sync(_chunk);

      // load next frame
      new_stream = false;
      if (load_frame())
        state = new_stream? state_format_change: state_frame_loaded;
      else
        state = flushing? state_flushing: state_no_data;
      return true;

    case state_format_change:
      // send flushing
      _chunk->set_empty(out_spk);
      _chunk->set_eos();

      // try to determine new output format
      if (decode_frame())
      {
        out_spk = parser->get_spk();
        state = state_frame_decoded;
      }
      else
      {
        out_spk = spk_unknown;
        state = state_transition;
        if (flushing)
          reset();
      }

      new_stream = false;
      return true;

    case state_flushing:
      // send flushing and switch to transition state
      _chunk->set_empty(out_spk);
      _chunk->set_eos();

      out_spk = parser->get_spk();
      state = state_frame_decoded;
      if (flushing)
        reset();
      return true;

    default:
      return false;
  }
}

bool
Decoder::load_frame()
{
  uint8_t *end = rawdata + size;
  bool result = stream.load_frame(&rawdata, end);
  size = end - rawdata;
  new_stream |= stream.is_new_stream();
  return result;
}

bool
Decoder::decode_frame()
{
  return parser->parse_frame(stream.get_frame(), stream.get_frame_size());
}

bool
Decoder::load_decode_frame()
{
  uint8_t *end = rawdata + size;
  while (stream.load_frame(&rawdata, end))
  {
    new_stream |= stream.is_new_stream();
    if (parser->parse_frame(stream.get_frame(), stream.get_frame_size()))
    {
      size = end - rawdata;
      return true;
    }
  }

  size = 0;
  return false;
}


AudioDecoder::AudioDecoder()
:NullFilter(FORMAT_MASK_MPA | FORMAT_MASK_AC3 | FORMAT_MASK_DTS)
{
  parser = 0;
  reset();
}

int
AudioDecoder::get_info(char *_buf, size_t _len) const
{
  return parser? parser->get_info(_buf, _len): 0;
}

void
AudioDecoder::reset()
{
  NullFilter::reset();
  sync_helper.reset();
  if (parser)
    parser->reset();

  stream_spk = spk_unknown;
  out_spk = spk_unknown;
}

bool
AudioDecoder::is_ofdd() const
{
  return true;
}


bool
AudioDecoder::set_input(Speakers _spk)
{
  if (!NullFilter::set_input(_spk))
    return false;

  switch (_spk.format)
  {
    case FORMAT_MPA: parser = &mpa; break;
    case FORMAT_AC3: parser = &ac3; break;
    case FORMAT_DTS: parser = &dts; break;
    default:
      return false;
  }
  parser->reset();
  return true;
}

bool 
AudioDecoder::process(const Chunk *_chunk)
{
  if (!parser)
    return false;

  // we must ignore dummy chunks
  if (_chunk->is_dummy())
    return true;

  FILTER_SAFE(receive_chunk(_chunk));

  sync_helper.receive_sync(sync, time);
  sync = false;

  load_frame();

  // if we did not start a stream we must
  // forget about current stream on flushing
  // and drop data currently buffered
  // (flushing state is also dropped so we 
  // do not pass eos event in this case)
  if (flushing && out_spk == spk_unknown)
    reset();

  return true;
}

Speakers
AudioDecoder::get_output() const
{
  return out_spk;
}

bool
AudioDecoder::is_empty() const
{
  if (parser)
    return !parser->is_frame_loaded() && !flushing;
  else
    return !flushing;
}

bool 
AudioDecoder::get_chunk(Chunk *_chunk)
{
  if (!parser) 
    return false;

  if (!parser->is_frame_loaded())
  {
    // flushing (see is_empty())
    _chunk->set_empty(out_spk, 0, 0, flushing);
    flushing = false;
    return true;
  }
  else if (!parser->decode_frame())
  {
    // decoding error
    // send dummy chunk and load next frame
    _chunk->set_dummy();
    load_frame();
    return true;
  }
  else if (parser->get_spk() != stream_spk)
  {
    // output format change flushing
    _chunk->set_empty(out_spk, 0, 0, true);
    stream_spk = parser->get_spk();
    out_spk = stream_spk;
    out_spk.format = FORMAT_LINEAR;
    return true;
  }
  else
  {
    // fill output chunk
    _chunk->set_linear(out_spk, parser->get_samples(), parser->get_nsamples());
    // timimg
    sync_helper.send_sync(_chunk);
    load_frame();
    return true;
  }

  // never be here
  return true;
}

bool
AudioDecoder::load_frame()
{
  uint8_t *buf_ptr = rawdata;
  uint8_t *end_ptr = rawdata + size;

  if (parser->load_frame(&buf_ptr, end_ptr))
  {
    if (stream_spk == spk_unknown)
    {
      stream_spk = parser->get_spk();
      out_spk = stream_spk;
      out_spk.format = FORMAT_LINEAR;
    }
    sync_helper.set_syncing(true);
    drop_rawdata(buf_ptr - rawdata);
    return true;
  }
  else
  {
    // we must not drop sync_helper's syncing state 
    // if it was no data to loaded
    if (size) sync_helper.set_syncing(false);
    drop_rawdata(buf_ptr - rawdata);
    return false;
  }
}
