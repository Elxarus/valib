#include <stdio.h>
#include "decoder.h"

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
  if (parser)
    parser->reset();
}

bool
AudioDecoder::query_input(Speakers _spk) const
{
  int format_mask = FORMAT_MPA | FORMAT_AC3 | FORMAT_DTS;
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


Speakers
AudioDecoder::get_output() const
{
  if (parser)
    return parser->get_spk();
  else
    return unk_spk;
}

bool 
AudioDecoder::get_chunk(Chunk *_chunk)
{
  if (!parser) 
    return false;

  sync_helper.receive_sync(sync, time);
  sync = false;

  uint8_t *buf_ptr = buf;
  uint8_t *end_ptr = buf + size;

  while (buf_ptr < end_ptr)
    if (parser->load_frame(&buf_ptr, end_ptr))
    {
      drop(buf_ptr - buf);

      // decode
      if (!parser->decode_frame())
        return false;

      // fill output chunk
      _chunk->set_spk(parser->get_spk());
      _chunk->set_samples(parser->get_samples(), parser->get_nsamples());
      // timimg
      sync_helper.send_sync(_chunk);
      sync_helper.set_syncing(true);
      // end-of-stream
      _chunk->set_eos(flushing && !size);
      flushing = flushing && size;

      // quick hack to overcome bug in splitters that report incorrect sample rate
      // _chunk->time *= double(_out->spk.sample_rate) / spk.sample_rate;


      return true;
    } // if (parser->load_frame(&buf, end))

  sync_helper.set_syncing(false);
  drop(buf_ptr - buf);
  return true;
}
