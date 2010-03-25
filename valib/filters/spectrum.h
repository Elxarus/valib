/*
  Spectrum analysys filter
*/

#ifndef VALIB_SPECTRUM_H
#define VALIB_SPECTRUM_H

#include "../filter2.h"
#include "../buffer.h"
#include "../dsp/fft.h"

class Spectrum : public SamplesFilter
{
protected:
  unsigned  length;
  FFT       fft;

  SampleBuf data;
  Samples   spectrum;
  Samples   win;

  size_t pos;
  bool is_ok;
  bool init();

public:
  Spectrum();

  unsigned get_length() const;
  bool     set_length(unsigned length);
  void     get_spectrum(int ch, sample_t *data, double *bin2hz);

  /////////////////////////////////////////////////////////
  // SamplesFilter overrides

  virtual void reset();
  virtual bool process(Chunk2 &in, Chunk2 &out);
};

#endif
