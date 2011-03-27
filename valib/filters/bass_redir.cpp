#include <math.h>
#include <boost/smart_ptr.hpp>
#include "bass_redir.h"
#include "../iir/linkwitz_riley.h"

#define BUF_SIZE 1024

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
  buf.allocate(BUF_SIZE);
}

void
BassRedir::update_filters()
{
  int nch = spk.nch();
  int sample_rate = spk.sample_rate;

  if (sample_rate)
  {
    boost::scoped_ptr<IIRInstance> lpf_iir(IIRLinkwitzRiley(4, freq, true).make(sample_rate));
    boost::scoped_ptr<IIRInstance> hpf_iir(IIRLinkwitzRiley(4, freq, false).make(sample_rate));

    // When we mix basses to several channels we have to
    // adjust the gain to keep the resulting loudness
    double bass_nch = mask_nch(ch_mask);
    double ch_gain = bass_nch? 1.0 / sqrt(bass_nch) : 1.0;
    lpf_iir->apply_gain(gain * ch_gain);

    lpf.init(lpf_iir.get());
    for (int i = 0; i < nch; i++)
      hpf[i].init(hpf_iir.get());
  }
  else
  {
    lpf.drop();
    for (int i = 0; i < nch; i++)
      hpf[i].drop();
  }
}

void
BassRedir::reset()
{
  int nch = spk.nch();
  lpf.reset();
  for (int i = 0; i < nch; i++)
    hpf[i].reset();
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
 
  if (!enabled)
    return true;

  // Do not filter if we have no channels to mix the bass to.
  if ((spk.mask & ch_mask) == 0)
    return true;

  // Do not filter if we have no channels to filter
  if ((spk.mask & ~ch_mask) == 0)
    return true;

  int ch, nch = spk.nch();
  order_t order;
  spk.get_order(order);
  samples_t samples = out.samples;
  size_t size = out.size;

  size_t pos = 0;
  while (pos < size)
  {
    size_t block_size = size - pos;
    if (block_size >= BUF_SIZE)
      block_size = BUF_SIZE;

    // Mix channels to be filtered
    // Skip channels where we want to mix the bass to
    buf.zero();
    for (ch = 0; ch < nch; ch++)
      if ((CH_MASK(order[ch]) & ch_mask) == 0)
        sum_samples(buf, samples[ch] + pos, block_size);

    // Filter bass channel
    lpf.process(buf, block_size);

    // Mix bass and do high-pass filtering
    for (ch = 0; ch < nch; ch++)
      if ((CH_MASK(order[ch]) & ch_mask) == 0)
        // High-pass filter
        hpf[ch].process(samples[ch] + pos, block_size);
      else
        // Mix bass
        sum_samples(samples[ch] + pos, buf, block_size);

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
