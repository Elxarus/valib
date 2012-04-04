#include "proc.h"

static const int format_mask = 
  FORMAT_MASK_LINEAR | 
  FORMAT_MASK_PCM16 | FORMAT_MASK_PCM24 | FORMAT_MASK_PCM32 |
  FORMAT_MASK_PCM16_BE | FORMAT_MASK_PCM24_BE | FORMAT_MASK_PCM32_BE | 
  FORMAT_MASK_PCMFLOAT | FORMAT_MASK_PCMDOUBLE |
  FORMAT_MASK_LPCM20 | FORMAT_MASK_LPCM24;

AudioProcessor::AudioProcessor(size_t _nsamples)
:in_conv(_nsamples), mixer(_nsamples), agc(_nsamples), drc(_nsamples), out_conv(_nsamples)
{
  rebuild = false;
  new_stream_state = no_new_stream;

  dithering = DITHER_AUTO;
  user_spk = spk_unknown;
  rebuild_chain();
}

bool 
AudioProcessor::query_user(Speakers new_user_spk) const
{
  return new_user_spk.is_unknown() || (FORMAT_MASK(new_user_spk.format) & format_mask) != 0;
}

bool 
AudioProcessor::set_user(Speakers new_user_spk)
{
  if (user_spk == new_user_spk)
    return true;

  if (!query_user(new_user_spk))
    return false;

  user_spk = new_user_spk;
  rebuild = true;
  return true;
}

Speakers
AudioProcessor::get_user() const
{
  return user_spk;
}

string
AudioProcessor::info() const
{
  double level = dithering_level();

  std::stringstream s;
  s << std::boolalpha << std::fixed << std::setprecision(1);
  s << "User format: " << user_spk.print() << nl;
  s << "Dithering mode: ";
  switch (dithering)
  {
    case DITHER_NONE:   s << "no dithering"; break;
    case DITHER_AUTO:   s << (level == 0? "auto (disabled)": "auto (enabled)"); break;
    case DITHER_ALWAYS: s << "always dither"; break;
    default:            s << "unknown"; break;
  }
  s << nl;
  if (level != 0)
    s << "Dithering level: " << std::setprecision(0) << value2db(level) << "dB" << nl;
  s << "Filter chain:\n" << chain.info();
  return s.str();
}

Speakers 
AudioProcessor::user2output(Speakers in_spk_, Speakers user_spk_) const
{
  if (!can_open(in_spk_) || !query_user(user_spk_))
    return spk_unknown;

  Speakers result = in_spk_;
  if (user_spk_.format != FORMAT_UNKNOWN)
  {
    result.format = user_spk_.format;
    result.level = user_spk_.level;
  }

  if (user_spk_.mask)
    result.mask = user_spk_.mask;

  if (user_spk_.sample_rate)
    result.sample_rate = user_spk_.sample_rate;

  result.relation = user_spk_.relation;

  return result;
}

double
AudioProcessor::dithering_level() const
{
  // Do not apply dithering when explicitly disabled
  if (dithering == DITHER_NONE)
    return 0;

  // Do not apply dithering to floating-point output
  if (out_spk.is_floating_point() || out_spk.format == FORMAT_LINEAR)
    return 0;

  bool use_dither = false;

  if (dithering == DITHER_ALWAYS)
    use_dither = true;

  if (dithering == DITHER_AUTO)
  {
    // sample size degrade
    if (out_spk.level < in_spk.level)
      use_dither = true;

    // floating-point to integer conversion
    if (in_spk.is_floating_point() || in_spk.format == FORMAT_LINEAR)
      use_dither = true;

    // sample rate conversion
    if (user_spk.sample_rate && in_spk.sample_rate != user_spk.sample_rate)
      use_dither = true;

    // equalizer
    if (equalizer.get_enabled())
      use_dither = true;
  }

  // Return dithering level
  if (use_dither && out_spk.level > 0)
    return 0.5 / out_spk.level;
  else
    return 0.0;
}


