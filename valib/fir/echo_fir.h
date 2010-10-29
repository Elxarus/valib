/**************************************************************************//**
  \file echo_fir.h
  \brief EchoFIR class
******************************************************************************/

#ifndef VALIB_ECHO_FIR
#define VALIB_ECHO_FIR

#include "../fir.h"

/**************************************************************************//**
  \class EchoFIR
  \brief Single echo filter

  Filter that adds a copy of the gained signal with some delay (single echo).

  When gain is zero it degrades to passthrough filter.

  When delay is zero, it degrades to gain filter with gain 1.0 + gain.

  \fn EchoFIR::EchoFIR()
    Construct the filter with zero delay and zero gain (passthrough filter).

  \fn EchoFIR::EchoFIR(vtime_t delay, double gain);
    \param delay Delay value
    \param gain Gain value

    Construct the filter with the delay and gain specified.

  \fn void EchoFIR::set(vtime_t delay, double gain)
    \param delay Delay value
    \param gain Gain value

    Set both delay and gain.

  \fn void EchoFIR::set_delay(vtime_t delay)
    \param delay Delay value
    Set delay.

  \fn vtime_t EchoFIR::get_delay() const
    Returns current delay value.

  \fn void EchoFIR::set_gain(double gain)
    \param gain Gain value
    Set gain.

  \fn double EchoFIR::get_gain() const
    Returns current gain value.

******************************************************************************/

class EchoFIR : public FIRGen
{
protected:
  int ver;
  vtime_t delay;
  double gain;

public:
  EchoFIR();
  EchoFIR(vtime_t delay, double gain);

  /////////////////////////////////////////////////////////
  // Parameters

  void set(vtime_t delay, double gain);

  void set_delay(vtime_t delay);
  vtime_t get_delay() const;

  void set_gain(double gain);
  double get_gain() const;

  /////////////////////////////////////////////////////////
  // FIRGen interface

  virtual int version() const;
  virtual const FIRInstance *make(int sample_rate) const;
};

#endif
