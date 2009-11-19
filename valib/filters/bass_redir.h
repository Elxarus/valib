/*
  Bass redirection filter
*/

#ifndef VALIB_BASS_REDIR_H
#define VALIB_BASS_REDIR_H

#include "../buffer.h"
#include "../filter.h"
#include "../iir.h"

///////////////////////////////////////////////////////////////////////////////
// Bass Redir - bass redirection filter class
///////////////////////////////////////////////////////////////////////////////

class BassRedir : public NullFilter
{
protected:
  bool      enabled;
  int       freq;
  sample_t  gain;
  int       ch_mask;

  Samples   buf;
  IIRFilter hpf[NCHANNELS];
  IIRFilter lpf;

  void update_filters(Speakers spk);

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
  int      get_freq() const;
  void     set_freq(int _freq);

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