bool 
AudioProcessor::rebuild_chain()
{
  chain.destroy();
  if (in_spk.is_unknown())
    return true;

  // Output configuration
  out_spk = user2output(in_spk, user_spk);
  if (out_spk.is_unknown())
    return false;

  // processing chain
  chain.add_back(&in_levels);
  chain.add_back(&in_cache);
  if (out_spk.nch() < in_spk.nch())
  {
    chain.add_back(&mixer);
    chain.add_back(&resample);
  }
  else
  {
    chain.add_back(&resample);
    chain.add_back(&mixer);
  }
  chain.add_back(&bass_redir);
  chain.add_back(&equalizer);
  chain.add_back(&drc);
  chain.add_back(&dither);
  chain.add_back(&agc);
  chain.add_back(&delay);
  chain.add_back(&out_cache);
  chain.add_back(&out_levels);

  // setup mixer
  Speakers mixer_spk = out_spk;
  mixer_spk.format = FORMAT_LINEAR;
  mixer.set_output(mixer_spk);

  // setup src
  resample.set_sample_rate(user_spk.sample_rate);

  // format conversion
  if (in_spk.format != FORMAT_LINEAR)
  {
    chain.add_front(&in_conv);
    in_conv.set_format(FORMAT_LINEAR);
  }

  if (out_spk.format != FORMAT_LINEAR)
  {
    chain.add_back(&out_conv);
    out_conv.set_format(out_spk.format);
  }

  dither.level = dithering_level();

  chain.open(in_spk);
  return true;
}

///////////////////////////////////////////////////////////////////////////////
// Filter interface

bool
AudioProcessor::can_open(Speakers new_spk) const
{
  return (FORMAT_MASK(new_spk.format) & format_mask) && 
         new_spk.sample_rate && new_spk.mask;
}

bool
AudioProcessor::open(Speakers new_spk)
{
  if (!can_open(new_spk)) 
  {
    close();
    return false;
  }

  in_spk = new_spk;
  if (!rebuild_chain())
  {
    close();
    return false;
  }

  rebuild = false;
  new_stream_state = no_new_stream;
  return true;
}

void
AudioProcessor::close()
{
  in_spk = spk_unknown;
  out_spk = spk_unknown;
  if (chain.is_open())
    chain.close();
}

void
AudioProcessor::reset()
{
  if (rebuild)
    rebuild_chain();

  rebuild = false;
  new_stream_state = no_new_stream;

  chain.reset();
}

bool
AudioProcessor::process(Chunk &in, Chunk &out)
{
  if (new_stream_state == show_new_stream)
    new_stream_state = no_new_stream;

  if (rebuild)
  {
    if (flush(out))
      return true;

    rebuild_chain();
    rebuild = false;
    new_stream_state = init_new_stream;
  }

  if (!chain.process(in, out))
    return false;

  if (new_stream_state == init_new_stream)
    new_stream_state = show_new_stream;

  return true;
}

bool
AudioProcessor::flush(Chunk &out)
{ return chain.flush(out); }

bool
AudioProcessor::new_stream() const
{ return (new_stream_state == show_new_stream) || chain.new_stream(); }

bool
AudioProcessor::is_open() const
{ return chain.is_open(); }

Speakers
AudioProcessor::get_input() const
{ return in_spk; }

Speakers
AudioProcessor::get_output() const
{ return out_spk; }

///////////////////////////////////////////////////////////////////////////////

