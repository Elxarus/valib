#include <math.h>
#include "crossover.h"

IIRInstance *
IIRCrossover::lowpass_proto(int m)
{
  // m - linkwitz-riley filter order
  // n - butterworth filter order
  int n = (m+1) / 2;
  int k = n / 2;
  int odd = n & 1;

  IIRInstance *iir = new IIRInstance(0);

  for (int i = 0; i < k; i++)
  {
    Biquad biquad(
      1.0, 2.0 * sin(double(2 * i + 1) / (2 * n) * M_PI), 1.0,
      -1.0, 0, 0);
    iir->sections.push_back(biquad);
    iir->sections.push_back(biquad);
  }

  if (odd)
    iir->sections.push_back(Biquad(1.0, 2.0, 1.0, -1.0, 0, 0));

  return iir;
}

IIRInstance *
IIRCrossover::highpass_proto(int m)
{
  // m - linkwitz-riley filter order
  // n - butterworth filter order
  int n = (m+1) / 2;
  int k = n / 2;
  int odd = n & 1;

  IIRInstance *iir = new IIRInstance(0);

  for (int i = 0; i < k; i++)
  {
    Biquad biquad(
      1.0, 2.0 * sin(double(2 * i + 1) / (2 * n) * M_PI), 1.0,
      0, 0, 1.0);
    iir->sections.push_back(biquad);
    iir->sections.push_back(biquad);
  }

  if (odd)
    iir->sections.push_back(Biquad(1.0, 2.0, 1.0, 0, 0, 1.0));

  return iir;
}

IIRInstance *
IIRCrossover::allpass_proto(int m)
{
  int n = (m+1) / 2;
  int k = n / 2;
  int odd = n & 1;

  IIRInstance *iir = new IIRInstance(0);

  for (int i = 0; i < k; i++)
  {
    Biquad biquad(
      1.0, 2.0 * sin(double(2 * i + 1) / (2 * n) * M_PI), 1.0,
      1.0, -2.0 * sin(double(2 * i + 1) / (2 * n) * M_PI), 1.0);
    iir->sections.push_back(biquad);
  }

  if (odd)
    iir->sections.push_back(Biquad(1.0, 1.0, 0, -1.0, 1.0, 0));

  return iir;
}

IIRInstance *
IIRCrossover::make(int sample_rate) const
{
  IIRInstance *iir = 0;
  switch (t)
  {
    case highpass: iir = highpass_proto(n); break;
    case allpass:  iir = allpass_proto(n); break;
    default:       iir = lowpass_proto(n); break;
  }
   
  double k = 1 / tan(M_PI * double(f) / sample_rate);
  iir->bilinear(k);
  iir->sample_rate = sample_rate;
  return iir;
}
