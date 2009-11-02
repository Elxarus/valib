#include <math.h>
#include "linkwitz_riley.h"

IIRInstance *linkwitz_riley_proto(int sample_rate, int m)
{
  // m - linkwitz-riley filter order
  // n - butterworth filter order

  if (m&1) m++; // make m even

  int n = m / 2;
  int k = (n+1) / 2;
  int odd = n & 1;

  IIRInstance *iir = new IIRInstance(sample_rate, m);
  if (!iir) return 0;
  if (!iir->n)
  {
    delete iir;
    return 0;
  }

  for (int i = 0; i < k - odd; i++)
  {
    Biquad *biquad = iir->sections + i*2;
    biquad->a[0] = 1.0;
    biquad->a[1] = -2.0 * cos(double(2 * i + n + 1) / (2 * n) * M_PI);
    biquad->a[2] = 1.0;
    biquad->b[0] = 1.0;
    biquad->b[1] = 0;
    biquad->b[2] = 0;
    biquad[1] = biquad[0];
  }

  if (odd)
  {
    Biquad *biquad = iir->sections + (m - 1);
    biquad->a[0] = 1.0;
    biquad->a[1] = 2.0;
    biquad->a[2] = 1.0;
    biquad->b[0] = 1.0;
    biquad->b[1] = 0;
    biquad->b[2] = 0;
  }

  return iir;
}

IIRInstance *
IIRLinkwitzRiley::make(int sample_rate) const
{
  IIRInstance *iir = linkwitz_riley_proto(sample_rate, n);
  if (!iir) return 0;

  if (!is_lpf)
  {
    // s = 1/s substitution
    // swap a0 and a2, b0 and b2
    for (int i = 0; i < iir->n; i++)
    {
      double a0 = iir->sections[i].a[0];
      iir->sections[i].a[0] = iir->sections[i].a[2];
      iir->sections[i].a[2] = a0;

      double b0 = iir->sections[i].b[0];
      iir->sections[i].b[0] = iir->sections[i].b[2];
      iir->sections[i].b[2] = b0;
    }
  }
  double k = 1 / tan(M_PI * double(f) / sample_rate);
  iir->bilinear(k);


  return iir;
}
