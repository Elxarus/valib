#include <math.h>
#include "eq_fir.h"
#include "../dsp/kaiser.h"

inline double sinc(double x) { return x == 0 ? 1 : sin(x)/x; }
inline double lpf(int i, double f) { return 2 * f * sinc(i * 2 * M_PI * f); }


EqFIR::EqFIR(): ver(0), nbands(0), freq(0), gain(0)
{}

EqFIR::EqFIR(size_t nbands_, int *freq_, double *gain_): ver(0), nbands(0), freq(0), gain(0)
{
  set_bands(nbands_, freq_, gain_);
}

EqFIR::~EqFIR()
{
  reset();
}

size_t
EqFIR::get_nbands() const
{ return nbands; }

bool
EqFIR::set_bands(size_t nbands_, const int *freq_, const double *gain_)
{
  size_t i;

  reset();

  if (nbands_ == 0)
    return true;

  if (!freq_ || !gain_)
    return false;

  freq = new int[nbands_];
  gain = new double[nbands_];
  if (!freq && !gain)
  {
    reset();
    return false;
  }

  nbands = 0;
  for (i = 0; i < nbands_; i++)
    if (freq_[i] > 0)
    {
      freq[nbands] = freq_[i];
      gain[nbands] = gain_[i];
      nbands++;
    }

  // simple bubble sort
  if (nbands > 1)
  {
    bool sorted = false;
    while (!sorted)
    {
      sorted = true;
      for (i = 0; i < nbands-1; i++)
        if (freq[i] > freq[i+1])
        {
          int f = freq[i];
          freq[i] = freq[i+1];
          freq[i+1] = f;
          double g = gain[i];
          gain[i] = gain[i+1];
          gain[i+1] = f;
          sorted = false;
        }
    }
  }

  ver++;
  return true;
}

void
EqFIR::get_bands(int *freq_, double *gain_) const
{
  size_t i;
  if (freq_) for (i = 0; i < nbands; i++) freq_[i] = freq[i];
  if (gain_) for (i = 0; i < nbands; i++) gain_[i] = gain[i];
}

void
EqFIR::reset()
{
  if (nbands)
  {
    nbands = 0;
    safe_delete(freq);
    safe_delete(gain);
    ver++;
  }
}

int
EqFIR::version() const
{ 
  return ver; 
}

const FIRInstance *
EqFIR::make(int sample_rate) const
{
  size_t i; int j;
  double q = db2value(0.5) - 1;

  /////////////////////////////////////////////////////////
  // Find the last meaningful band

  size_t max_band = 0;
  while (max_band < nbands && freq[max_band] <= sample_rate / 2)
    max_band++;

  /////////////////////////////////////////////////////////
  // Trivial cases

  if (max_band == 0)
    return new IdentityFIRInstance(sample_rate);
  else if (max_band == 1)
    return new GainFIRInstance(sample_rate, gain[0]);

  /////////////////////////////////////////////////////////
  // Find the filter length

  int max_n = 1;
  int max_c = 0;
  double max_a = 0;
  for (i = 0; i < max_band - 1; i++)
  {
    double dg = gain[i+1] - gain[i];
    if (fabs(dg) > 0)
    {
      double df = double(freq[i+1] - freq[i]) / sample_rate;
      double a  = -value2db(gain[i] * q / fabs(dg));
      int n = kaiser_n(a, df) | 1;
      if (n > max_n) max_n = n, max_a = a;
    }
  }
  max_c = max_n / 2;

  /////////////////////////////////////////////////////////
  // Build the filter
  // Use variable transition bandwidth to minimize Gibbs
  // effect. To do this, adjust Kaiser window parameter
  // for each filter. Wider the band, lower the ripple.

  double *data = new double[max_n];
  if (!data)
    return 0;

  for (j = 0; j < max_n; j++) data[j] = 0;

  data[max_c] += gain[max_band-1];
  for (i = 0; i < max_band - 1; i++)
    if (gain[i] != gain[i+1])
    {
      double df = double(freq[i+1] - freq[i]) / sample_rate;
      double cf = double(freq[i+1] + freq[i]) / 2 / sample_rate;
      double alpha = kaiser_alpha(kaiser_a(max_n, df));
      for (int j = -max_c; j < max_c; j++)
        data[max_c + j] += (gain[i] - gain[i+1]) * lpf(j, cf) * kaiser_window(j, max_n, alpha);
    }

/*
  /////////////////////////////////////////////////////////
  // Build the filter
  // Constant transition band width, but only one window
  // calculation. Equal ripple at each band.

  double *data = new double[max_n];
  if (!data)
    return 0;

  for (j = 0; j < max_n; j++) data[j] = 0;

  data[max_c] += gain[max_band-1];
  for (i = 0; i < max_band - 1; i++)
    if (gain[i] != gain[i+1])
      for (int j = -max_c; j < max_c; j++)
        data[max_c + j] += (gain[i] - gain[i+1]) * lpf(j, double(freq[i+1] - freq[i]) / sample_rate);

  double alpha = kaiser_alpha(max_a);
  for (j = -max_c; j < max_c; j++)
    data[max_c + j] *= kaiser_window(j, max_n, alpha);
*/

  return new DynamicFIRInstance(sample_rate, firt_custom, max_n, max_c, data);
}
