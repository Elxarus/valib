#include <math.h>
#include <string.h>
#include "param_fir.h"
#include "../dsp/kaiser.h"

inline double sinc(double x) { return x == 0 ? 1 : sin(x)/x; }
inline double lpf(int i, double f) { return 2 * f * sinc(i * 2 * M_PI * f); }

ParamFIR::ParamFIR():
ver(0), type(low_pass), f1(0.0), f2(0.0), df(0.0), a(0.0), norm(false)
{}

ParamFIR::ParamFIR(filter_t type_, double f1_, double f2_, double df_, double a_, bool norm_):
ver(0), type(type_), f1(f1_), f2(f2_), df(df_), a(a_), norm(norm_)
{}

void
ParamFIR::set(filter_t type_, double f1_, double f2_, double df_, double a_, bool norm_)
{
  ver++;
  type = type_;
  f1 = f1_;
  f2 = f2_;
  df = df_;
  a  = a_;
  norm = norm_;

  if (type == band_pass || type == band_stop)
    if (f1 > f2)
    {
      double temp = f1;
      f1 = f2; f2 = temp;
    }
}

void
ParamFIR::get(filter_t *type_, double *f1_, double *f2_, double *df_, double *a_, bool *norm_)
{
  if (type_) *type_ = type;
  if (f1_)   *f1_ = f1;
  if (f2_)   *f2_ = f2;
  if (df_)   *df_ = df;
  if (a_)    *a_  = a;
  if (norm_) *norm_ = norm;
}

int
ParamFIR::version() const
{ 
  return ver; 
}

const FIRInstance *
ParamFIR::make(int sample_rate) const
{
  int i;

  /////////////////////////////////////////////////////////////////////////////
  // Normalize

  double norm_factor = norm? 1.0: 1.0 / sample_rate;
  double f1_ = f1 * norm_factor;
  double f2_ = f2 * norm_factor;
  double df_ = df * norm_factor;

  /////////////////////////////////////////////////////////////////////////////
  // Trivial cases

  if (f1_ < 0.0 || f2_ < 0.0 || df_ <= 0.0 || a < 0.0) return 0;
  if (a == 0.0) return new IdentityFIRInstance(sample_rate);

  switch (type)
  {
    case low_pass:
      if (f1_ >= 0.5) return new IdentityFIRInstance(sample_rate);
      if (f1_ == 0.0) return new GainFIRInstance(sample_rate, db2value(a));
      break;

    case high_pass:
      if (f1_ >= 0.5) return new GainFIRInstance(sample_rate, db2value(a));
      if (f1_ == 0.0) return new IdentityFIRInstance(sample_rate);
      break;

    case band_pass:
      if (f1_ >= 0.5) return new GainFIRInstance(sample_rate, db2value(a));
      if (f2_ == 0.0) return new GainFIRInstance(sample_rate, db2value(a));
      if (f1_ == 0.0 && f2_ >= 0.5) return new IdentityFIRInstance(sample_rate);
      break;

    case band_stop:
      if (f1_ >= 0.5) return new IdentityFIRInstance(sample_rate);
      if (f2_ == 0.0) return new IdentityFIRInstance(sample_rate);
      if (f1_ == 0.0 && f2_ >= 0.5) return new GainFIRInstance(sample_rate, db2value(a));
      break;

    default:
      return 0;
  }

  /////////////////////////////////////////////////////////////////////////////
  // Build the filter

  int n = kaiser_n(a, df_) | 1; // make odd (type 1 filter)
  int c = n / 2;

  DynamicFIRInstance *fir = new DynamicFIRInstance(sample_rate, n, c);
  double *filter = fir->buf;

  filter[0] = 1.0;
  double alpha = kaiser_alpha(a);
  switch (type)
  {
    case low_pass:
      for (i = 0; i < n; i++)
        filter[i] = (sample_t) (2 * f1_ * sinc((i - c) * 2 * M_PI * f1_) * kaiser_window(i - c, n, alpha));
      return fir;

    case high_pass:
      for (i = 0; i < n; i++)
        filter[i] = (sample_t) (-2 * f1_ * sinc((i - c) * 2 * M_PI * f1_) * kaiser_window(i - c, n, alpha));
      filter[c] = (sample_t) ((1 - 2 * f1_) * kaiser_window(0, n, alpha));
      return fir;

    case band_pass:
      for (i = 0; i < n; i++)
        filter[i] = (sample_t) ((2 * f2_ * sinc((i - c) * 2 * M_PI * f2_) - 2 * f1_ * sinc((i - c) * 2 * M_PI * f1_)) * kaiser_window(i - c, n, alpha));
      return fir;

    case band_stop:
      for (i = 0; i < n; i++)
        filter[i] = (sample_t) ((2 * f1_ * sinc((i - c) * 2 * M_PI * f1_) - 2 * f2_ * sinc((i - c) * 2 * M_PI * f2_)) * kaiser_window(i - c, n, alpha));
      filter[c] = (sample_t) ((2 * f1_ + 1 - 2 * f2_) * kaiser_window(0, n, alpha));
      return fir;
  };

  // never be here
  assert(false);
  delete filter;
  return 0;
}
