#include <math.h>
#include "linkwitz_riley.h"

IIRInstance *linkwitz_riley_proto(int m)
{
  // m - linkwitz-riley filter order
  // n - butterworth filter order

  if (m&1) m++; // make m even

  int n = m / 2;
  int k = (n+1) / 2;
  int odd = n & 1;

  IIRInstance *iir = new IIRInstance(0);

  for (int i = 0; i < k - odd; i++)
  {
    Biquad biquad(
      1.0, -2.0 * cos(double(2 * i + n + 1) / (2 * n) * M_PI), 1.0,
      1.0, 0, 0);
    iir->sections.push_back(biquad);
    iir->sections.push_back(biquad);
  }

  if (odd)
    iir->sections.push_back(Biquad(1.0, 2.0, 1.0, 1.0, 0, 0));

  return iir;
}

IIRInstance *
IIRLinkwitzRiley::make(int sample_rate) const
{
  IIRInstance *iir = linkwitz_riley_proto(n);

  if (!is_lpf)
    for (size_t i = 0; i < iir->sections.size(); i++)
    {
      // s = 1/s substitution
      Biquad q = iir->sections[i];
      iir->sections[i].set(q.a[2], q.a[1], q.a[0], q.b[2], q.b[1], q.b[0]);
    }

  double k = 1 / tan(M_PI * double(f) / sample_rate);
  iir->bilinear(k);
  iir->sample_rate = sample_rate;

  return iir;
}
