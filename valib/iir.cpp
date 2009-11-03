#include "iir.h"

///////////////////////////////////////////////////////////////////////////////
// Constant generators

IIRZero iir_zero;
IIRIdentity iir_identity;



///////////////////////////////////////////////////////////////////////////////
// IIRInstance

IIRInstance::IIRInstance(int _sample_rate, int _n, double _gain)
{
  sample_rate = _sample_rate;
  sec = new Biquad[_n];
  n = sec? _n: 0;
  gain = _gain;
}

IIRInstance::~IIRInstance()
{
  safe_delete(sec);
}

void IIRInstance::bilinear(double k)
{
  for (int i = 0; i < n; i++)
    sec[i].bilinear(k);
}

void IIRInstance::normalize()
{
  for (int i = 0; i < n; i++)
    sec[i].normalize();
}

void IIRInstance::apply_gain(double _gain)
{
  gain *= _gain;
}

double IIRInstance::get_gain() const
{
  double total_gain = gain;
  for (int i = 0; i < n; i++)
    total_gain *= sec[i].get_gain();
  return total_gain;
}

bool IIRInstance::is_null() const
{
  if (gain == 0) return true;

  for (int i = 0; i < n; i++)
    if (sec[i].is_null())
      return true;
  return false;
}

bool IIRInstance::is_identity() const
{
  return is_gain() && get_gain() == 1.0;
}

bool IIRInstance::is_gain() const
{
  for (int i = 0; i < n; i++)
    if (!sec[i].is_gain())
      return false;
  return true;
}

bool IIRInstance::is_infinity() const
{
  for (int i = 0; i < n; i++)
    if (sec[i].is_infinity())
      return true;
  return false;
}



///////////////////////////////////////////////////////////////////////////////
// IIRFilter

IIRFilter::IIRFilter(): n(0), gain(1.0), sec(0)
{}

IIRFilter::IIRFilter(const IIRInstance *iir): n(0), gain(1.0), sec(0)
{
  init(iir);
}

IIRFilter::~IIRFilter()
{
  uninit();
}

bool
IIRFilter::init(const IIRInstance *iir)
{
  int i, j;

  uninit();

  /////////////////////////////////////////////////////////
  // Special cases

  if (!iir)
    return true;

  if (iir->is_infinity())
    return false;

  if (iir->is_null())
  {
    gain = 0;
    return true;
  }

  /////////////////////////////////////////////////////////
  // Global gain equals to the filter gain
  // Note, that we will normalize each section later

  gain = iir->get_gain();

  /////////////////////////////////////////////////////////
  // Count non-trivial sections

  n = 0;
  for (i = 0; i < iir->n; i++)
    if (!iir->sec[i].is_gain())
      n++;

  if (n == 0)
    // the filter is triuvial
    return true;

  /////////////////////////////////////////////////////////
  // Build sections array

  sec = new Section[n];
  if (!sec)
  {
    n = 0;
    return false;
  }

  for (i = 0, j = 0; i < iir->n; i++)
    if (!iir->sec[i].is_gain())
    {
      Biquad b = iir->sec[i];
      sec[j].a1 = b.a[1] / b.a[0];
      sec[j].a2 = b.a[2] / b.a[0];
      sec[j].b1 = b.b[1] / b.b[0];
      sec[j].b2 = b.b[2] / b.b[0];
      sec[j].h1 = 0;
      sec[j].h2 = 0;
      j++;
    }

  return true;
}

void
IIRFilter::uninit()
{
  n = 0;
  gain = 1.0;
  safe_delete(sec);
}

void
IIRFilter::process(sample_t *samples, size_t nsamples)
{
  Section *s;
  sample_t g = gain;
  sample_t y, h;

  /////////////////////////////////////
  // Trivial cases

  if (n == 0)
  {
    if (gain == 1.0)
    {
      // Do nothing
    }
    else if (gain == 0)
    {
      // Zero samples
      while (nsamples--)
        *samples++ = 0;
    }
    else
    {
      // Gain samples
      while (nsamples--)
        *samples++ *= g;
    }
    return;
  }

  /////////////////////////////////////
  // Apply direct form 2 cascade

  while (nsamples--)
  {
    y = g * *samples;
    for (s = sec; s < sec+n; s++)
    {
      h = y - s->a1 * s->h1 - s->a2 * s->h2;
      y = h + s->b1 * s->h1 + s->b2 * s->h2;
      s->h2 = s->h1;
      s->h1 = h;
    }
    *samples++ = y;
  }
}

void
IIRFilter::reset()
{
  for (int i = 0; i < n; i++)
  {
    sec[i].h1 = 0;
    sec[i].h2 = 0;
  }
}
