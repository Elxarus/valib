#ifndef VALIB_EQUALIZER_H
#define VALIB_EQUALIZER_H

#include <math.h>
#include "convolver.h"
#include "../fir/eq_fir.h"

///////////////////////////////////////////////////////////////////////////////
// Equalizer
// Just a wrapper for Convolver and EqFIR
///////////////////////////////////////////////////////////////////////////////

class Equalizer : public FilterWrapper
{
protected:
  EqFIR eq;
  Convolver conv;
  bool enabled;

public:
  Equalizer(): FilterWrapper(&conv), enabled(false)
  {}

  Equalizer(const EqBand *bands, size_t nbands): eq(bands, nbands), enabled(true)
  { conv.set_fir(&eq); }

  ~Equalizer()
  { conv.release_fir(); }

  /////////////////////////////////////////////////////////
  // Equalizer interface

  bool get_enabled() const { return enabled; }
  void set_enabled(bool enabled_)
  {
    if (enabled_ != enabled)
    {
      enabled = enabled_;
      if (enabled)
        conv.set_fir(&eq);
      else
        conv.set_fir(&fir_identity);
    }
  }

  size_t get_nbands() const { return eq.get_nbands(); };
  size_t set_bands(const EqBand *bands, size_t nbands) { return eq.set_bands(bands, nbands); }
  size_t get_bands(EqBand *bands, size_t first_band, size_t nbands) const { return eq.get_bands(bands, first_band, nbands); }
  void clear_bands() { eq.clear_bands(); }
};

#endif
