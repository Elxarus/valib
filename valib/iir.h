/**************************************************************************//**
  \file iir.h
  \brief Infinite impulse response generator and instance classes
******************************************************************************/

#ifndef VALIB_IIR_H
#define VALIB_IIR_H

#include <vector>
#include "defs.h"

class IIRGen;
class IIRInstance;
class Biquad;

class IIRZero;
class IIRIdentity;
class IIRGain;

class IIRFilter;

/**************************************************************************//**
  \class IIRGen
  \brief Infinite impulse response filter generator

  Interface for infinite impulse responce generator. It is used to generate a
  filter from a set of user-controllable parameters (if any) and a given
  sample rate.

  So IIRGen descendant may act as parameters container and these parameters
  may change, resulting in change of the filter. For class clients to notice
  these changes version is used. When version changes this means that we have
  to regenerate the filter.

  Sample rate may change during normal data flow and therefore we need to
  regenerate the filter for a new sample rate. Sample rate change does not
  change the version because it is not a contained parameter, but an external
  one.

  Several common generators available:
  \li IIRZero - Constant generator that always returns zero response.
  \li IIRIdentity - Constant generator that always returns identity response.
  \li IIRGain - Gain response generator.

  \fn int IIRGen::version() const
    \return Returns the response version.
  
    When the response function changes, the version must also change so
    clients of the generator can be notified about this change and rebuild
    the response. Constant generators like FIRZero or FIRIdentity never change
    the version (may return zero or any other constant).

  \fn IIRInstance *IIRGen::make(int sample_rate) const
    \param sample_rate Sample rate to build the FIR for
    \return Returns the pointer to the instance built.

    Builds a filter instance for the sample rate given. Class client is
    responsible for instance deletition.

******************************************************************************/

class IIRGen
{
public:
  IIRGen() {}
  virtual ~IIRGen() {}

  virtual int version() const = 0;
  virtual IIRInstance *make(int sample_rate) const = 0;
};



/**************************************************************************//**
  \class Biquad
  \brief A basic building block of an IIR filter

  Biquad represents the following form:
  \verbatim
  b0 + b1*z^-1 + b2*z^-2
  ----------------------
  a0 + a1*z^-1 + a2*z^-2
  \endverbatim

  \fn Biquad::Biquad()
    Constructs an identity biquad where b0 = 1, a0 = 1 and other coeffitients are
    zero.

  \fn Biquad::Biquad(double gain)
    \param gain Gain for the biquad

    Constructs a gain biquad where b0 = gain, a0 = 1 and other coeffitients are
    zero.

  \fn Biquad::Biquad(sample_t a0, sample_t a1, sample_t a2, sample_t b0, sample_t b1, sample_t b2)
    Constructs a biquad with the coeffitients given.

  \fn void Biquad::set(sample_t a0, sample_t a1, sample_t a2, sample_t b0, sample_t b1, sample_t b2)
    Directly sets biquad coeffitients.

  \fn void Biquad::bilinear(double k)
    \param k bilinear transform coefficient

    Applies the bilinear transform to the biquad: s = k * (1 - z^-1) / (1 + z^-1)

  \fn void Biquad::normalize()
    Normalizes the biquad: b_i = b_i / a0; a_i = a_i / a0

    So producing the following biquad:
    \verbatim
    (b0/a0) + (b1/a0)*z^-1 + (b2/a0)*z^-2
    -------------------------------------
       1 + (a1/a0)*z^-1 + (a2/a0)*z^-2
    \endverbatim

  \fn void Biquad::apply_gain(double gain)
    \param gain Gain to be applied

    Applies gain to the biquad: b_i = b_i * gain

  \fn double Biquad::get_gain() const
    Returns the gain of the biquad: gain = b0/a0.

  \fn bool Biquad::is_null() const
    Returns true when the biquad ia a null biquad (b0 == 0).

  \fn bool Biquad::is_gain() const
    Returns true when the biquad is a gain biquad.

    Gain biquad is the following biquad: b0 / a0, where a0 <> 0 and other
    coefficients are 0. Null and identity are special cases of the gain biquad.

  \fn bool Biquad::is_identity() const
    Returns true when the biquad is an identity biquad.

    Identity biquad has b0 = a0, a0 <> 0 and other coefficients are 0.

  \fn bool Biquad::is_infinity() const
    Returns true when the biquad is an infinity biquad (a0 == 0).

******************************************************************************/

class Biquad
{
public:
  double a[3]; ///< Denominator coefficients
  double b[3]; ///< Numerator coefficients

  Biquad()
  { 
    // default biquad is identity
    set(1.0, 0, 0, 1.0, 0, 0);
  }

  Biquad(double gain)
  { 
    set(1.0, 0, 0, gain, 0, 0);
  }

  Biquad(sample_t a0, sample_t a1, sample_t a2, sample_t b0, sample_t b1, sample_t b2)
  {
    set(a0, a1, a2, b0, b1, b2);
  }

  void set(sample_t a0, sample_t a1, sample_t a2, sample_t b0, sample_t b1, sample_t b2)
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
    set(a0, a1, a2, b0, b1, b2);
  }

  void normalize()
  {
    if (a[0] != 1.0 && a[0] != 0)
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
  bool is_gain()     const { return b[1] == 0 && b[2] == 0 && a[0] != 0 && a[1] == 0 && a[2] == 0; }
  bool is_identity() const { return is_gain() && b[0] == a[0]; }
  bool is_infinity() const { return a[0] == 0; }
};



