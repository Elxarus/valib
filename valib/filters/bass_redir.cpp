#include <math.h>
#include "bass_redir.h"

IIR2::IIR2()
{
  sample_rate = 48000;
  freq        = 120;

  a  = 1.0;
  a1 = 0;
  a2 = 0;
  b1 = 0;
  b2 = 0;
}

void
IIR2::process(sample_t *_s, int _nsamples)
{
  double x, x1, x2, y, y1, y2;

  x1 = this->x1;
  x2 = this->x2;
  y1 = this->y1;
  y2 = this->y2;

  while (_nsamples--)
  {
    x = *_s;
    y = a*x + a1*x1 + a2*x2 - b1*y1 - b2*y2;
    *_s++ = y;

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
HPF::update()
{
  double omega = 2 * M_PI * freq / sample_rate;
  double s = sin(omega);
  double c = cos(omega);
  double alfa = s * sinh(log(2) / 2 * omega / s);

  a  = (1 + c) / 2 / (1 + alfa);
  a1 = -(1 + c) / (1 + alfa);
  a2 = (1 + c) / 2 / (1 + alfa);
  b1 = -(2 * c) / (1 + alfa);
  b2 = (1 - alfa) / (1 + alfa);

}

void
LPF::update()
{
  double omega = 2 * M_PI * freq / sample_rate;
  double s = sin(omega);
  double c = cos(omega);
  double alfa = s * sinh(log(2) / 2 * omega / s);

  a  = (1 - c) / 2 / (1 + alfa);
  a1 = (1 - c) / (1 + alfa);
  a2 = (1 - c) / 2 / (1 + alfa);
  b1 = -(2 * c) / (1 + alfa);
  b2 = (1 - alfa) / (1 + alfa);
}


BassRedir::BassRedir()
{
  spk = def_spk; // have to be initialized
  enabled = true;

  hpf[0].set_sample_rate(spk.sample_rate);
  hpf[1].set_sample_rate(spk.sample_rate);
  hpf[2].set_sample_rate(spk.sample_rate);
  hpf[3].set_sample_rate(spk.sample_rate);
  hpf[4].set_sample_rate(spk.sample_rate);
  lpf.set_sample_rate(spk.sample_rate);

  set_freq(120);
}


void 
BassRedir::reset()
{
  chunk.set_empty();

  hpf[0].set_sample_rate(spk.sample_rate);
  hpf[1].set_sample_rate(spk.sample_rate);
  hpf[2].set_sample_rate(spk.sample_rate);
  hpf[3].set_sample_rate(spk.sample_rate);
  hpf[4].set_sample_rate(spk.sample_rate);
  lpf.set_sample_rate(spk.sample_rate);
}

bool 
BassRedir::process(const Chunk *_chunk)
{
  if (!NullFilter::process(_chunk))
    return false;

  if (!enabled || !spk.lfe())
    return true;

  int ch;
  int nch = spk.nch();

  // optimize: move scale factor to hpf/lpf...
  sample_t k = 1.0 / (nch - 1);

  // mix all channels to LFE
  for (ch = 0; ch < nch - 1; ch++)
  {
    sample_t *c   = chunk.samples[ch];
    sample_t *lfe = chunk.samples[nch-1];
    
    // unroll 4 times
    int i = chunk.size >> 2;
    while (i--)
    {
      lfe[0] += k * c[0];
      lfe[1] += k * c[1];
      lfe[2] += k * c[2];
      lfe[3] += k * c[3];
      lfe += 4;
      c += 4;
    }

    i = chunk.size & 3;
    while (i--)
      *lfe++ += k * *c++;
  }
/*
  // HPF for fbw channel
  for (ch = 0; ch < nch - 1; ch++)
    hpf[ch].process(chunk.samples[ch], chunk.size);
*/
  // LPF for LFE channel
  lpf.process(chunk.samples[nch-1], chunk.size);

  return true;
}

