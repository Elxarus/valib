/*
  Infinite impulse response generator and instance classes
*/

#ifndef VALIB_IIR_H
#define VALIB_IIR_H

#include "defs.h"

class IIRGen;
class IIRInstance;
class Biquad;

class IIRZero;
class IIRIdentity;
class IIRGain;

class IIRFilter;

///////////////////////////////////////////////////////////////////////////////
// IIRGen - Infinite impulse response filter generator
//
// Interface for filter generator. It is used to generate a filter from a set
// of user-controllable parameters (if any) and a given sample rate.
//
// So IIRGen descendant may act as parameters container and these parameters
// may change, resulting in change of the filter. For class clients to notice
// these changes version is used. When version changes this means that we have
// to regenerate the filter.
//
// Sample rate may change during normal data flow and therefore we need to
// regenerate the filter for a new sample rate. Sample rate change does not
// change the version because it is not a contained parameter, but an external
// one.
//
// version()
//   Returns the impulse response version. If the response function changes,
//   the version must also change so users of the generator can be notified
//   about this change and rebuild the response. Constant responses like zero
//   or identity never change the version (may return zero or any other const).
//
// make()
//   Builds the filter instance for the sample rate given.
//
///////////////////////////////////////////////////////////////////////////////

class IIRGen
{
public:
  IIRGen() {}
  virtual ~IIRGen() {}

  virtual int version() const = 0;
  virtual IIRInstance *make(int sample_rate) const = 0;
};



///////////////////////////////////////////////////////////////////////////////
// Biquad - a basic building block of an IIR filter
//
// Biquad represents the following form:
//
//   b0 + b1*z^-1 + b2*z^-2
//   ----------------------
//   a0 + a1*z^-1 + a2*z^-2
//
// Default biquad value is identity: b0 = 1 and a0 = 1, other coeffitients are
// zero).
//
// bilinear(double k) 
//   bilinear transform: s = k * (1 - z^-1) / (1 + z^-1)
//
// normalize()
//   Normalize the biquad: b_i = b_i / a0; a_i = a_i / a0
//   So producing the following biquad:
//
//   (b0/a0) + (b1/a0)*z^-1 + (b2/a0)*z^-2
//   -------------------------------------
//      1 + (a1/a0)*z^-1 + (a2/a0)*z^-2
//
// apply_gain(double gain)
//   Gain the biquad: b_i = b_i * gain
//
// get_gain()
//   Returns the gain of the biquad: gain = b0/a0.
//
// is_null(), is_identity(), is_gain(), is_infinity()
//   Determine the special case:
//   * Null biquad is a biquad with b0 = 0.
//   * Gain biquad is the following biquad: b0 / a0, where a0 <> 0 and b0 <> 0.
//   * Identity biquad is when b0 = a0 <> 0 and other coefficients are zero.
//     It is the special case of gain biquad, so is_identity() => is_gain()
//   * Infinity biquad is a biquad with a0 = 0.
///////////////////////////////////////////////////////////////////////////////

class Biquad
{
public:
  double a[3];
  double b[3];

  Biquad()
  { 
    // default biquad is identity
    a[0] = 1.0; a[1] = 0; a[2] = 0;
    b[0] = 1.0; b[1] = 0; b[2] = 0;
  }

  Biquad(sample_t a0, sample_t a1, sample_t a2, sample_t b0, sample_t b1, sample_t b2)
  {
    a[0] = a0; a[1] = a1; a[2] = a2;
    b[0] = b0; b[1] = b1; b[2] = b2;
  }

  void bilinear(double k)
  {
    double a0, a1, a2, b0, b1, b2;
    a0 = a[0] + a[1] * k + a[2] * k * k;
    a1 = 2 * (a[0] - a[2] * k * k);
    a2 = a[0] - a[1] * k + a[2] * k * k;
    b0 = b[0] + b[1] * k + b[2] * k * k;
    b1 = 2 * (b[0] - b[2] * k * k);
    b2 = b[0] - b[1] * k + b[2] * k * k;

    a[0] = a0; a[1] = a1; a[2] = a2;
    b[0] = b0; b[1] = b1; b[2] = b2;
  }

