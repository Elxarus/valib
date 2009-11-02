#include <math.h>
#include "butterworth.h"

IIRInstance *butterworth_proto(int sample_rate, int n)
{
  int k = (n+1) / 2;
  int odd = n & 1;

  IIRInstance *iir = new IIRInstance(sample_rate, k);
  if (!iir) return 0;
  if (!iir->n)
  {
    delete iir;
    return 0;
  }

  for (int i = 0; i < k - odd; i++)
  {
    Biquad *biquad = iir->sections + i;
    biquad->a[0] = 1.0;
    biquad->a[1] = -2.0 * cos(double(2 * i + n + 1) / (2 * n) * M_PI);
    biquad->a[2] = 1.0;
    biquad->b[0] = 1.0;
    biquad->b[1] = 0;
    biquad->b[2] = 0;
  }

  if (odd)
  {
    Biquad *biquad = iir->sections + (k - 1);
    biquad->a[0] = 1.0;
    biquad->a[1] = 1.0;
    biquad->a[2] = 0;
    biquad->b[0] = 1.0;
    biquad->b[1] = 0;
    biquad->b[2] = 0;
  }

  return iir;
}

IIRInstance *
IIRButterworth::make(int sample_rate) const
{
  IIRInstance *iir = butterworth_proto(sample_rate, n);
  if (!iir) return 0;

  double k = 1/tan(M_PI * double(f) / sample_rate);
  iir->bilinear(k);

  return iir;
}
