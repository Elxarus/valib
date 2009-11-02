#include <math.h>
#include "bass_redir.h"
#include "../iir/linkwitz_riley.h"

#define BUF_SIZE 1024

///////////////////////////////////////////////////////////////////////////////
// BassRedir
///////////////////////////////////////////////////////////////////////////////

BassRedir::BassRedir()
:NullFilter(FORMAT_MASK_LINEAR)
{
  // use default lpf filter setup (passthrough)
  enabled = false;
  freq = 80;
  gain = 1.0;
  ch_mask = CH_MASK_LFE;
  buf.allocate(BUF_SIZE);
}

void
BassRedir::update_filters(Speakers _spk)
{
  int nch = _spk.nch();
  int sample_rate = _spk.sample_rate;

  if (sample_rate)
  {
    IIRInstance *lpf_iir = IIRLinkwitzRiley(4, freq, true).make(sample_rate);
    IIRInstance *hpf_iir = IIRLinkwitzRiley(4, freq, false).make(sample_rate);

    // When we mix basses to several channels we have to
    // adjust the gain to keep the resulting loudness
    double bass_nch = mask_nch(ch_mask);
    double ch_gain = bass_nch? 1.0 / sqrt(bass_nch) : 1.0;
    lpf_iir->apply_gain(gain * ch_gain);

    lpf.init(lpf_iir);
    for (int i = 0; i < nch; i++)
      hpf[i].init(hpf_iir);

    safe_delete(lpf_iir);
    safe_delete(hpf_iir);
  }
  else
  {
    lpf.uninit();
    for (int i = 0; i < nch; i++)
      hpf[i].uninit();
  }
}

void
BassRedir::on_reset()
{
  int nch = spk.nch();
  lpf.reset();
  for (int i = 0; i < nch; i++)
    hpf[i].reset();
}

bool
BassRedir::on_set_input(Speakers _spk)
{
  update_filters(_spk);
  return true;
}

bool 
BassRedir::on_process()
{
  if (!enabled)
    return true;

  // Do not filter if we have no channels to mix the bass to.
  if ((spk.mask & ch_mask) == 0)
    return true;

  // Do not filter if we have no channels to filter
  if ((spk.mask & ~ch_mask) == 0)
    return true;

  int ch;
  int nch = spk.nch();
  order_t order;
  spk.get_order(order);

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
      {
        sample_t *sptr = samples[ch] + pos;
        for (size_t i = 0; i < block_size; i++)
          buf[i] += sptr[i];
      }

    // Filter bass channel
    lpf.process(buf, block_size);

    // Mix bass and do high-pass filtering
    for (ch = 0; ch < nch; ch++)
      if ((CH_MASK(order[ch]) & ch_mask) == 0)
      {
        // High-pass filter
        if (do_hpf)
          hpf[ch].process(samples[ch] + pos, block_size);
      }
      else
      {
        // Mix bass
        sample_t *sptr = samples[ch] + pos;
        for (size_t i = 0; i < block_size; i++)
          sptr[i] += buf[i];
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
BassRedir::set_enabled(bool _enabled)
{
  int nch = spk.nch();
  if (_enabled && !enabled)
  {
    lpf.reset();
    for (int i = 0; i < nch; i++)
      hpf[i].reset();
  }
  enabled = _enabled;
}

double 
BassRedir::get_freq() const
{
  return freq;
}

void 
BassRedir::set_freq(double _freq)
{
  if (freq != _freq)
  {
    freq = _freq;
    update_filters(spk);
  }
}

sample_t
BassRedir::get_gain() const
{
  return gain;
}

void 
BassRedir::set_gain(sample_t _gain)
{
  if (gain != _gain)
  {
    gain = _gain;
    update_filters(spk);
  }
}

int
BassRedir::get_channels() const
{
  return ch_mask;
}

void
BassRedir::set_channels(int _ch_mask)
{
  if (ch_mask != _ch_mask)
  {
    ch_mask = _ch_mask & CH_MASK_ALL;
    update_filters(spk);
  }
}

bool
BassRedir::get_hpf() const
{
  return do_hpf;
}

void
BassRedir::set_hpf(bool _do_hpf)
{
  do_hpf = _do_hpf;
}
