#include "dvd_decoder.h"


DVDDecoder::DVDDecoder()
:proc(4096)
{
  in_spk  = Speakers(FORMAT_PCM16, MODE_STEREO, 48000);
  out_spk = Speakers(FORMAT_PCM16, MODE_STEREO, 48000);

  spdif = FORMAT_MASK_AC3;
  spdif_mode = SPDIF_MODE_NONE;

  encoder.set_bitrate(640000);
  proc.set_output(out_spk);

  stream = 0;
  substream = 0;
  reset();
}

bool 
DVDDecoder::query_output(Speakers _spk)
{  
  if (proc.query_spk(_spk))
    return true;

  if (_spk.format != FORMAT_SPDIF)
    return false;

  _spk.format = FORMAT_LINEAR;
  return proc.query_spk(_spk);
}

bool 
DVDDecoder::set_output(Speakers _spk)
{
  _spk.sample_rate = in_spk.sample_rate;
  if (proc.query_spk(_spk))
  {
    out_spk = _spk;
    out_spk.sample_rate = in_spk.sample_rate;
    return proc.set_output(_spk);
  }

  if (_spk.format != FORMAT_SPDIF)
    return false;

  out_spk = _spk;
  _spk.format = FORMAT_LINEAR;
  return proc.set_output(_spk) && encoder.query_input(_spk);
}

// Filter interface

void 
DVDDecoder::reset()
{
  demux.reset();
  dec.reset();
  proc.reset();

  encoder.reset();
  spdifer.reset();

  stream = 0;
  substream = 0;

  spdif_mode = SPDIF_MODE_NONE;
  state_ptr = 0;
}

bool 
DVDDecoder::query_input(Speakers _spk) const
{
  return demux.query_input(_spk) || 
         dec.query_input(_spk) || 
         proc.query_input(_spk);
}

bool 
DVDDecoder::set_input(Speakers _spk)
{
  if (!query_input(_spk))
    return false;

  in_spk = _spk;
  out_spk.sample_rate = in_spk.sample_rate;
  reset();
  return true;
}

Speakers
DVDDecoder::get_input() const
{
  return in_spk;
}

bool 
DVDDecoder::process(const Chunk *_chunk)
{
  if (_chunk->get_spk() != in_spk && !set_input(_chunk->get_spk()))
    return false;

  state_ptr = 0;
  if (demux.query_input(_chunk->get_spk()))
  {
    FILTER_SAFE(demux.process(_chunk));
    push_state(state_demux);
    return process_internal();
  }

  if ((out_spk.format == FORMAT_SPDIF) && 
      (spdif & FORMAT_MASK(_chunk->get_spk().format)))
  {
    FILTER_SAFE(spdifer.process(_chunk));
    push_state(state_spdif);
    return process_internal();
  }

  if (dec.query_input(_chunk->get_spk()))
  {
    FILTER_SAFE(dec.process(_chunk));
    push_state(state_dec);
    return process_internal();
  }

  if (proc.query_input(_chunk->get_spk()))
  {
    FILTER_SAFE(proc.process(_chunk));
    push_state(state_proc);
    return process_internal();
  }

  return false;
}

Speakers 
DVDDecoder::get_output() const
{
  return out_spk;
}

bool 
DVDDecoder::is_empty() const
{
  if (!chunk.is_empty() || chunk.is_eos())
    return false;

  for (int i = 0; i < state_ptr; i++)
    switch (state_stack[i])
    {
      case state_spdif: if (!spdifer.is_empty()) return false;
      case state_enc:   if (!encoder.is_empty()) return false;
      case state_proc:  if (!proc.is_empty())    return false;
      case state_dec:   if (!dec.is_empty())     return false;
      case state_demux: if (!demux.is_empty())   return false;
    }

  return true;
}

