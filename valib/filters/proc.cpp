#include "proc.h"


AudioProcessor::AudioProcessor(size_t _nsamples)
:conv1(_nsamples), conv2(_nsamples), mixer(_nsamples)
{
  in_spk  = def_spk;
  out_spk = def_spk;

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
    if (!query_spk(_spk)) 
      return false;

    in_spk = _spk;

    // todo: add sample rate convertor and remove this
    out_spk.sample_rate = in_spk.sample_rate;

    rebuild_chain();
  }
  return true;
}

bool 
AudioProcessor::set_output(Speakers _spk)
{
  if (out_spk != _spk)
  {
    if (!query_spk(_spk)) 
      return false;

    out_spk = _spk;

    // todo: add sample rate convertor and remove this
    out_spk.sample_rate = in_spk.sample_rate;

    rebuild_chain();
  }
  return true;
}

void 
AudioProcessor::rebuild_chain()
{
  // Output configuration
  Speakers spk = out_spk;

  spk.format = FORMAT_LINEAR;

  if (!spk.mask) 
    spk.mask = in_spk.mask;

  if (!spk.sample_rate)
    spk.sample_rate = in_spk.sample_rate;

  // processing chain
  chain.clear();
  chain.add_back(&in_levels, "Input levels");
  chain.add_back(&mixer,     "Mixer");
  chain.add_back(&bass_redir,"Bass redirection");
  chain.add_back(&agc,       "AGC");
  chain.add_back(&delay,     "Delay");
  chain.add_back(&out_levels,"Output levels");
  chain.add_back(&sync_gen,  "Synchronizer");

  // setup mixer
  mixer.set_output(spk);

  // format conversion
  if (in_spk.format != FORMAT_LINEAR)
  {
    chain.add_front(&conv1, "PCM->Linear converter");
    conv1.set_format(FORMAT_LINEAR);
  }
  if (out_spk.format != FORMAT_LINEAR)
  {
    chain.add_back(&conv2, "Linear->PCM converter");
    conv2.set_format(out_spk.format);
  }

  chain.set_input(in_spk);
}

// Filter interface

void 
AudioProcessor::reset()
{
  chain.reset();
}

bool 
AudioProcessor::query_input(Speakers _spk) const
{
  return query_spk(_spk);
}

Speakers
AudioProcessor::get_input() const
{
  return chain.get_input();
}

bool 
AudioProcessor::process(const Chunk *_chunk)
{
  if (_chunk->get_spk() != in_spk && !set_input(_chunk->get_spk()))
    return false;
  else
    return chain.process(_chunk);
}

Speakers 
AudioProcessor::get_output() const
{
  return out_spk;
}

bool 
AudioProcessor::is_empty() const
{
  return chain.is_empty();
}

bool 
AudioProcessor::get_chunk(Chunk *_chunk)
{
  return chain.get_chunk(_chunk);
}
