#include <math.h>
#include <boost/smart_ptr.hpp>
#include "bass_redir.h"
#include "../iir/crossover.h"

static const size_t buf_size = 1024;
static const vtime_t level_period = 0.1; // 100ms level average period

///////////////////////////////////////////////////////////////////////////////
// BassRedir
///////////////////////////////////////////////////////////////////////////////

BassRedir::BassRedir()
{
  // use default lpf filter setup (passthrough)
  enabled = false;
  freq = 80;
  gain = 1.0;
  ch_mask = CH_MASK_LFE;
  level = 0;

  buf.allocate(buf_size);
}

void
BassRedir::update_filters()
{
  int ch, nch = spk.nch();
  int sample_rate = spk.sample_rate;
  order_t order;
  spk.get_order(order);

  if (sample_rate)
  {
    boost::scoped_ptr<IIRInstance> lpf_iir(IIRCrossover(4, freq, IIRCrossover::lowpass).make(sample_rate));
    boost::scoped_ptr<IIRInstance> hpf_iir(IIRCrossover(4, freq, IIRCrossover::highpass).make(sample_rate));
    boost::scoped_ptr<IIRInstance> apf_iir(IIRCrossover(4, freq, IIRCrossover::allpass).make(sample_rate));

    // When we mix basses to several channels we have to
    // adjust the gain to keep the resulting loudness
    int bass_nch = mask_nch(ch_mask);
    double ch_gain = bass_nch? 1.0 / sqrt((double)bass_nch) : 1.0;
    lpf_iir->apply_gain(gain * ch_gain);

    lpf.init(lpf_iir.get());
    for (ch = 0; ch < nch; ch++)
      if ((CH_MASK(order[ch]) & ch_mask) == 0)
        f[ch].init(hpf_iir.get());
      else
        f[ch].init(apf_iir.get());

    for (ch = nch; ch < NCHANNELS; ch++)
      f[ch].drop();
  }
  else
  {
    lpf.drop();
    for (int i = 0; i < nch; i++)
      f[i].drop();
  }
}

void
BassRedir::reset()
{
  int nch = spk.nch();
  lpf.reset();
  for (int ch = 0; ch < nch; ch++)
    f[ch].reset();

  level = 0;
  level_accum = 0;
  level_samples = 0;
}

bool
BassRedir::init()
{
  update_filters();
  return true;
}

bool 
BassRedir::process(Chunk &in, Chunk &out)
{
  // Passthrough (process inplace later)
  out = in;
  in.clear();
  if (out.is_dummy())
    return false;
 
  if (!enabled ||
      (spk.mask &  ch_mask) == 0 || // Do not filter if we have no channels to mix the bass to.
      (spk.mask & ~ch_mask) == 0)   // Do not filter if we have no channels to filter
  {
    level = 0;
    return true;
  }

  int ch, nch = spk.nch();
  order_t order;
  spk.get_order(order);
  samples_t samples = out.samples;
  size_t size = out.size;

  size_t pos = 0;
  while (pos < size)
  {
    size_t block_size = buf.size();
    if (block_size > size - pos)
      block_size = size - pos;

    // Mix channels to be filtered
    // Skip channels where we want to mix the bass to
    buf.zero();
    for (ch = 0; ch < nch; ch++)
      if ((CH_MASK(order[ch]) & ch_mask) == 0)
        sum_samples(buf, samples[ch] + pos, block_size);

    // Filter bass channel
    lpf.process(buf, block_size);

    // Find bass level
    level_accum = max_samples(level_accum, buf, block_size);
    level_samples += block_size;
    if (level_samples > level_period * spk.sample_rate)
    {
      level = level_accum / spk.level;
      level_accum = 0;
      level_samples = 0;
    }

    // Do filtering and mix bass
    for (ch = 0; ch < nch; ch++)
      if ((CH_MASK(order[ch]) & ch_mask) == 0)
      {
        // High-pass filter
        f[ch].process(samples[ch] + pos, block_size);
      }
      else
      {
        // Allpass and mix bass
        f[ch].process(samples[ch] + pos, block_size);
        sum_samples(samples[ch] + pos, buf, block_size);
      }

    // Next block
    pos += block_size;
  }

  return true;
}

///////////////////////////////////////////////////////////////////////////////
// BassRedir interface

bool 
BassRedir::get_enabled() const
{
  return enabled;
}

void 
BassRedir::set_enabled(bool enabled_)
{
  // Need reset on disabled to enabled state change
  if (enabled_ && !enabled)
    reset();
  enabled = enabled_;
}

bool
BassRedir::is_active() const
{
  return is_open() && enabled && ((spk.mask & ch_mask) != 0) && ((spk.mask & ~ch_mask) != 0);
}

sample_t
BassRedir::get_level() const
{
  return level;
}

int
BassRedir::get_freq() const
{
  return freq;
}

void 
BassRedir::set_freq(int freq_)
{
  if (freq != freq_)
  {
    freq = freq_;
    update_filters();
  }
}

sample_t
BassRedir::get_gain() const
{
  return gain;
}

void 
BassRedir::set_gain(sample_t gain_)
{
  if (gain != gain_)
  {
    gain = gain_;
    update_filters();
  }
}

int
BassRedir::get_channels() const
{
  return ch_mask;
}

void
BassRedir::set_channels(int ch_mask_)
{
  if (ch_mask != ch_mask_)
  {
    ch_mask = ch_mask_ & CH_MASK_ALL;
    update_filters();
  }
}
