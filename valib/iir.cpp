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
  sections = new Biquad[_n];
  n = sections? _n: 0;
  gain = _gain;
}

IIRInstance::~IIRInstance()
{
  safe_delete(sections);
}

void IIRInstance::bilinear(double k)
{
  for (int i = 0; i < n; i++)
    sections[i].bilinear(k);
}

void IIRInstance::normalize()
{
  for (int i = 0; i < n; i++)
    sections[i].normalize();
}

void IIRInstance::apply_gain(double _gain)
{
  gain *= _gain;
}

double IIRInstance::get_gain() const
{
  double total_gain = gain;
  for (int i = 0; i < n; i++)
    total_gain *= sections[i].get_gain();
  return total_gain;
}

bool IIRInstance::is_null() const
{
  if (gain == 0) return true;

  for (int i = 0; i < n; i++)
    if (sections[i].is_null())
      return true;
  return false;
}

bool IIRInstance::is_identity() const
{
  if (gain != 1.0) return false;

  for (int i = 0; i < n; i++)
    if (!sections[i].is_identity())
      return false;
  return true;
}

bool IIRInstance::is_gain() const
{
  for (int i = 0; i < n; i++)
    if (!sections[i].is_gain())
      return false;
  return true;
}

bool IIRInstance::is_infinity() const
{
  for (int i = 0; i < n; i++)
    if (sections[i].is_infinity())
      return true;
  return false;
}



///////////////////////////////////////////////////////////////////////////////
// IIRFilter

IIRFilter::IIRFilter(): n(0), gain(1.0), sec(0)
{}

IIRFilter::~IIRFilter()
{
  uninit();
}

bool
IIRFilter::init(const IIRInstance *iir)
{
  uninit();

  if (!iir)
    return true;

  if (iir->is_null())
  {
    gain = 0;
    return true;
  }
  if (iir->is_identity())
  {
    gain = 1.0;
    return true;
  }
  if (iir->is_gain())
  {
    gain = iir->get_gain();
    return true;
  }
  if (iir->is_infinity())
    return false;

  gain = iir->gain;
  if (iir->n)
  {
    sec = new Section[iir->n];
    if (!sec) return false;
    n = iir->n;

    for (int i = 0; i < n; i++)
    {
      Biquad *b = iir->sections + i;
      gain *= b->b[0] / b->a[0];
      sec[i].a1 = b->a[1] / b->a[0];
      sec[i].a2 = b->a[2] / b->a[0];
      sec[i].b1 = b->b[1] / b->b[0];
      sec[i].b2 = b->b[2] / b->b[0];
      sec[i].h1 = 0;
      sec[i].h2 = 0;
    }
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
