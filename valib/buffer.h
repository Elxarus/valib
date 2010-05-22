/*
  Buffer allocation
  * Rawdata   - raw data buffer
  * Samples   - array of samples
  * SampleBuf - multichannel sample buffer

  SampleBuf
  =========
  This class is exception safe. But not strictly safe, because reallocate()
  does not preserve the content on exception.


  SampleBuf()
    Default constructor. Does not allocate the buffer.
    Can throw std::bad_alloc.

  SampleBuf(unsigned nch, size_t nsamples)
    Init constructor. Allocates the buffer for 'nch' channels, 'nsamples'
    samples each. Can throw std::bad_alloc.

  void allocate(unsigned nch, size_t nsamples)
    Allocate the buffer for 'nch' channels, 'nsamples' samples each. Preserves
    the data at the buffer if possible. When buffer grows, new space is
    initialized with zeros. Can throw std::bad_alloc.

  void reallocate(unsigned nch, size_t nsamples)
    Reallocate the buffer for 'nch' channels, 'nsamples' samples each.
    Can throw std::bad_alloc.

  void free()
    Release the memory allocated.
    Does not throw.
    
  void zero()
    Fill the memory allocated with zeros.

  unsigned nch() const
    Returns the number of channels allocated.

  size_t nsamples() const
    Returns the number of samples allocated for each channel.

  samples_t samples() const
    Returns channel pointers.

  bool is_allocated() const
    Returns true when buffer is allocated.

  operator samples_t() const
    Automatic cast to samples_t

  sample_t *operator [](unsigned ch) const
    Returns the pointer to the channel 'ch'.
*/

#ifndef VALIB_BUFFER_H
#define VALIB_BUFFER_H

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

  inline void allocate(unsigned nch, size_t nsamples)
  {
    // f_buf is exception-safe, so just try to allocate
    f_buf.allocate(nch * nsamples);

    f_nch = nch;
    f_nsamples = nsamples;
    f_samples.zero();
    for (unsigned ch = 0; ch < nch; ch++)
      f_samples[ch] = f_buf.begin() + ch * nsamples;
  }

  inline void reallocate(unsigned nch, size_t nsamples)
  {
    unsigned ch;
    unsigned min_nch = MIN(f_nch, nch);

    // Compact data before reallocation
    if (min_nch > 1 && nsamples < f_nsamples)
      for (ch = 1; ch < min_nch; ch++)
        move_samples(f_buf, ch * nsamples, f_buf, ch * f_nsamples, nsamples);

    // Reallocate
    f_buf.reallocate(nch * nsamples);

    // Expand data after reallocation
    // Zero the tail
    if (min_nch > 1 && nsamples > f_nsamples)
    {
      for (ch = min_nch - 1; ch > 0; ch--)
      {
        move_samples(f_buf, ch * nsamples, f_buf, ch * f_nsamples, f_nsamples);
        zero_samples(f_buf, ch * nsamples + f_nsamples, nsamples - f_nsamples);
      }
      zero_samples(f_buf, f_nsamples, nsamples - f_nsamples);
    }

    // Zero new channels
    if (nch > f_nch)
      zero_samples(f_buf, f_nch * nsamples, (nch - f_nch) * nsamples);

    // Update state
    f_nch = nch;
    f_nsamples = nsamples;
    for (ch = 0; ch < nch; ch++)
      f_samples[ch] = f_buf.begin() + ch * nsamples;
  }

  inline void free()
  {
    f_nch = 0;
    f_nsamples = 0;
    f_samples.zero();
    f_buf.free();
  }

  inline void zero()
  {
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