AudioProcessorState *
AudioProcessor::get_state(vtime_t time)
{
  AudioProcessorState *state = new AudioProcessorState;
  if (!state) return 0;

  // Channel order
  get_input_order(state->input_order);
  get_output_order(state->output_order);
  // Master gain
  state->master = get_master();
  state->gain   = get_gain();
  // AGC options
  state->auto_gain = get_auto_gain();
  state->normalize = get_normalize();
  state->attack    = get_attack();
  state->release   = get_release();
  // DRC
  state->drc       = get_drc();
  state->drc_power = get_drc_power();
  state->drc_level = get_drc_level();
  // Matrix
  get_matrix(state->matrix);
  // Automatrix options
  state->auto_matrix      = get_auto_matrix();
  state->normalize_matrix = get_normalize_matrix();
  state->voice_control    = get_voice_control();
  state->expand_stereo    = get_expand_stereo();
  // Automatrix levels
  state->clev = get_clev();
  state->slev = get_slev();
  state->lfelev = get_lfelev();
  // Input/output gains
  get_input_gains(state->input_gains);
  get_output_gains(state->output_gains);
  // Input/output levels
  get_input_levels(time, state->input_levels);
  get_output_levels(time, state->output_levels);
  // SRC
  state->src_quality = get_src_quality();
  state->src_att     = get_src_att();
  // Equalizer
  state->eq = get_eq();
  state->eq_master_nbands = get_eq_nbands(CH_NONE);
  state->eq_master_bands = 0;
  if (state->eq_master_nbands)
  {
    state->eq_master_bands = new EqBand[state->eq_master_nbands];
    get_eq_bands(CH_NONE, state->eq_master_bands, 0, state->eq_master_nbands);
  }

  for (int ch_name = 0; ch_name < CH_NAMES; ch_name++)
  {
    state->eq_nbands[ch_name] = get_eq_nbands(ch_name);
    state->eq_bands[ch_name] = 0;
    if (state->eq_nbands[ch_name])
    {
      state->eq_bands[ch_name] = new EqBand[state->eq_nbands[ch_name]];
      get_eq_bands(ch_name, state->eq_bands[ch_name], 0, state->eq_nbands[ch_name]);
    }
  }

  // Bass redirection
  state->bass_redir = get_bass_redir();
  state->bass_freq = get_bass_freq();
  state->bass_channels = get_bass_channels();
  // Delays
  state->delay = get_delay();
  state->delay_units = get_delay_units();
  get_delays(state->delays);
  // Dithering
  state->dithering = get_dithering();

  return state;
}

void
AudioProcessor::set_state(const AudioProcessorState *state)
{
  if (!state) return;

  // Channel order
  set_input_order(state->input_order);
  set_output_order(state->output_order);
  // Master gain
  set_master(state->master);
  // AGC
  set_auto_gain(state->auto_gain);
  set_normalize(state->normalize);
  set_attack(state->attack);
  set_release(state->release);
  // DRC
  set_drc(state->drc);
  set_drc_power(state->drc_power);
  // Matrix
  // (!) Auto matrix option must be set before setting the matrix
  // because when auto matrix is on, mixer rejects the new matrix.
  set_auto_matrix(state->auto_matrix);
  set_matrix(state->matrix);
  // Automatrix options
  set_normalize_matrix(state->normalize_matrix);
  set_voice_control(state->voice_control);
  set_expand_stereo(state->expand_stereo);
  // Automatrix levels
  set_clev(state->clev);
  set_slev(state->slev);
  set_lfelev(state->lfelev);
  // Input/output gains
  set_input_gains(state->input_gains);
  set_output_gains(state->output_gains);
  // SRC
  set_src_quality(state->src_quality);
  set_src_att(state->src_att);
  // Eqalizer
  set_eq(state->eq);
  if (state->eq_master_bands)
    set_eq_bands(CH_NONE, state->eq_master_bands, state->eq_master_nbands);
  for (int ch = 0; ch < CH_NAMES; ch++)
    if (state->eq_bands[ch])
      set_eq_bands(ch, state->eq_bands[ch], state->eq_nbands[ch]);
  // Bass redirection
  set_bass_redir(state->bass_redir);
  set_bass_freq(state->bass_freq);
  set_bass_channels(state->bass_channels);
  // Delays
  set_delay(state->delay);
  set_delay_units(state->delay_units);
  set_delays(state->delays);
  // Dithering
  set_dithering(state->dithering);
}
