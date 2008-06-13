#ifndef VALIB_CONVOLVER_H
#define VALIB_CONVOLVER_H

#include <math.h>
#include "../filter.h"
#include "../fir.h"
#include "../sync.h"

///////////////////////////////////////////////////////////////////////////////
// Convolver class
// Use impulse response to implement FIR filtering.
///////////////////////////////////////////////////////////////////////////////

class Convolver : public NullFilter
{
protected:
  int ver;
  FIRRef gen;
  const FIRInstance *fir;
  SyncHelper sync_helper;

  int       n, c;
  sample_t *filter;
  int      *fft_ip;
  sample_t *fft_w;

  int       pos;
  sample_t *buf[NCHANNELS];
  sample_t *delay[NCHANNELS];
  samples_t out;

  int pre_samples;
  int post_samples;

  bool init();
  void uninit();
  void process_block();

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
  // Filter interface

  virtual void reset();
  virtual bool set_input(Speakers spk);
  virtual bool process(const Chunk *chunk);
  virtual bool get_chunk(Chunk *chunk);
};

#endif
