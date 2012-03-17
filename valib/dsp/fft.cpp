#include <math.h>
#include "fft.h"
#include "fftsg.h"

FFT::FFT(): len(0)
{}

FFT::FFT(unsigned length)
{
  set_length(length);
}

void
FFT::set_length(unsigned length)
{
  if (len == length)
    return;

  len = 0;
  fft_ip.allocate((int)(2 + sqrt(double(length * 2))));
  fft_w.allocate(length/2+1);
  fft_ip[0] = 0;
  len = length;
}

void
FFT::rdft(sample_t *samples)
{
  assert(is_ok());
  ::rdft(len, 1, samples, fft_ip, fft_w);
}

void
FFT::inv_rdft(sample_t *samples)
{
  assert(is_ok());
  ::rdft(len, -1, samples, fft_ip, fft_w);
}
