#ifndef VALIB_EQUALIZER_MCH_H
#define VALIB_EQUALIZER_MCH_H

#include <math.h>
#include "convolver_mch.h"
#include "../fir/eq_fir.h"
#include "../fir/multi_fir.h"

///////////////////////////////////////////////////////////////////////////////
// EqualizerMch
// Just a wrapper for ConvolverMch and EqFIR
///////////////////////////////////////////////////////////////////////////////

class EqualizerMch : public FilterWrapper
{
protected:
  EqFIR master;                 // master equalizer
  EqFIR ch_eq[CH_NAMES];        // channel equalizers
  MultiFIR multi_fir[CH_NAMES]; // sum of master and channel equalizers
  const FIRGen *firs[CH_NAMES];

  bool enabled;
  ConvolverMch conv;

public:
  EqualizerMch(): FilterWrapper(&conv), enabled(false)
  {
    FIRGen *master_plus_channel[2];
    master_plus_channel[0] = &master;

    for (int ch_name = 0; ch_name < CH_NAMES; ch_name++)
    {
      master_plus_channel[1] = &ch_eq[ch_name];
      multi_fir[ch_name].set(master_plus_channel, 2);
      firs[ch_name] = &multi_fir[ch_name];
    }
  }

  ~EqualizerMch()
  {
    conv.release_all_firs();
  }

  /////////////////////////////////////////////////////////
  // Equalizer interface

  bool get_enabled() const { return enabled; }
  void set_enabled(bool new_enabled)
  {
    if (new_enabled != enabled)
    {
      enabled = new_enabled;
      if (enabled)
        conv.set_all_firs(firs);
      else
        conv.release_all_firs();
    }
  }

  // Per-channel equalizers
  // CH_NONE references to master (all-channels) equalizer

  size_t get_nbands(int ch_name) const;
  size_t set_bands(int ch_name, const EqBand *bands, size_t nbands);
  size_t get_bands(int ch_name, EqBand *bands, size_t first_band, size_t nbands) const;
  void clear_bands(int ch_name);

  // Is channel equalized?
  bool is_equalized(int ch_name) const;
};

#endif
