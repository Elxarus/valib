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

class EqualizerMch : public Filter2
{
protected:
  EqFIR master;                 // master equalizer
  EqFIR ch_eq[CH_NAMES];        // channel equalizers
  MultiFIR multi_fir[CH_NAMES]; // sum of master and channel equalizers
  const FIRGen *firs[CH_NAMES];

  bool enabled;
  ConvolverMch conv;

public:
  EqualizerMch(): enabled(false)
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
  void reset_eq(int ch_name);

  /////////////////////////////////////////////////////////
  // Open/close the filter

  virtual bool can_open(Speakers spk) const { return conv.can_open(spk); }
  virtual bool open(Speakers spk)           { return conv.open(spk);     }
  virtual void close()                      { conv.close();              }

  /////////////////////////////////////////////////////////
  // Processing

  virtual bool process(Chunk2 &in, Chunk2 &out) { return conv.process(in, out); }
  virtual bool flush(Chunk2 &out)           { return conv.flush(out);           }
  virtual void reset()                      { conv.reset();                     }
  virtual bool new_stream() const           { return conv.new_stream();         }

  // Filter state
  virtual bool is_open() const              { return conv.is_open();     }
  virtual bool is_ofdd() const              { return conv.is_ofdd();     }
  virtual Speakers get_input() const        { return conv.get_input();   }
  virtual Speakers get_output() const       { return conv.get_output();  }
};

#endif
