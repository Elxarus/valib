#include "proc.h"


AudioProcessor::AudioProcessor()
{
  in_spk  = Speakers(FORMAT_LINEAR, MODE_STEREO, 44100);
  out_spk = Speakers(FORMAT_LINEAR, MODE_STEREO, 44100);
  rebuild_chain();
}

bool 
AudioProcessor::query_spk(Speakers _spk) const
{
  return (_spk.format == FORMAT_LINEAR)   ||
         (_spk.format == FORMAT_PCM16)    ||
         (_spk.format == FORMAT_PCM24)    ||
         (_spk.format == FORMAT_PCM32)    ||
         (_spk.format == FORMAT_PCMFLOAT) ||
         (_spk.format == FORMAT_PCM16_LE) ||
         (_spk.format == FORMAT_PCM24_LE) ||
         (_spk.format == FORMAT_PCM32_LE) ||
         (_spk.format == FORMAT_PCMFLOAT_LE);
}

bool 
AudioProcessor::set_input(Speakers _spk)
{
  if (in_spk != _spk)
  {
    if (!query_input(_spk)) return false;
    in_spk = _spk;
    rebuild_chain();
  }
  return true;
}

bool 
AudioProcessor::set_output(Speakers _spk)
{
  if (out_spk != _spk)
  {
    if (!query_spk(_spk)) return false;
    out_spk = _spk;
    rebuild_chain();
  }
  return true;
}

void 
AudioProcessor::rebuild_chain()
{
  out_spk.sample_rate = in_spk.sample_rate;

  chain.clear();

  if (in_spk.format != FORMAT_LINEAR)
  {
    chain.add(&conv1, "PCM->Linear converter");
    conv1.set_format(FORMAT_LINEAR);
  }

  chain.add(&in_levels, "Input levels");
  chain.add(&mixer,     "Mixer");
  chain.add(&bass_redir,"Bass redirection");
  chain.add(&agc,       "AGC");
  chain.add(&delay,     "Delay");
  chain.add(&out_levels,"Output levels");
  chain.add(&dejitter,  "Dejitter");

  Speakers mixer_spk = out_spk;
  mixer_spk.format = FORMAT_LINEAR;
  mixer.set_output(mixer_spk);

  if (out_spk.format != FORMAT_LINEAR)
  {
    chain.add(&conv2, "Linear->PCM converter");
    conv2.set_format(out_spk.format);
  }

  chain.reset();

  chain.set_input(in_spk);
  out_spk = chain.get_output();
}

// Filter interface

void 
AudioProcessor::reset()
{
  chunk.set_empty();
  chain.reset();
}

bool 
AudioProcessor::query_input(Speakers _spk) const
{
  return query_spk(_spk);
}

bool 
AudioProcessor::process(const Chunk *_chunk)
{
  if (_chunk->is_empty())
    return true;

  if (_chunk->spk != in_spk && !set_input(_chunk->spk))
    return false;
  else
    return chain.process(_chunk);
}

Speakers 
AudioProcessor::get_output()
{
  return out_spk;
}

bool 
AudioProcessor::is_empty()
{
  return chain.is_empty();
}

bool 
AudioProcessor::get_chunk(Chunk *_chunk)
{
  return chain.get_chunk(_chunk);
}
