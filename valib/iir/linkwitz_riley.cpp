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

  IIRInstance *iir = new IIRInstance(sample_rate, n);
  if (!iir) return 0;
  if (!iir->n)
  {
    delete iir;
    return 0;
  }

  for (int i = 0; i < k - odd; i++)
    iir->sec[i*2] = iir->sec[i*2+1] = Biquad(
      1.0, -2.0 * cos(double(2 * i + n + 1) / (2 * n) * M_PI), 1.0,
      1.0, 0, 0);

  if (odd)
    iir->sec[n-1].set(1.0, 2.0, 1.0, 1.0, 0, 0);

  return iir;
}

IIRInstance *
IIRLinkwitzRiley::make(int sample_rate) const
{
  IIRInstance *iir = linkwitz_riley_proto(sample_rate, n);
  if (!iir) return 0;

  if (!is_lpf)
    for (int i = 0; i < iir->n; i++)
    {
      // s = 1/s substitution
      Biquad q = iir->sec[i];
      iir->sec[i] = Biquad(q.a[2], q.a[1], q.a[0], q.b[2], q.b[1], q.b[0]);
    }

  double k = 1 / tan(M_PI * double(f) / sample_rate);
  iir->bilinear(k);

  return iir;
}