/**************************************************************************//**
  \class IIRInstance
  \brief IIR filter instance. An array of biquad sections.

  IIR filter consists of zero or more biquad sections and a global gain,
  applied to the whole filter.

  \var IIRInstance::sample_rate
    Sample rate the filter is designed for. When sample rate is 0 it means
    that filter is a prototype filter.

  \var IIRInstance::gain
    Gain factor for the filter. It is independent from biquads' gain. So we
    can apply gain to the filter without changing sections. If the filter
    have no biquads (N = 0) the filter is considered to be a simple gain
    filter.

  \var IIRInstance::sections
    Array of biquad sections.

  \fn IIRInstance::IIRInstance(int sample_rate, double gain = 1.0)
    \param sample_rate Sample rate an instance is built for
    \param gain Gain for this filter.

    Construct an instance for the parameters given.

  \fn void IIRInstance::bilinear(double k)
    \param k Bilinear transform coeffitient

    Applies bilinear transform to all biquad sections (see Biquad::bilinear()).

  \fn void IIRInstance::normalize()
    Normalizes all biquad sections (see Biquad::normalize()).

  \fn void IIRInstance::apply_gain(double gain)
    \param gain Gain to be applied

    Applies the gain to the filter. Note, that biquad sections remain
    unchanged, only IIRInstance::gain changes.

  \fn double IIRInstance::get_gain() const
    Calculates the gain of the filter. It consists of the global filter gain,
    multiplied by each section's gain (see Biquad::get_gain()).

  \fn bool IIRInstance::is_null() const
    Returns true when the filter is a null filter, i.e. turns any signal into
    null function.

  \fn bool IIRInstance::is_identity() const
    Returns true when the filter is an identity filter, i.e. does not change
    the signal at all.

  \fn bool IIRInstance::is_gain() const
    Returns true when the filter is a gain filter, i.e. just gains input
    signal.

  \fn bool IIRInstance::is_infinity() const
    Returns true when the filter contains infinity biquads and cannot be
    implemented.

******************************************************************************/

class IIRInstance
{
public:
  int sample_rate;
  double gain;
  std::vector<Biquad> sections;

  IIRInstance(int sample_rate, double gain = 1.0);

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
//
// Several filter types may be generated by any generator. And because of its
// importance these types are made as special classes.
///////////////////////////////////////////////////////////////////////////////

/// \brief Zero filter generator
class IIRZero : public IIRGen
{
public:
  IIRZero() {}
  virtual IIRInstance *make(int sample_rate) const { return new IIRInstance(sample_rate, 0); }
  virtual int version() const { return 0; }
};

/// \brief Identity filter generator
class IIRIdentity : public IIRGen
{
public:
  IIRIdentity() {}
  virtual IIRInstance *make(int sample_rate) const { return new IIRInstance(sample_rate, 1.0); }
  virtual int version() const { return 0; }
};

/**
  \class IIRGain
  \brief Generator that retruns gain filter

  \fn void IIRGain::set_gain(double gain)
  \param gain FIR gain
  Sets the filter gain.

  \fn double IIRGain::get_gain() const
  \return Returns the filter gain
*/

class IIRGain : public IIRGen
{
protected:
  int ver;     ///< Current version
  double gain; ///< Filter gain

public:
  IIRGain(double gain_ = 1.0): ver(0), gain(gain_)
  {}

  void set_gain(double _gain)
  {
    if (gain != _gain)
      gain = _gain, ver++;
  }

  double get_gain() const
  { return gain; }

  virtual IIRInstance *make(int sample_rate) const
  { return new IIRInstance(sample_rate, gain); }

  virtual int version() const
  { return ver; }
};

/**************************************************************************//**
  \class IIRFilter
  \brief Simple direct form II filter implementation

  \fn IIRFilter::IIRFilter()
    Constructs a default filter that does not change the input signel.

  \fn IIRFilter::IIRFilter(const IIRInstance *iir)
    \param iir Infinite impulse response

    Constructs and initializes the filter with the given response.

  \fn bool IIRFilter::init(const IIRInstance *iir)
    \param iir Infinite impulse response

    Initializes the filter with the given response.

  \fn void IIRFilter::drop()
    Drops the filter. Equivalent to initialization with an identity filter.

  \fn void IIRFilter::process(sample_t *samples, size_t nsamples)
    \param samples Data pointer
    \param nsamples Number of samples to process

    Process the data.

  \fn void IIRFilter::reset()
    Reset the internal processing state.

******************************************************************************/

class IIRFilter
{
protected:
  //! Direct form 2 IIR filter section
  struct Section
  {
    sample_t a1, a2;
    sample_t b1, b2;
    sample_t h1, h2;
  };

  sample_t gain;                 ///< Global gain
  std::vector<Section> sections; ///< Filter sections

public:
  IIRFilter();
  IIRFilter(const IIRInstance *iir);

  bool init(const IIRInstance *iir);
  void drop();

  void process(sample_t *samples, size_t nsamples);
  void reset();
};



/******************************************************************************
  These generators do no have any parameters and never change. So we can
  make it global and use everywhere.
******************************************************************************/

extern IIRZero iir_zero;         ///< Zero IIR generator
extern IIRIdentity iir_identity; ///< Identity IIR generator


#endif
