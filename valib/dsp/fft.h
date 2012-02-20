/*
  Simple wrapper class for Ooura FFT

  FFT();
    Create an uninitialized FFT transform

  FFT(unsigned length);
    Create and initialize an FFT transform of length 'length'
    Can throw std::bad_alloc

  void set_length(unsigned length)
    Initialize the FFT transorm of length 'length'
    Can throw std::bad_alloc

  unsigned get_length() const
    Return the length of the FFT transform. Returns 0 when the transform was
    not initialized.

  bool is_ok() const
    Returns true when transform is initialized and false otherwise.
*/

#ifndef FFT_H
#define FFT_H

#include "../defs.h"
#include "../auto_buf.h"

class FFT
{
protected:
  AutoBuf<int> fft_ip;
  AutoBuf<sample_t> fft_w;
  unsigned len;

public:
  FFT();
  FFT(unsigned length);

  void set_length(unsigned length);
  unsigned get_length() const { return len; }
  bool is_ok() const { return len > 0; }

  void rdft(sample_t *samples);
  void inv_rdft(sample_t *samples);
};

#endif