bool 
DVDDecoder::process_internal()
{ 
  if (!chunk.is_empty() || chunk.is_eos())
    // we did not return the chunk
    return true;

  #define PUSH_STATE(s) \
  {                     \
    push_state(s);      \
    continue;           \
  }

  #define POP_STATE  \
  {                  \
    pop_state();     \
    continue;        \
  }

  while (state_ptr)
  {
    switch (state())
    {
      case state_demux:
        ///////////////////////////////////////////////////
        // Demux

        if (demux.is_empty())
          POP_STATE;

        FILTER_SAFE(demux.get_chunk(&chunk));
 
        // ??? (eos or other kind of chunk possible)
        if (chunk.is_empty())
          continue;

        // reset on stream change
        if ((stream && stream != demux.get_stream()) ||
            (stream && substream && substream != demux.get_substream()))
        {
          // todo: send end-of-stream
          dec.reset();
          spdifer.reset();
          spdif_mode = SPDIF_MODE_NONE;
        }
        stream = demux.get_stream();
        substream = demux.get_substream();

        if ((out_spk.format == FORMAT_SPDIF) && 
            (FORMAT_MASK(chunk.get_spk().format) & spdif))
        {
          // state_demux -> state_spdif
          FILTER_SAFE(spdifer.process(&chunk));
          PUSH_STATE(state_spdif);
        }
        else if (proc.query_input(chunk.get_spk()))
        {
          // state_demux -> state_proc
          FILTER_SAFE(proc.process(&chunk));
          PUSH_STATE(state_proc);
        }
        else
        {
          // state_demux -> state_dec
          FILTER_SAFE(dec.process(&chunk));
          PUSH_STATE(state_dec);
        }

      case state_dec:
        ///////////////////////////////////////////////////
        // Decode

        if (dec.is_empty())
          POP_STATE;

        FILTER_SAFE(dec.get_chunk(&chunk));

        // state_dec -> state_proc
        FILTER_SAFE(proc.process(&chunk));
        PUSH_STATE(state_proc);

      case state_proc:
        ///////////////////////////////////////////////////
        // Processing

        if (proc.is_empty())
          POP_STATE;

        FILTER_SAFE(proc.get_chunk(&chunk));

        if (chunk.get_spk().format != FORMAT_LINEAR)
        {
          spdif_mode = SPDIF_MODE_NONE;
          return true;
        }

        // state_proc -> state_enc
        FILTER_SAFE(encoder.process(&chunk));
        PUSH_STATE(state_enc);

      case state_enc:
        ///////////////////////////////////////////////////
        // AC3 encode

        if (encoder.is_empty())
          POP_STATE;

        FILTER_SAFE(encoder.get_chunk(&chunk));

        // state_enc -> state_spdif
        FILTER_SAFE(spdifer.process(&chunk));
        PUSH_STATE(state_spdif);

      case state_spdif:
        ///////////////////////////////////////////////////
        // Format SPDIF

        if (spdifer.is_empty())
          POP_STATE;

        FILTER_SAFE(spdifer.get_chunk(&chunk));

        if (chunk.is_empty())
          POP_STATE;

        if (out_spk.format != FORMAT_SPDIF)
        {
          // SPDIF is not possible
          // state_spdif -> state_dec
          FILTER_SAFE(dec.process(&chunk));
          PUSH_STATE(state_dec);
        }

        if (state_stack[state_ptr-2] == state_enc)
          spdif_mode = SPDIF_MODE_ENCODE;
        else
          spdif_mode = SPDIF_MODE_PASSTHROUGH;
        return true;

    } // switch (state())
  } // while (state_ptr)

  ///////////////////////////////////////////////////
  // End of processing

  chunk = Chunk(out_spk, 0, 0);
  return true;
}

bool 
DVDDecoder::get_chunk(Chunk *_chunk)
{
  FILTER_SAFE(process_internal());

  *_chunk = chunk;
  chunk.set_empty();
  chunk.set_eos(false);
  return true;
}
