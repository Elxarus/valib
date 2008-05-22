#ifndef VALIB_EQUALIZER_H
#define VALIB_EQUALIZER_H

#include <math.h>
#include "convolver.h"
#include "../fir/eq_fir.h"

///////////////////////////////////////////////////////////////////////////////
// Equalizer
// Just a wrapper for Convolver and EqFIR
///////////////////////////////////////////////////////////////////////////////

class Equalizer : public Filter
{
protected:
  EqFIR eq;
  Convolver conv;

public:
  Equalizer()
  { conv.set_fir(&eq); }

  Equalizer(int nbands, int *freq, double *gain): eq(nbands, freq, gain)
  { conv.set_fir(&eq); }

  ~Equalizer()
  { conv.release_fir(); }

  /////////////////////////////////////////////////////////
  // Equalizer interface

  size_t get_nbands() const { return eq.get_nbands(); };
  bool set_bands(size_t nbands, int *freq, double *gain) { return eq.set_bands(nbands, freq, gain); }
  void get_bands(int *freq, double *gain) const { eq.get_bands(freq, gain); }
  void reset_eq() { eq.reset(); }

  /////////////////////////////////////////////////////////
  // Filter interface

  virtual void reset()                     { conv.reset();                 }
  virtual bool is_ofdd() const             { return conv.is_ofdd();        }

  virtual bool query_input(Speakers spk) const { return conv.query_input(spk); }
  virtual bool set_input(Speakers spk)     { return conv.set_input(spk);   }
  virtual Speakers get_input() const       { return conv.get_input();      }
  virtual bool process(const Chunk *chunk) { return conv.process(chunk);   }

  virtual Speakers get_output() const      { return conv.get_output();     }
  virtual bool is_empty() const            { return conv.is_empty();       }
  virtual bool get_chunk(Chunk *chunk)     { return conv.get_chunk(chunk); }
};

#endif
