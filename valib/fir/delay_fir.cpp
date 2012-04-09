#include "delay_fir.h"

DelayFIR::DelayFIR()
:ver(0), delay(0)
{}

DelayFIR::DelayFIR(vtime_t delay_)
:ver(0), delay(0)
{
  set_delay(delay_);
}

void
DelayFIR::set_delay(vtime_t delay_)
{
  delay = delay_;
  if (delay < 0)
    delay = 0;
  ver++;
}

vtime_t
DelayFIR::get_delay() const
{
  return delay;
}

int DelayFIR::version() const
{
  return ver;
}

const FIRInstance *
DelayFIR::make(int sample_rate) const
{
  int samples = int(delay * sample_rate);
  DynamicFIRInstance *fir = new DynamicFIRInstance(sample_rate, samples+1, 0);
  fir->buf[samples] = 1.0;
  return fir;
}
