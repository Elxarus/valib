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
    iir->sec[i] = Biquad(
      1.0, -2.0 * cos(double(2 * i + n + 1) / (2 * n) * M_PI), 1.0,
      1.0, 0, 0);

  if (odd)
    iir->sec[k-1].set(1.0, 1.0, 0, 1.0, 0, 0);

  return iir;
}

IIRInstance *
IIRButterworth::make(int sample_rate) const
{
  IIRInstance *iir = butterworth_proto(sample_rate, n);
  if (!iir) return 0;

  if (!is_lpf)
    for (int i = 0; i < iir->n; i++)
    {
      // s = 1/s substitution
      Biquad q = iir->sec[i];
      iir->sec[i] = Biquad(q.a[2], q.a[1], q.a[0], q.b[2], q.b[1], q.b[0]);
    }

  double k = 1/tan(M_PI * double(f) / sample_rate);
  iir->bilinear(k);

  return iir;
}
