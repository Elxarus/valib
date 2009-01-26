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
    drop();
    if (f_buf.allocate(nch * nsamples))
    {
      f_nch = nch;
      f_nsamples = nsamples;
      for (unsigned ch = 0; ch < nch; ch++)
        f_samples[ch] = f_buf.data() + ch * nsamples;
      return true;
    }
    return false;
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
    f_buf.zero();
  }

  inline unsigned  nch()      const { return f_nch;      }
  inline size_t    nsamples() const { return f_nsamples; }
  inline samples_t samples()  const { return f_samples;  }

  inline operator samples_t() const { return f_samples; }
  inline sample_t *operator [](unsigned ch) const { return f_samples[ch]; }
};

#endif
