#include <stdio.h>
#include "decoder.h"

AudioDecoder::AudioDecoder()
{
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
  chunk.set_empty();
  sync.reset();

  switch (spk.format)
  {
    case FORMAT_MPA: parser = &mpa; break;
    case FORMAT_AC3: parser = &ac3; break;
    case FORMAT_DTS: parser = &dts; break;
    default: parser = 0;
  }

  if (parser)
    parser->reset();
}

bool
AudioDecoder::query_input(Speakers _spk) const
{
  return _spk.format == FORMAT_MPA ||
         _spk.format == FORMAT_AC3 ||
         _spk.format == FORMAT_DTS;
}


Speakers
AudioDecoder::get_output()
{
  if (parser)
    return parser->get_spk();
  else
    return unk_spk;
}

bool
AudioDecoder::is_empty()
{
  return chunk.is_empty();
}

bool 
AudioDecoder::process(const Chunk *_chunk)
{
  if (_chunk->is_empty())
    return true;

  if (!NullFilter::process(_chunk))
    return false;

  if (!chunk.is_empty())
    sync.receive_timestamp(chunk.timestamp, chunk.time);

  if (!parser) 
    return false;
  else
    return true;
}
 
bool 
AudioDecoder::get_chunk(Chunk *_out)
{
  _out->set_empty();
  if (chunk.is_empty())
    return true;

  if (!parser) 
    return false;

  uint8_t *buf = chunk.buf;
  uint8_t *end = chunk.buf + chunk.size;
  while (buf < end)
    if (parser->load_frame(&buf, end))
    {
      // decode
      if (!parser->decode_frame())
      {
        chunk.drop(buf - chunk.buf);
        return true;
      }
      _out->set_spk(parser->get_spk());
      _out->set_samples(parser->get_samples(), parser->get_nsamples());

      // timimg
      sync.syncpoint(true);
      sync.set_time(_out);

      // quick hack to overcome bug in splitters that report incorrect sample rate
      _out->time *= double(_out->spk.sample_rate) / spk.sample_rate;

      chunk.drop(buf - chunk.buf);
      return true;
    } // if (parser->load_frame(&buf, end))

  sync.syncpoint(false);
  chunk.drop(buf - chunk.buf);
  return true;
}
