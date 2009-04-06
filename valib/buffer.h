/*
  Buffer allocation
  * Rawdata   - raw data buffer
  * Samples   - array of samples
  * SampleBuf - multichannel sample buffer
*/

#ifndef VALIB_DATA_H
#define VALIB_DATA_H

#include "auto_buf.h"
#include "spk.h"

typedef AutoBuf<uint8_t> Rawdata;
typedef AutoBuf<sample_t> Samples;

class SampleBuf
{
protected:
  unsigned  f_nch;
  size_t    f_nsamples;
  samples_t f_samples;

  Samples   f_buf;

public:
  SampleBuf(): f_nch(0), f_nsamples(0)
  {}

  SampleBuf(unsigned nch, size_t nsamples): f_nch(0), f_nsamples(0)
  {
    allocate(nch, nsamples);
  }

  inline bool allocate(unsigned nch, size_t nsamples)
  {
    if (f_buf.size() < nch * nsamples)
    {
      drop();
      if (f_buf.allocate(nch * nsamples) == 0)
        return false;
    }

    f_nch = nch;
    f_nsamples = nsamples;
    f_samples.zero();
    for (unsigned ch = 0; ch < nch; ch++)
      f_samples[ch] = f_buf.data() + ch * nsamples;
    return true;
  }

  inline void drop()
  {
    f_nch = 0;
    f_nsamples = 0;
    f_samples.zero();
    f_buf.drop();
  }

  inline void zero()
  {
    assert(is_allocated());
    f_buf.zero();
  }

  inline unsigned  nch()      const { return f_nch;      }
  inline size_t    nsamples() const { return f_nsamples; }
  inline samples_t samples()  const { return f_samples;  }
  inline bool is_allocated()  const { return f_buf.is_allocated(); }

  inline operator samples_t() const { return f_samples; }
  inline sample_t *operator [](unsigned ch) const { return f_samples[ch]; }
};

#endif
