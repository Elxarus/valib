#ifndef VALIB_CONVOLVER_MCH_H
#define VALIB_CONVOLVER_MCH_H

#include <math.h>
#include "../filter.h"
#include "../fir.h"
#include "../sync.h"
#include "../buffer.h"
#include "../dsp/fft.h"


///////////////////////////////////////////////////////////////////////////////
// Multichannel convolver class
// Use impulse response to implement FIR filtering.
///////////////////////////////////////////////////////////////////////////////

class ConvolverMch : public SamplesFilter
{
protected:
  int ver[CH_NAMES];
  FIRRef gen[CH_NAMES];

  SyncHelper sync;

  bool trivial;
  const FIRInstance *fir[NCHANNELS];
  enum { type_pass, type_gain, type_zero, type_conv } type[NCHANNELS];

  int buf_size;
  int n, c;
  int pos;

  FFT       fft;
  SampleBuf filter;
  SampleBuf buf;
  Samples   fft_buf;

  int pre_samples;
  int post_samples;

  bool fir_changed() const;
  bool need_flushing() const
  { return !trivial && post_samples > 0; }

  void process_trivial(samples_t samples, size_t size);
  void process_convolve();

public:
  ConvolverMch();
  ~ConvolverMch();

  /////////////////////////////////////////////////////////
  // Handle FIR generator changes

  void set_fir(int ch_name, const FIRGen *gen);
  const FIRGen *get_fir(int ch_name) const;
  void release_fir(int ch_name);

  void set_all_firs(const FIRGen *gen[CH_NAMES]);
  void get_all_firs(const FIRGen *gen[CH_NAMES]);
  void release_all_firs();

  /////////////////////////////////////////////////////////
  // SamplesFilter overrides

  virtual bool init();
  virtual void uninit();

  virtual bool process(Chunk2 &in, Chunk2 &out);
  virtual bool flush(Chunk2 &out);
  virtual void reset();
};

#endif
