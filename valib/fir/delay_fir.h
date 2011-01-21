/**************************************************************************//**
  \file delay_fir.h
  \brief DelayFIR class
******************************************************************************/

#ifndef VALIB_DELAY_FIR
#define VALIB_DELAY_FIR

#include "../fir.h"

/**************************************************************************//**
  \class DelayFIR
  \brief Delay filter

  Delays the signal by an integer number of samples.

  When delay iz zero filter degrades to passthrough filter.

  \fn DelayFIR::DelayFIR()
    Construct the filter with zero delay (passthrough filter).

  \fn DelayFIR::DelayFIR(vtime_t delay)
    \param delay Delay value

    Construct the filter with the delay specified.

  \fn void DelayFIR::set_delay(vtime_t delay)
    \param delay Delay value

    Set the delay. Must be positive, zero delay is set otherwise.

  \fn vtime_t DelayFIR::get_delay() const
    Returns the current delay.
******************************************************************************/

class DelayFIR : public FIRGen
{
protected:
  int ver;
  vtime_t delay;

public:
  DelayFIR();
  DelayFIR(vtime_t delay);

  /////////////////////////////////////////////////////////
  // Parameters

  void set_delay(vtime_t delay);
  vtime_t get_delay() const;

  /////////////////////////////////////////////////////////
  // FIRGen interface

  virtual int version() const;
  virtual const FIRInstance *make(int sample_rate) const;
};

#endif
