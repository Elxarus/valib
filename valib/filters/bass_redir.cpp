#include <math.h>
#include "bass_redir.h"

#define BUF_SIZE 1024

///////////////////////////////////////////////////////////////////////////////
// IIR, HPF, LPF
///////////////////////////////////////////////////////////////////////////////

void
IIR::process(sample_t *_samples, size_t _nsamples)
{
  double x, x1, x2, y, y1, y2;

  x1 = this->x1;
  x2 = this->x2;
  y1 = this->y1;
  y2 = this->y2;

  while (_nsamples--)
  {
    x = *_samples;
    y = a*x + a1*x1 + a2*x2 - b1*y1 - b2*y2;
    *_samples++ = y;

    x2 = x1;
    x1 = x;
    y2 = y1;
    y1 = y;
  }

  this->x1 = x1;
  this->x2 = x2;
  this->y1 = y1;
  this->y2 = y2;
}


void
LPF::update()
{
  if ((sample_rate < 10) || (freq < 10))
  {
    // setup as passthrough on incorrect parameters
    a  = 1.0;
    a1 = 0;
    a2 = 0;
    b1 = 0;
    b2 = 0;
    return;
  }

  double omega = 2.0 * M_PI * freq / sample_rate;
  double s = sin(omega);
  double c = cos(omega);
  double alfa = s * sinh(log(2.0) / 2.0 * omega / s);

  a  = gain * (1.0 + c) / 2.0 / (1.0 + alfa);
  a1 = gain * -(1.0 + c) / (1.0 + alfa);
  a2 = gain * (1.0 + c) / 2.0 / (1.0 + alfa);
  b1 = -(2.0 * c) / (1.0 + alfa);
  b2 = (1.0 - alfa) / (1.0 + alfa);

}

void
HPF::update()
{
  if ((sample_rate < 10) || (freq < 10))
  {
    // setup as passthrough on incorrect parameters
    a  = 1.0;
    a1 = 0;
    a2 = 0;
    b1 = 0;
    b2 = 0;
    return;
  }

  double omega = 2.0 * M_PI * freq / sample_rate;
  double s = sin(omega);
  double c = cos(omega);
  double alfa = s * sinh(log(2.0) / 2.0 * omega / s);

  a  = gain * (1.0 - c) / 2.0 / (1.0 + alfa);
  a1 = gain * (1.0 - c) / (1.0 + alfa);
  a2 = gain * (1.0 - c) / 2.0 / (1.0 + alfa);
  b1 = -(2.0 * c) / (1.0 + alfa);
  b2 = (1.0 - alfa) / (1.0 + alfa);
}



///////////////////////////////////////////////////////////////////////////////
// BassRedir
///////////////////////////////////////////////////////////////////////////////

double channels_gain(int mask)
{
  // When we mix basses to several channels we have to
  // adjust the gain to keep the resulting loudness
  double bass_nch = mask_nch(mask);
  return bass_nch? 1.0 / sqrt(bass_nch) : 1.0;
}

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
BassRedir::on_reset()
{
  lpf.reset();
  for (int i = 0; i < NCHANNELS; i++)
    hpf[i].reset();
}

bool
BassRedir::on_set_input(Speakers _spk)
{
  lpf.sample_rate = _spk.sample_rate;
  lpf.freq = freq;
  lpf.gain = gain * channels_gain(ch_mask & _spk.mask);
  lpf.update();
  lpf.reset();

  for (int i = 0; i < NCHANNELS; i++)
  {
    hpf[i].sample_rate = _spk.sample_rate;
    hpf[i].freq = freq;
    hpf[i].gain = 1.0;
    hpf[i].update();
    hpf[i].reset();
  }

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
  if (_enabled && !enabled)
  {
    lpf.reset();
    for (int i = 0; i < NCHANNELS; i++)
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
  freq = _freq;

  lpf.freq = _freq;
  lpf.update();
  for (int i = 0; i < NCHANNELS; i++)
  {
    hpf[i].freq = _freq;
    hpf[i].update();
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
  gain = _gain;
  lpf.gain = gain * channels_gain(ch_mask & spk.mask);
  lpf.update();
}

int
BassRedir::get_channels() const
{
  return ch_mask;
}

void
BassRedir::set_channels(int _ch_mask)
{
  ch_mask = _ch_mask & CH_MASK_ALL;
  lpf.gain = gain * channels_gain(ch_mask & spk.mask);
  lpf.update();
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
