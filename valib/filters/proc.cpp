#include "proc.h"

static const int format_mask = 
  FORMAT_MASK_LINEAR | 
  FORMAT_MASK_PCM16 | FORMAT_MASK_PCM24 | FORMAT_MASK_PCM32 |
  FORMAT_MASK_PCM16_BE | FORMAT_MASK_PCM24_BE | FORMAT_MASK_PCM32_BE | 
  FORMAT_MASK_PCMFLOAT;

AudioProcessor::AudioProcessor(size_t _nsamples)
:in_conv(_nsamples), mixer(_nsamples), agc(_nsamples), out_conv(_nsamples)
{
  user_spk = spk_unknown;
  rebuild_chain();
}

int
AudioProcessor::get_info(char *_buf, size_t _len) const
{
  return chain.chain_text(_buf, _len);
}


bool 
AudioProcessor::query_user(Speakers _user_spk) const
{
  return user_spk.is_unknown() || (FORMAT_MASK(_user_spk.format) & format_mask) != 0;
}

bool 
AudioProcessor::set_user(Speakers _user_spk)
{
  if (user_spk != _user_spk)
  {
    if (!query_user(_user_spk))
      return false;

    user_spk = _user_spk;
    if (!rebuild_chain())
    {
      in_spk = spk_unknown;
      out_spk = spk_unknown;
      return false;
    }
  }
  return true;
}

Speakers
AudioProcessor::get_user() const
{
  return user_spk;
}

Speakers 
AudioProcessor::user2output(Speakers _in_spk, Speakers _user_spk) const
{
  if (!query_input(_in_spk) || !query_user(_user_spk))
    return spk_unknown;

  Speakers result = _in_spk;
  if (_user_spk.format != FORMAT_UNKNOWN)
  {
    result.format = _user_spk.format;
    result.level = _user_spk.level;
  }

  if (_user_spk.mask)
    result.mask = _user_spk.mask;

  if (_user_spk.sample_rate)
    result.sample_rate = _user_spk.sample_rate;

  result.relation = _user_spk.relation;

  return result;
}

double
AudioProcessor::dithering_level() const
{
  if (equalizer.get_enabled() || in_spk.sample_rate != user_spk.sample_rate)
    if (out_spk.level > 128)
      return 0.5 / out_spk.level;

  return 0.0;
}


bool 
AudioProcessor::rebuild_chain()
{
  chain.drop();
  if (in_spk.is_unknown())
    return true;

  // Output configuration
  out_spk = user2output(in_spk, user_spk);
  if (out_spk.is_unknown())
    return false;

  // processing chain
  FILTER_SAFE(chain.add_back(&in_levels, "Input levels"));
  if (out_spk.nch() < in_spk.nch())
  {
    FILTER_SAFE(chain.add_back(&mixer,     "Mixer"));
    FILTER_SAFE(chain.add_back(&equalizer, "Equalizer"));
    FILTER_SAFE(chain.add_back(&resample,  "SRC"));
  }
  else
  {
    FILTER_SAFE(chain.add_back(&equalizer, "Equalizer"));
    FILTER_SAFE(chain.add_back(&resample,  "SRC"));
    FILTER_SAFE(chain.add_back(&mixer,     "Mixer"));
  }
  FILTER_SAFE(chain.add_back(&bass_redir,"Bass redirection"));
  FILTER_SAFE(chain.add_back(&dither,    "Dither"));
  FILTER_SAFE(chain.add_back(&agc,       "AGC"));
  FILTER_SAFE(chain.add_back(&delay,     "Delay"));
  FILTER_SAFE(chain.add_back(&spectrum,  "Spectrum"));
  FILTER_SAFE(chain.add_back(&out_levels,"Output levels"));

  // setup mixer
  Speakers mixer_spk = out_spk;
  mixer_spk.format = FORMAT_LINEAR;
  FILTER_SAFE(mixer.set_output(mixer_spk));

  // setup src
  resample.set_sample_rate(user_spk.sample_rate);

  // format conversion
  if (in_spk.format != FORMAT_LINEAR)
  {
    FILTER_SAFE(chain.add_front(&in_conv, "PCM->Linear converter"));
    FILTER_SAFE(in_conv.set_format(FORMAT_LINEAR));
  }

  if (out_spk.format != FORMAT_LINEAR)
  {
    FILTER_SAFE(chain.add_back(&out_conv, "Linear->PCM converter"));
    FILTER_SAFE(out_conv.set_format(out_spk.format));
  }

  dither.level = dithering_level();

  FILTER_SAFE(chain.set_input(in_spk));
  return true;
}

// Filter interface

void 
AudioProcessor::reset()
{
  in_levels.reset();
  mixer.reset();
  resample.reset();
  bass_redir.reset();
  agc.reset();
  delay.reset();
  out_levels.reset();

  chain.reset();
}

bool
AudioProcessor::is_ofdd() const
{
  return false;
}


bool 
AudioProcessor::query_input(Speakers _spk) const
{
  return (FORMAT_MASK(_spk.format) & format_mask) && 
         _spk.sample_rate && 
         _spk.mask;
}

bool 
AudioProcessor::set_input(Speakers _spk)
{
  reset();
  if (!query_input(_spk)) 
    return false;

  in_spk = _spk;
  if (!rebuild_chain())
  {
    in_spk = spk_unknown;
    out_spk = spk_unknown;
    return false;
  }
  return true;
}

Speakers
AudioProcessor::get_input() const
{
  return chain.get_input();
}

bool 
AudioProcessor::process(const Chunk *_chunk)
{
  if (_chunk->is_dummy())
    return true;

  if (_chunk->spk != in_spk && !set_input(_chunk->spk))
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
