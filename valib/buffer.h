/**************************************************************************//**
  \file buffer.h
  \brief Buffer classes
******************************************************************************/

#ifndef VALIB_BUFFER_H
#define VALIB_BUFFER_H

#include "auto_buf.h"
#include "spk.h"

/**************************************************************************//**
  \class Rawdata
  \brief Raw data buffer

  Specification of AutoBuf class.

  \class Samples
  \brief Buffer of samples

  Specification of AutoBuf class.

******************************************************************************/

typedef AutoBuf<uint8_t> Rawdata;
typedef AutoBuf<sample_t> Samples;

/**************************************************************************//**
  \class SampleBuf
  \brief Multichannel sample buffer

  This class is exception safe. But not strictly safe, because reallocate()
  does not preserve the content on exception.

  \fn SampleBuf::SampleBuf()
    Default constructor. Does not allocate the buffer.

    Can throw std::bad_alloc.

  \fn SampleBuf::SampleBuf(unsigned nch, size_t nsamples)
    \param nch      Number of channels to allocate
    \param nsamples Number of samples per channel

    Init constructor. Allocates the buffer for 'nch' channels, 'nsamples'
    samples each.

    Can throw std::bad_alloc.

  \fn void SampleBuf::allocate(unsigned nch, size_t nsamples)
    \param nch      Number of channels to allocate
    \param nsamples Number of samples per channel

    Allocate the buffer for 'nch' channels, 'nsamples' samples each. Preserves
    the data at the buffer if possible. When buffer grows, new space is
    initialized with zeros.

    Can throw std::bad_alloc.

  \fn void SampleBuf::reallocate(unsigned nch, size_t nsamples)
    \param nch      New number of channels
    \param nsamples New number of samples per channel

    Reallocate the buffer for 'nch' channels, 'nsamples' samples each.
    Preserves the previous content of the buffer.

    Can throw std::bad_alloc.

  \fn void SampleBuf::free()
    Release the memory allocated.

    Does not throw.
    
  \fn void SampleBuf::zero()
    Fill the memory allocated with zeros.

  \fn unsigned SampleBuf::nch() const
    \return Returns the number of channels allocated.

  \fn size_t SampleBuf::nsamples() const
    \return Returns the number of samples allocated for each channel.

  \fn samples_t SampleBuf::samples() const
    \return Returns sample_t structure that points inside the buffer.

    Explicit cast to samples_t.

  \fn bool SampleBuf::is_allocated() const
    \return Returns true when buffer is allocated and false otherwise.

  \fn SampleBuf::operator samples_t() const
    \return Returns sample_t structure that points inside the buffer.

    Automatic cast to samples_t

  \fn sample_t *SampleBuf::operator [](unsigned ch) const
    \param ch Channel to return
    \return Returns the pointer to the beginning of the channel 'ch'.
*/

class SampleBuf
{
protected:
  unsigned  f_nch;      ///< Number of channels
  size_t    f_nsamples; ///< Number of samples per channel
  samples_t f_samples;  ///< Channel buffers pointers

  Samples   f_buf;      ///< Data buffer

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
