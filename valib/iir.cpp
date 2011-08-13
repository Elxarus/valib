#include "iir.h"

///////////////////////////////////////////////////////////////////////////////
// Constant generators

IIRZero iir_zero;
IIRIdentity iir_identity;



///////////////////////////////////////////////////////////////////////////////
// IIRInstance

IIRInstance::IIRInstance(int sample_rate_, double gain_):
sample_rate(sample_rate_), gain(gain_)
{}

void IIRInstance::bilinear(double k)
{
  for (size_t i = 0; i < sections.size(); i++)
    sections[i].bilinear(k);
}

void IIRInstance::normalize()
{
  for (size_t i = 0; i < sections.size(); i++)
    sections[i].normalize();
}

void IIRInstance::apply_gain(double _gain)
{
  gain *= _gain;
}

double IIRInstance::get_gain() const
{
  double total_gain = gain;
  for (size_t i = 0; i < sections.size(); i++)
    total_gain *= sections[i].get_gain();
  return total_gain;
}

bool IIRInstance::is_null() const
{
  if (gain == 0) return true;

  for (size_t i = 0; i < sections.size(); i++)
    if (sections[i].is_null())
      return true;
  return false;
}

bool IIRInstance::is_identity() const
{
  return is_gain() && get_gain() == 1.0;
}

bool IIRInstance::is_gain() const
{
  for (size_t i = 0; i < sections.size(); i++)
    if (!sections[i].is_gain())
      return false;
  return true;
}

bool IIRInstance::is_infinity() const
{
  for (size_t i = 0; i < sections.size(); i++)
    if (sections[i].is_infinity())
      return true;
  return false;
}



///////////////////////////////////////////////////////////////////////////////
// IIRFilter

IIRFilter::IIRFilter(): gain(1.0)
{}

IIRFilter::IIRFilter(const IIRInstance *iir): gain(1.0)
{
  init(iir);
}

bool
IIRFilter::init(const IIRInstance *iir)
{
  drop();

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
  // Normalize and copy non-trivial sections

  for (size_t i = 0; i < iir->sections.size(); i++)
    if (!iir->sections[i].is_gain())
    {
      Section s;
      Biquad b = iir->sections[i];
      s.a1 = b.a[1] / b.a[0];
      s.a2 = b.a[2] / b.a[0];
      s.b1 = b.b[1] / b.b[0];
      s.b2 = b.b[2] / b.b[0];
      s.h1 = 0;
      s.h2 = 0;

      sections.push_back(s);
    }

  return true;
}

void
IIRFilter::drop()
{
  gain = 1.0;
  sections.clear();
}

void
IIRFilter::process(sample_t *samples, size_t nsamples)
{
  sample_t g = gain;

  /////////////////////////////////////
  // Trivial cases

  if (sections.empty())
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

  Section *s;
  Section *begin = &sections[0];
  Section *end   = begin + sections.size();
  sample_t y, h;
  while (nsamples--)
  {
    y = g * *samples;
    for (s = begin; s < end; s++)
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
  for (size_t i = 0; i < sections.size(); i++)
  {
    sections[i].h1 = 0;
    sections[i].h2 = 0;
  }
}
