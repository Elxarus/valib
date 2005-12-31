#include <math.h>
#include "bass_redir.h"

///////////////////////////////////////////////////////////////////////////////
// IIR, HPF, LPF
///////////////////////////////////////////////////////////////////////////////

void
IIR::process(sample_t *_samples, int _nsamples)
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

  double omega = 2 * M_PI * freq / sample_rate;
  double s = sin(omega);
  double c = cos(omega);
  double alfa = s * sinh(log(2) / 2 * omega / s);

  a  = gain * (1 + c) / 2 / (1 + alfa);
  a1 = gain * -(1 + c) / (1 + alfa);
  a2 = gain * (1 + c) / 2 / (1 + alfa);
  b1 = -(2 * c) / (1 + alfa);
  b2 = (1 - alfa) / (1 + alfa);

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

  double omega = 2 * M_PI * freq / sample_rate;
  double s = sin(omega);
  double c = cos(omega);
  double alfa = s * sinh(log(2) / 2 * omega / s);

  a  = gain * (1 - c) / 2 / (1 + alfa);
  a1 = gain * (1 - c) / (1 + alfa);
  a2 = gain * (1 - c) / 2 / (1 + alfa);
  b1 = -(2 * c) / (1 + alfa);
  b2 = (1 - alfa) / (1 + alfa);
}



///////////////////////////////////////////////////////////////////////////////
// BassRedir
///////////////////////////////////////////////////////////////////////////////

BassRedir::BassRedir()
{
  // use default lpf filter setup (passthrough)
  enabled = false;
}


void
BassRedir::reset()
{
  NullFilter::reset();
  lpf.reset();
}

bool
BassRedir::set_input(Speakers _spk)
{
  if (!NullFilter::set_input(_spk))
    return false;

  lpf.sample_rate = spk.sample_rate;
  lpf.update();
  lpf.reset();

  return true;
}

bool 
BassRedir::process(const Chunk *_chunk)
{
  if (!receive_chunk(_chunk))
    return false;

  if (!enabled || !spk.lfe())
    return true;

  int ch;
  int nch = spk.nch();

  // mix all channels to LFE
  for (ch = 0; ch < nch - 1; ch++)
  {
    sample_t *c   = samples[ch];
    sample_t *lfe = samples[nch-1];
    
    int i = size >> 2;
    while (i--)
    {
      lfe[0] += c[0];
      lfe[1] += c[1];
      lfe[2] += c[2];
      lfe[3] += c[3];
      lfe += 4;
      c += 4;
    }

    i = size & 3;
    while (i--)
      *lfe++ += *c++;
  }

  // Filter LFE channel
  lpf.process(samples[nch-1], size);

  return true;
}

