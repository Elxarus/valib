/*
  Bass redirection filter
*/


#ifndef BASS_REDIR_H
#define BASS_REDIR_H

#include "filter.h"

// 2nd order IIR filter abstract base class
class IIR2
{
protected:
  double sample_rate;
  double freq;

  double a, a1, a2, b1, b2;
  sample_t x1, x2, y1, y2;
  virtual void update() = 0; // calculates a, a1, a2, b1, b2 coefs

public:
  IIR2();

  inline double get_sample_rate();
  inline void   set_sample_rate(double _sample_rate);

  inline double get_freq();
  inline void   set_freq(double _freq);

  inline void reset();
  void process(sample_t *samples, int nsamples);
};

// High-pass filter class
class HPF : public IIR2
{
protected:
  void update();
};

// Low-pass filter class
class LPF : public IIR2
{
protected:
  void update();
};

// Bass redirection filter class
class BassRedir : public NullFilter
{
protected:
  bool enabled;
  double freq;
  double bandwidth;

  HPF hpf[5];  // High-pass filters for all channels (currently disabled)
  LPF lpf;     // Low-pass filter for subwoofer channel

public:
  BassRedir();

  inline bool   get_enabled();
  inline void   set_enabled(bool _enabled);

  inline double get_freq();
  inline void   set_freq(double _freq);

  // Filter interface
  virtual void reset();
  virtual bool process(const Chunk *chunk);
};


///////////////////////////////////////////////////////////////////////////////
// IIR inlines
///////////////////////////////////////////////////////////////////////////////

inline double IIR2::get_sample_rate()
{ return sample_rate;  } 

inline double IIR2::get_freq()
{ return freq;  } 

inline void IIR2::set_sample_rate(double _sample_rate)
{ 
  sample_rate = _sample_rate; 
  update(); 
  reset(); 
}

inline void IIR2::set_freq(double _freq)
{ 
  freq = _freq; 
  update(); 
  reset(); 
}

inline void IIR2::reset()
{
  x1 = 0;
  x2 = 0;
  y1 = 0;
  y2 = 0;
}

///////////////////////////////////////////////////////////////////////////////
// BassRedir inlines
///////////////////////////////////////////////////////////////////////////////

inline bool BassRedir::get_enabled()
{ return enabled; }

inline double BassRedir::get_freq()
{ return freq; }

inline void BassRedir::set_enabled(bool _enabled)
{
  if (_enabled && !enabled)
  {
    hpf[0].reset();
    hpf[1].reset();
    hpf[2].reset();
    hpf[3].reset();
    hpf[4].reset();
    lpf.reset();
  }
  enabled = _enabled;
}

inline void BassRedir::set_freq(double _freq)
{
  freq = _freq;
  if (freq < 10) freq = 10; // fool protection
  hpf[0].set_freq(freq);
  hpf[1].set_freq(freq);
  hpf[2].set_freq(freq);
  hpf[3].set_freq(freq);
  hpf[4].set_freq(freq);
  lpf.set_freq(freq);
}

#endif
