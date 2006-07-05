#include <stdio.h>
#include "decoder.h"

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
