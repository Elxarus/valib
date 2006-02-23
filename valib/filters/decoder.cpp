#include <stdio.h>
#include "decoder.h"

// Decoder should be empty after process() if it was no frame loaded.
// (because it should report right output format at get_chunk())

static const int format_mask = FORMAT_MASK_MPA | FORMAT_MASK_AC3 | FORMAT_MASK_DTS;

AudioDecoder::AudioDecoder()
{
  parser = 0;
  reset();
}

void 
AudioDecoder::get_info(char *_buf, int _len)
{
  if (parser)
    parser->get_info(_buf, _len);
  else
    if (_len) *_buf = 0;
}

void
AudioDecoder::reset()
{
  NullFilter::reset();
  sync_helper.reset();
  if (parser)
    parser->reset();
  out_spk = spk_unknown;
}

bool
AudioDecoder::query_input(Speakers _spk) const
{
  return (FORMAT_MASK(_spk.format) & format_mask) != 0;
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
  if (!NullFilter::process(_chunk))
    return false;

  if (!parser)
    return false;

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

  // here we may have a frame loaded

  if (!parser->is_frame_loaded())
  {
    // flushing (see is_empty())
    _chunk->set_empty(out_spk, 0, 0, flushing);
    flushing = false;
  }
  else if (!parser->decode_frame())
  {
    // send dummy chunk
    _chunk->set_empty(out_spk, 0, 0, flushing);
  }
  else
  {
    // fill output chunk
    _chunk->set_linear(out_spk, parser->get_samples(), parser->get_nsamples());
    // timimg
    sync_helper.send_sync(_chunk);
  }

  load_frame();
  return true;
}

bool
AudioDecoder::load_frame()
{
  uint8_t *buf_ptr = rawdata;
  uint8_t *end_ptr = rawdata + size;

  if (parser->load_frame(&buf_ptr, end_ptr))
  {
    out_spk = parser->get_spk();
    out_spk.format = FORMAT_LINEAR;
    sync_helper.set_syncing(true);
    drop_rawdata(buf_ptr - rawdata);
    return true;
  }
  else
  {
    sync_helper.set_syncing(false);
    drop_rawdata(buf_ptr - rawdata);
    return false;
  }
}


/*
bool 
AudioDecoder::get_chunk(Chunk *_chunk)
{
  if (!parser) 
    return false;

  sync_helper.receive_sync(sync, time);
  sync = false;

  uint8_t *buf_ptr = rawdata;
  uint8_t *end_ptr = rawdata + size;

  while (buf_ptr < end_ptr)
    if (parser->load_frame(&buf_ptr, end_ptr))
    {
      drop(buf_ptr - rawdata);

      // decode
      if (!parser->decode_frame())
        return false;

      // fill output chunk
      _chunk->set
      (
        parser->get_spk(),
        parser->get_samples(), parser->get_nsamples(),
        0, 0,
        flushing && !size
      );
      // timimg
      sync_helper.send_sync(_chunk);
      sync_helper.set_syncing(true);
      // end-of-stream
      flushing = flushing && size;

      // quick hack to overcome bug in splitters that report incorrect sample rate
      // _chunk->time *= double(_out->spk.sample_rate) / spk.sample_rate;


      return true;
    } // if (parser->load_frame(&buf, end))

  sync_helper.set_syncing(false);
  drop(buf_ptr - rawdata);
  return true;
}
*/