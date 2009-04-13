#include <math.h>
#include "eq_fir.h"
#include "../dsp/kaiser.h"

inline double sinc(double x) { return x == 0 ? 1 : sin(x)/x; }
inline double lpf(int i, double f) { return 2 * f * sinc(i * 2 * M_PI * f); }

static const double min_ripple = 0.001;
static const double max_ripple = 3.0;
static const double def_ripple = 0.1;
static const int max_length = 64*1024-1; // Max filter length is 64K

EqFIR::EqFIR(): ver(0), nbands(0), ripple(def_ripple)
{}

EqFIR::EqFIR(const EqBand *new_bands, size_t new_nbands): ver(0), nbands(0)
{
  set_bands(new_bands, new_nbands);
}

size_t
EqFIR::get_nbands() const
{
  return nbands;
}

size_t
EqFIR::set_bands(const EqBand *new_bands, size_t new_nbands)
{
  size_t i;

  reset();
  if (new_nbands == 0 || !new_bands)
    return 0;

  bands.allocate(new_nbands);
  if (!bands.is_allocated())
    return 0;

  nbands = 0;
  for (i = 0; i < new_nbands; i++)
    if (new_bands[i].freq > 0)
      bands[nbands++] = new_bands[i];

  for (i = 0; i < nbands; i++)
    if (bands[i].gain > 1e10)
      bands[i].gain = 1e10;

  // simple bubble sort
  if (nbands > 1)
  {
    bool sorted = false;
    while (!sorted)
    {
      sorted = true;
      for (i = 0; i < nbands-1; i++)
        if (bands[i].freq > bands[i+1].freq)
        {
          EqBand tmp = bands[i];
          bands[i] = bands[i+1];
          bands[i+1] = tmp;
          sorted = false;
        }
    }
  }

  ver++;
  return nbands;
}

size_t
EqFIR::get_bands(EqBand *out_bands, size_t first_band, size_t out_nbands) const
{
  if (!out_bands || !out_nbands || first_band >= nbands)
    return 0;

  if (first_band + out_nbands > nbands)
    out_nbands = nbands - first_band;

  for (size_t i = 0; i < out_nbands; i++)
    out_bands[i] = bands[first_band + i];

  return nbands;
}

double
EqFIR::get_ripple() const
{ return ripple; }

void
EqFIR::set_ripple(double new_ripple)
{
  new_ripple = fabs(new_ripple);
  if (new_ripple > max_ripple) new_ripple = max_ripple;
  if (new_ripple < min_ripple) new_ripple = min_ripple;

  if (ripple != new_ripple)
  {
    ripple = new_ripple;
    ver++;
  }
}

void
EqFIR::reset()
{
  if (nbands)
  {
    nbands = 0;
    ver++;
  }
}

int
EqFIR::version() const
{ return ver; }

const FIRInstance *
EqFIR::make(int sample_rate) const
{
  size_t i; int j;
  double q = db2value(ripple) - 1;

  /////////////////////////////////////////////////////////
  // Find the last meaningful band

  size_t max_band = 0;
  while (max_band < nbands && bands[max_band].freq <= sample_rate / 2)
    max_band++;

  /////////////////////////////////////////////////////////
  // Trivial cases

  if (max_band == 0)
    return new IdentityFIRInstance(sample_rate);
  else if (max_band == 1)
    return new GainFIRInstance(sample_rate, bands[0].gain);

  /////////////////////////////////////////////////////////
  // Find the filter length

  int max_n = 1;
  int max_c = 0;
  double max_a = 0;
  for (i = 0; i < max_band - 1; i++)
    if (bands[i+1].gain != bands[i].gain)
    {
      double dg = bands[i+1].gain - bands[i].gain;
      double df = double(bands[i+1].freq - bands[i].freq) / sample_rate;
      double a  = -value2db(q / fabs(dg));
      int n = kaiser_n(a, df) | 1;
      if (n > max_length) n = max_length;
      if (n > max_n) max_n = n, max_a = a;
    }
  max_c = max_n / 2;

  /////////////////////////////////////////////////////////
  // Build the filter
  // Change at one band does not affect other bands

  double *data = new double[max_n];
  if (!data)
    return 0;

  for (j = 0; j < max_n; j++) data[j] = 0;

  data[max_c] += bands[max_band-1].gain;
  for (i = 0; i < max_band - 1; i++)
    if (bands[i].gain != bands[i+1].gain)
    {
      double dg = bands[i].gain - bands[i+1].gain;
      double df = double(bands[i+1].freq - bands[i].freq) / sample_rate;
      double cf = double(bands[i+1].freq + bands[i].freq) / 2 / sample_rate;
      double a  = -value2db(q / fabs(dg));
      double alpha = kaiser_alpha(a);
      int n = kaiser_n(a, df) | 1;
      if (n > max_length) n = max_length;
      int c = n / 2;

      for (int j = -c; j <= c; j++)
        data[max_c + j] += dg * lpf(j, cf) * kaiser_window(j, n, alpha);
    }
/*
  /////////////////////////////////////////////////////////
  // Build the filter
  // Use variable transition bandwidth to minimize Gibbs
  // effect. To do this, adjust Kaiser window parameter
  // for each filter. Wider the band, lower the ripple.

  double *data = new double[max_n];
  if (!data)
    return 0;

  for (j = 0; j < max_n; j++) data[j] = 0;

  data[max_c] += bands[max_band-1].gain;
  for (i = 0; i < max_band - 1; i++)
    if (bands[i].gain != bands[i+1].gain)
    {
      double dg = bands[i].gain - bands[i+1].gain;
      double df = double(bands[i+1].freq - bands[i].freq) / sample_rate;
      double cf = double(bands[i+1].freq + bands[i].freq) / 2 / sample_rate;
      double alpha = kaiser_alpha(kaiser_a(max_n, df));
      for (int j = -max_c; j < max_c; j++)
        data[max_c + j] += dg * lpf(j, cf) * kaiser_window(j, max_n, alpha);
    }
*/
/*
  /////////////////////////////////////////////////////////
  // Build the filter
  // Constant transition band width, but only one window
  // calculation. Equal ripple at each band.

  double *data = new double[max_n];
  if (!data)
    return 0;

  for (j = 0; j < max_n; j++) data[j] = 0;

  data[max_c] += bands[max_band-1].gain;
  for (i = 0; i < max_band - 1; i++)
    if (bands[i].gain != bands[i+1].gain)
      for (int j = -max_c; j < max_c; j++)
        data[max_c + j] += (bands[i].gain - bands[i+1].gain) * lpf(j, double(bands[i+1].freq - bands[i].freq) / sample_rate);

  double alpha = kaiser_alpha(max_a);
  for (j = -max_c; j < max_c; j++)
    data[max_c + j] *= kaiser_window(j, max_n, alpha);
*/
  return new DynamicFIRInstance(sample_rate, firt_custom, max_n, max_c, data);
}
