#ifndef VALIB_EQUALIZER_H
#define VALIB_EQUALIZER_H

#include <math.h>
#include "convolver.h"
#include "../fir/eq_fir.h"

///////////////////////////////////////////////////////////////////////////////
// Equalizer
// Just a wrapper for Convolver and EqFIR
///////////////////////////////////////////////////////////////////////////////

class Equalizer : public Filter2
{
protected:
  EqFIR eq;
  Convolver conv;
  bool enabled;

public:
  Equalizer(): enabled(false)
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
  void reset_eq() { eq.reset(); }

  /////////////////////////////////////////////////////////
  // Open/close the filter

  virtual bool can_open(Speakers spk) const { return conv.can_open(spk); }
  virtual bool open(Speakers spk)           { return conv.open(spk);     }
  virtual void close()                      { conv.close();              }

  virtual bool is_open() const              { return conv.is_open();     }
  virtual bool is_ofdd() const              { return conv.is_ofdd();     }
  virtual bool is_inplace() const           { return conv.is_inplace();  }
  virtual Speakers get_input() const        { return conv.get_input();   }

  /////////////////////////////////////////////////////////
  // Processing

  virtual bool process(Chunk2 &in, Chunk2 &out) { return conv.process(in, out); }
  virtual bool flush(Chunk2 &out)           { return conv.flush(out);           }
  virtual void reset()                      { conv.reset();                     }

  virtual bool eos() const                  { return conv.eos();           }
  virtual bool need_flushing() const        { return conv.need_flushing(); }
  virtual Speakers get_output() const       { return conv.get_output();    }
};

#endif
