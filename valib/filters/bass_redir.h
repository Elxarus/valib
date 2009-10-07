/*
  Bass redirection filter
  todo: much of optimizations...
*/

#ifndef VALIB_BASS_REDIR_H
#define VALIB_BASS_REDIR_H

#include "../buffer.h"
#include "../filter.h"

///////////////////////////////////////////////////////////////////////////////
// IIR - 2nd order IIR filter abstract base class
///////////////////////////////////////////////////////////////////////////////

class IIR
{
protected:
  double a, a1, a2, b1, b2;
  sample_t x1, x2, y1, y2;

public:
  double gain;
  double freq;
  double sample_rate;

  IIR()
  {
    gain = 1.0;
    freq = 0;
    sample_rate = 0;

    // passthrough filter
    a  = 1.0;
    a1 = 0;
    a2 = 0;
    b1 = 0;
    b2 = 0;

    x1 = 0;
    x2 = 0;
    y1 = 0;
    y2 = 0;
  };

  inline void reset()
  {
    x1 = 0;
    x2 = 0;
    y1 = 0;
    y2 = 0;
  };

  virtual void update() = 0; // update a, a1, a2, b1, b2

  void process(sample_t *samples, size_t nsamples);
};

///////////////////////////////////////////////////////////////////////////////
// HPF - high-pass filter class
///////////////////////////////////////////////////////////////////////////////

class HPF : public IIR
{
public:
  void update();
};

///////////////////////////////////////////////////////////////////////////////
// HPF - low-pass filter class
///////////////////////////////////////////////////////////////////////////////

class LPF : public IIR
{
public:
  void update();
};

///////////////////////////////////////////////////////////////////////////////
// Bass Redir - bass redirection filter class
///////////////////////////////////////////////////////////////////////////////

class BassRedir : public NullFilter
{
protected:
  bool     enabled;
  double   freq;
  sample_t gain;
  int      ch_mask;
  bool     do_hpf;

  Samples  buf;
  LPF hpf[NCHANNELS];
  HPF lpf;

  /////////////////////////////////////////////////////////
  // NullFilter overrides

  virtual void on_reset();
  virtual bool on_set_input(Speakers spk);
  virtual bool on_process();

public:
  BassRedir();

  /////////////////////////////////////////////////////////
  // BassRedir interface

  // filter is enabled
  bool     get_enabled() const;
  void     set_enabled(bool _enabled);

  // cutoff frequency
  double   get_freq() const;
  void     set_freq(double _freq);

  // bass gain
  sample_t get_gain() const;
  void     set_gain(sample_t gain);

  // channels to mix the bass to
  int      get_channels() const;
  void     set_channels(int ch_mask);

  // do high-pass filtering
  bool     get_hpf() const;
  void     set_hpf(bool do_hpf);
};

#endif
