#include "dvd_decoder.h"


DVDDecoder::DVDDecoder()
:proc(4096)
{
  in_spk  = def_spk;
  out_spk = def_spk;

  spdif = FORMAT_MASK_AC3;
  spdif_mode = SPDIF_MODE_NONE;

  encoder.set_bitrate(640000);

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
    proc.set_output(_spk);
    return true;
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
  state_stack[0] = state_none;
  empty = true;
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

  if (demux.query_input(_chunk->get_spk()))
  {
    state_stack[0] = state_demux;
    state_ptr = 0;
    if (!demux.process(_chunk))
      return false;
    empty = demux.is_empty();
    return true;
  }

  if ((out_spk.format == FORMAT_SPDIF) && 
      (spdif & FORMAT_MASK(_chunk->get_spk().format)))
  {
    state_stack[0] = state_spdif;
    state_ptr = 0;
    if (!spdifer.process(_chunk))
      return false;
    empty = spdifer.is_empty();
    return true;
  }

  if (dec.query_input(_chunk->get_spk()))
  {
    state_stack[0] = state_dec;
    state_ptr = 0;
    if (!dec.process(_chunk))
      return false;
    empty = dec.is_empty();
    return true;
  }

  if (proc.query_input(_chunk->get_spk()))
  {
    state_stack[0] = state_proc;
    state_ptr = 0;
    if (!proc.process(_chunk))
      return false;
    empty = proc.is_empty();
    return true;
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
  return empty;
}

bool 
DVDDecoder::get_chunk(Chunk *_out)
{
  _out->set_empty();
  Chunk chunk;

  #define PUSH_STATE(s)            \
  {                                \
    state_stack[++state_ptr] = s;  \
    continue;                      \
  }

  #define POP_STATE  \
  {                  \
    if (!state_ptr)  \
    {                \
      empty = true;  \
      return true;   \
    }                \
    else             \
    {                \
      state_ptr--;   \
      continue;      \
    }                \
  }

  while (1)
  {
    switch (state_stack[state_ptr])
    {
      case state_none:
        empty = true;
        return true;

      case state_demux:
        ///////////////////////////////////////////////////
        // Demux

        if (demux.is_empty())
          POP_STATE;

        if (!demux.get_chunk(&chunk))
          return false;

        if (chunk.is_empty())
          continue;

        // reset on stream change
        if ((stream && stream != demux.get_stream()) ||
            (stream && substream && substream != demux.get_substream()))
        {
          dec.reset();
          spdifer.reset();
          spdif_mode = SPDIF_MODE_NONE;
        }
        stream = demux.get_stream();
        substream = demux.get_substream();

        if ((out_spk.format == FORMAT_SPDIF) && 
            (chunk.get_spk().format & spdif))
        {
          // state_demux -> state_spdif
          if (!spdifer.process(&chunk))
            return false;
          PUSH_STATE(state_spdif);
        }
        else if (proc.query_input(chunk.get_spk()))
        {
          // state_demux -> state_proc
          if (!proc.process(&chunk))
            return false;
          PUSH_STATE(state_proc);
        }
        else
        {
          // state_demux -> state_dec
          if (!dec.process(&chunk))
            return false;
          PUSH_STATE(state_dec);
        }

      case state_dec:
        ///////////////////////////////////////////////////
        // Decode

        if (dec.is_empty())
          POP_STATE;

        if (!dec.get_chunk(&chunk))
          return false;

        // state_dec -> state_proc
        if (!proc.process(&chunk))
          return false;
        PUSH_STATE(state_proc);

      case state_proc:
        ///////////////////////////////////////////////////
        // Processing

        if (proc.is_empty())
          POP_STATE;

        if (!proc.get_chunk(&chunk))
          return false;

        if (chunk.get_spk().format != FORMAT_LINEAR)
        {
          spdif_mode = SPDIF_MODE_NONE;
          *_out = chunk;
          return true;
        }

        // state_proc -> state_enc
        if (!encoder.process(&chunk))
          return false;
        PUSH_STATE(state_enc);

      case state_enc:
        ///////////////////////////////////////////////////
        // AC3 encode

        if (encoder.is_empty())
          POP_STATE;

        if (!encoder.get_chunk(&chunk))
          return false;

        // state_proc -> state_enc
        if (!spdifer.process(&chunk))
          return false;
        PUSH_STATE(state_spdif);

      case state_spdif:
        ///////////////////////////////////////////////////
        // Format SPDIF

        if (spdifer.is_empty())
          POP_STATE;

        if (!spdifer.get_chunk(&chunk))
          return false;

        if (out_spk.format == FORMAT_SPDIF)
        {
          if (state_ptr > 1)
            spdif_mode = SPDIF_MODE_ENCODE;
          else
            spdif_mode = SPDIF_MODE_PASSTHROUGH;
          *_out = chunk;
          return true;
        }

        // state_demux -> state_dec
        if (!dec.process(&chunk))
          return false;
        PUSH_STATE(state_dec);

    } // switch (state_stack[state_ptr])
  } // while (1)
}
