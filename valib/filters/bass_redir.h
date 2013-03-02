/**************************************************************************//**
  \file bass_redir.h
  \brief BassRedir: Bass redirection filter
******************************************************************************/

#ifndef VALIB_BASS_REDIR_H
#define VALIB_BASS_REDIR_H

#include "../buffer.h"
#include "../filter.h"
#include "../iir.h"

/**************************************************************************//**
  \class BassRedir
  \brief Bass redirection filter

  Properties:
  \param enabled  Enable/disable the filter. Default is false.
  \param freq     Crossover frequency. Default is 80Hz.
  \param gain     Bass gain. Default is 1.0.
  \param channels Destination channels. Default is subwoofer.

  This filter implements bass redirection. It filters bass out from main
  channels and routes it to bass channel(s).

  Filter is active only when it have something to do. When redirecting to the
  subwoofer and it is no subwoofer channel at the input format, filter does
  nothing (even when enabled).

  Filter can redirect to more than one channel (redirect to front channel for
  instance). Destination channels is a channel mask that defines these channels.
  Note, that when redirecting to more than one channel an additional gain is
  automatically applied to cancel the loudness difference (bass reproduced with
  two speakers is louder than bass reproduced with one speaker).

  It is inplace, linear format filter.

  \fn bool BassRedir::get_enabled() const
    Returns true when filter is enabled.

  \fn void BassRedir::set_enabled(bool enabled)
    \param enabled True to enable and false to disable bass redirection.

    Allows to enable/disable the filter. When set to false filter does nothing.
    When set to true filter does bass redirection to destination channels only.
    If input format does not have these channels filter does nothing (enabled
    but inactive).

  \fn bool BassRedir::is_active() const
    Returns true when filter is enabled and active.

    Filter may be inactive due to:
    - Filter closed or disabled
    - Input format does not include destination channel(s). Example: format is
      stereo (2.0) and bass destination is subwoofer.
    - Input format includes destination channels only. Example: format is
      stereo and bass destination is front channels.

  \fn sample_t BassRedir::get_level() const
    Returns current bass level (level at the low-pass filter's output). Useful
    for monitoring the activity of the filter.

  \fn int BassRedir::get_freq() const
    Returns current crossover freuqency.

  \fn void BassRedir::set_freq(int freq)
    \param freq Crossover frequency.

    Set the crossover frequency.

  \fn sample_t BassRedir::get_gain() const
    Returns current bass gain.

  \fn void BassRedir::set_gain(sample_t gain)
    \param gain Bass gain.

    Set bass gain. Note, that when redirecting to more than one channel, an
    addition gain is applied that cancels the loudness difference (-3dB for
    2 channels). So the resulting gain is a multiple of both.

  \fn int BassRedir::get_channels() const
    Returns current bass destination channels.

  \fn void BassRedir::set_channels(int ch_mask)
    \param ch_mask Channel mask for destination channels.

    Sets bass destination channels.

******************************************************************************/

class BassRedir : public SamplesFilter
{
public:
  BassRedir();

  /////////////////////////////////////////////////////////
  // BassRedir interface

  // filter is enabled
  bool     get_enabled() const;
  void     set_enabled(bool enabled);

  // Redirection activity
  bool     is_active() const;
  sample_t get_level() const;

  // cutoff frequency
  int      get_freq() const;
  void     set_freq(int freq);

  // bass gain
  sample_t get_gain() const;
  void     set_gain(sample_t gain);

  // bass destination channel(s)
  int      get_channels() const;
  void     set_channels(int ch_mask);

  /////////////////////////////////////////////////////////
  // SamplesFilter overrides

  virtual void reset();
  virtual bool init();
  virtual bool process(Chunk &in, Chunk &out);
  virtual string info() const;

protected:
  bool      enabled;        //!< Enabled flag
  int       freq;           //!< Crossover frequency
  sample_t  gain;           //!< Bass gain
  int       ch_mask;        //!< Destination channels
  sample_t  level;          //!< Current bass level
  sample_t  level_accum;    //!< Level accumulator
  size_t    level_samples;  //!< Number of samples accumulated

  Samples   buf;            //!< Bass channel buffer
  IIRFilter f[NCHANNELS];   //!< Channel filters
  IIRFilter lpf;            //!< Bass channel lowpass filter

  void update_filters();    //!< Recalculate filters
};

#endif