  void normalize()
  {
    if (a[0] != 1 && a[0] != 0)
    {
      b[0] /= a[0]; b[1] /= a[0]; b[2] /= a[0];
      a[1] /= a[0]; a[2] /= a[0];
      a[0] = 1.0;
    }
  }

  void apply_gain(double gain)
  {
    b[0] *= gain; b[1] *= gain; b[2] *= gain;
  }

  double get_gain()  const { return a[0] == 0? 0: b[0] / a[0]; }

  bool is_null()     const { return b[0] == 0; }
  bool is_gain()     const { return b[0] != 0 && b[1] == 0 && b[2] == 0 && a[0] != 0 && a[1] == 0 && a[2] == 0; }
  bool is_identity() const { return is_gain() && b[0] == a[0]; }
  bool is_infinity() const { return a[0] == 0; }
};



///////////////////////////////////////////////////////////////////////////////
// IIFInstance - IIR filter instance. An array of biquad sections
//
// sample_rate
//   Sample rate the filter is designed for.
//
// gain
//   Gain factor for the filter. It is independent from biquads gain. So we can
//   apply gain to the filter without changing biquads. If the filter have no
//   biquads (N = 0) the filter is considered to be a simple gain filter.
//
// n
//   Number of sections of the filter. Zero means that filter is a simple gain
//   filter (only gain is applied).
//
// sections
//   Biquads array.
//
// apply_gain(double gain)
//   Gain the filter. It is applied only to the gain factor, (not the biquads).
//
// get_gain()
//   The total gain factor of the filter. Product of each biquad gain and the
//   filter's gain.
//
// is_null(), is_identity(), is_gain()
//   Determine the degenerated form.
//   Filter is null when any biquad is null, or the filter's gian is zero.
//   Filter is the gain filter when all of the biquads are gain biquads.
//   Filter is identity when all biquads are identity and the filter's gain is 1.
//   Filter is infinity when any biquad is infinity.
///////////////////////////////////////////////////////////////////////////////

class IIRInstance
{
public:
  int sample_rate;
  double gain;

  int n;
  Biquad *sections;

  IIRInstance(int sample_rate, int n, double gain = 1.0);
  ~IIRInstance();

  void bilinear(double k);
  void normalize();

  void apply_gain(double gain);
  double get_gain() const;

  bool is_null() const;
  bool is_identity() const;
  bool is_gain() const;
  bool is_infinity() const;
};



///////////////////////////////////////////////////////////////////////////////
// General IIR generators
///////////////////////////////////////////////////////////////////////////////

class IIRZero : public IIRGen
{
public:
  IIRZero() {}
  virtual IIRInstance *make(int sample_rate) const { return new IIRInstance(sample_rate, 0, 0); }
  virtual int version() const { return 0; }
};

class IIRIdentity : public IIRGen
{
public:
  IIRIdentity() {}
  virtual IIRInstance *make(int sample_rate) const { return new IIRInstance(sample_rate, 0, 1.0); }
  virtual int version() const { return 0; }
};

class IIRGain : public IIRGen
{
protected:
  int ver;
  double gain;

public:
  IIRGain(double _gain = 1.0): ver(0), gain(_gain) {}

  virtual IIRInstance *make(int sample_rate) const { return new IIRInstance(sample_rate, 0, gain); }
  virtual int version() const { return ver; }

  void set_gain(double _gain) { if (gain != _gain) gain = _gain, ver++; }
  double get_gain() const { return gain; }
};



///////////////////////////////////////////////////////////////////////////////
// IIRFilter
// Simple direct form II filter implementation
///////////////////////////////////////////////////////////////////////////////

class IIRFilter
{
protected:
  struct Section
  {
    // direct form 2
    sample_t a1, a2;
    sample_t b1, b2;
    sample_t h1, h2;
  };

  int n;
  sample_t gain;
  Section *sec;

public:
  IIRFilter();
  ~IIRFilter();

  bool init(const IIRInstance *iir);
  void uninit();

  void process(sample_t *samples, size_t nsamples);
  void reset();
};



///////////////////////////////////////////////////////////////////////////////
// Constant generators
//
// These generators do no have any parameters and never change. So we can
// make it global and use everywhere.
///////////////////////////////////////////////////////////////////////////////

extern IIRZero iir_zero;
extern IIRIdentity iir_identity;


#endif
