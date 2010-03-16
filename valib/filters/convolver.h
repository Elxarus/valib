#ifndef VALIB_CONVOLVER_H
#define VALIB_CONVOLVER_H

#include <math.h>
#include "../filter2.h"
#include "../fir.h"
#include "../sync.h"
#include "../buffer.h"
#include "../dsp/fft.h"


///////////////////////////////////////////////////////////////////////////////
// Convolver class
// Use impulse response to implement FIR filtering.
///////////////////////////////////////////////////////////////////////////////

class Convolver : public SamplesFilter
{
protected:
  int ver;
  FIRRef gen;
  const FIRInstance *fir;
  SyncHelper sync;

  int buf_size;
  int n, c;
  int pos;

  FFT       fft;
  Samples   filter;
  SampleBuf buf;
  Samples   fft_buf;

  int pre_samples;
  int post_samples;

  bool fir_changed() const;
  void convolve();

  enum { state_filter, state_zero, state_pass, state_gain } state;

public:
  Convolver(const FIRGen *gen_ = 0);
  ~Convolver();

  /////////////////////////////////////////////////////////
  // Handle FIR generator changes

  void set_fir(const FIRGen *gen_) { gen.set(gen_);    }
  const FIRGen *get_fir() const    { return gen.get(); }
  void release_fir()               { gen.release();    }

  /////////////////////////////////////////////////////////
  // SamplesFilter overrides

  virtual bool init(Speakers spk);
  virtual void uninit();

  virtual bool process(Chunk2 &in, Chunk2 &out);
  virtual bool flush(Chunk2 &out);
  virtual void reset();

  virtual bool need_flushing() const
  { return state == state_filter && post_samples > 0; }

  virtual bool is_inplace() const
  { return state != state_filter; }
};

#endif
