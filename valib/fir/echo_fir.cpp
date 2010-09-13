#include "echo_fir.h"

EchoFIR::EchoFIR()
:ver(0), delay(0), gain(1.0)
{}

EchoFIR::EchoFIR(vtime_t delay_, double gain_)
:ver(0), delay(0), gain(1.0)
{
  set(delay_, gain_);
}

void
EchoFIR::set(vtime_t delay_, double gain_)
{
  delay = delay_;
  gain = gain_;
  if (delay < 0) delay = 0;
  ver++;
}

void
EchoFIR::get(vtime_t *delay_, double *gain_) const
{
  if (delay_) *delay_ = delay;
  if (gain_) *gain_ = gain;
}

void
EchoFIR::set_delay(vtime_t delay_)
{ set(delay_, gain); }

void
EchoFIR::set_gain(double gain_)
{ set(delay, gain_); }

vtime_t
EchoFIR::get_delay() const
{ return delay; }

double
EchoFIR::get_gain() const
{ return gain; }

int
EchoFIR::version() const
{
  return ver;
}

const FIRInstance *
EchoFIR::make(int sample_rate) const
{
  int samples = int(delay * sample_rate);

  if (samples == 0)
    return new GainFIRInstance(sample_rate, gain);

  DynamicFIRInstance *fir = new DynamicFIRInstance(sample_rate, samples+1, 0);

  fir->buf[0] = 1.0;
  fir->buf[samples] = gain;
  return fir;
}
