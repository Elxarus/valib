#ifndef VALIB_CONVOLVER_H
#define VALIB_CONVOLVER_H

#include <math.h>
#include "../filter.h"
#include "../fir.h"

///////////////////////////////////////////////////////////////////////////////
// Convolver class
// Use impulse response to implement FIR filtering.
///////////////////////////////////////////////////////////////////////////////

class Convolver : public NullFilter
{
protected:
  int ver;
  ImpulseResponseRef ir;

  int       n, c;
  sample_t *filter;
  int      *fft_ip;
  sample_t *fft_w;

  int       pos;
  sample_t *buf[NCHANNELS];
  sample_t *delay[NCHANNELS];
  samples_t out;

  int pre_samples;

  void init();
  void uninit();
  void process();

  enum { state_filter, state_zero, state_pass } state;

public:
  Convolver(const ImpulseResponse *_ir = 0);
  ~Convolver();

  /////////////////////////////////////////////////////////
  // Handle impulse response changes

  void set_ir(const ImpulseResponse *_ir) { ir.set(_ir);     }
  const ImpulseResponse *get_ir() const   { return ir.get(); }
  void release_ir()                       { ir.release();    }

  /////////////////////////////////////////////////////////
  // Filter interface

  bool set_input(Speakers _spk);
  void reset();
  bool get_chunk(Chunk *chunk);
};

#endif
