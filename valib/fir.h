/**************************************************************************//**
  \file fir.h
  \brief Finite impulse response generator and instance classes
******************************************************************************/

#ifndef VALIB_FIR_H
#define VALIB_FIR_H

#include "defs.h"
#include "auto_buf.h"

enum fir_t { firt_zero, firt_identity, firt_gain, firt_custom };

class FIRGen;
class FIRInstance;

class FIRZero;
class FIRIdentity;
class FIRGain;

/**************************************************************************//**
  \class FIRGen
  \brief Finite impulse response function generator

  Interface for impulse response generator. It is used to generate an impulse
  response from a set of user-controllable parameters (if any) and a given
  sample rate.

  So FIRGen descendant may act as parameters container and these parameters
  may change, resulting in change of the impulse response. For class clients
  to notice these changes version is used. When version changes this means
  that we have to regenerate the response.

  Sample rate may change during normal data flow and therefore we need to
  regenerate the response for a new sample rate. Sample rate change does not
  change the version because it is not a contained parameter, but an external
  one.

  Depending on user parameters impulse response may degenerate into identity,
  gain or even zero response. Obviously, these cases require less computation
  and may be implemented more effective.

  Several common generators available:
  \li FIRZero - Constant generator that always returns zero response.
  \li FIRIdentity - Constant generator that always returns identity response.
  \li FIRGain - Gain response generator.

  \fn int FIRGen::version() const
    \return Returns the response version.
  
    When the response function changes, the version must also change so
    clients of the generator can be notified about this change and rebuild
    the response. Constant generators like FIRZero or FIRIdentity never change
    the version (may return zero or any other constant).

  \fn const FIRInstance *FIRGen::make(int sample_rate) const
    \param sample_rate Sample rate to build the FIR for
    \return Returns the pointer to the instance built.

    Builds response function instance for the sample rate given.

******************************************************************************/

class FIRGen
{
public:
  FIRGen() {}
  virtual ~FIRGen() {}

  virtual int version() const = 0;
  virtual const FIRInstance *make(int sample_rate) const = 0;
};

/**************************************************************************//**
  \class FIRInstance
  \brief Impulse response function instance.

  Simple container for FIR data.

  FIR instance is a const object. We should never change an instance after it
  was built. When we need to modify the response, we should build a new
  instance.

  For this reason FIRGen always returns a const pointer, that we cannot modify.

  'data' is const pointer to allow an instance to contain statically allocated
  function (see StaticFIRInstance).

  Normally you should never create an instance directly. Only FIRGen
  descendants should make responses. However, generator should use one of the
  following classes:
  \li StaticFIRInstance when it needs to return a statically allocated response
  function.
  \li DynamicFIRInstance when response function has to be built dynamically.
  \li ZeroFIRInstance to return a zero response.
  \li IdentityFIRInstance to return an identity response.
  \li GainFIRInstance to return a gain response.

  \fn FIRInstance::FIRInstance(int sample_rate, int length, int center, const double *data = 0)
  \param sample_rate Sample rate this function was made for
  \param length      Length of the function
  \param center      Position of the center of the function
  \param data        Pointer to the function buffer

  Constructs an instance for the data given.

  Protected to prevent direct construction (only descendants can construct it).

  \fn FIRInstance::~FIRInstance()
  Virtual desctructor is nessesary because the class will be overloaded and
  destructed using FIRInstance pointer.

  \fn fir_t FIRInstance::type() const
  \return The type of the instance

  Determined the type of the instance based on instance data.

  \var FIRInstance::sample_rate
    Sample rate this instance was constructed for.

  \var FIRInstance::length
    Length of the function.

  \var FIRInstance::center
    Position of the center of the function.

  \var FIRInstance::data
    Pointer to the function buffer.

******************************************************************************/

class FIRInstance
{
protected:
  FIRInstance(int sample_rate_, int length_, int center_, const double *data_ = 0):
  sample_rate(sample_rate_), length(length_), center(center_), data(data_) {}

public:
  int sample_rate;
  int length;
  int center;
  const double *data;

  virtual ~FIRInstance() {}

  fir_t type() const
  {
    assert(length > 0);
    if (length > 1)     return firt_custom;
    if (data[0] == 0.0) return firt_zero;
    if (data[0] == 1.0) return firt_identity;
    return firt_gain;
  };
};

/**************************************************************************//**
  \class StaticFIRInstance
  \brief Instance for statically allocated response function.

  This class should be instantiated by a generator when it builds a response
  from a statically allocated data.

  \fn StaticFIRInstance::StaticFIRInstance(int sample_rate, int length, int center, const double *data = 0)
  \param sample_rate Sample rate this function was made for
  \param length      Length of the function
  \param center      Position of the center of the function
  \param data        Pointer to the function buffer

  Constructs an instance for the data given.
******************************************************************************/

class StaticFIRInstance : public FIRInstance
{
public:
  StaticFIRInstance(int sample_rate_, int length_, int center_, const double *data_ = 0):
  FIRInstance(sample_rate_, length_, center_, data_)
  {}
};

/**************************************************************************//**
  \class DynamicFIRInstance
  \brief Instance for dynamically allocated response function.

  This class should be instantiated by generator when it builds a response and
  needs memory for the resulting function. The memory is allocated dynamically
  and it is deleted on instance destruction.

  Memory is initialized with zeros.

  Generator has write access to the memory allocated through 'buf' pointer.
  However, when generator returns the result as const FIRInstance pointer,
  the client of the generator looses this right, so constant property of
  FIRInstance is preserved.

  \fn DynamicFIRInstance::DynamicFIRInstance(int sample_rate, int length, int center)
  \param sample_rate Sample rate this function was made for
  \param length      Length of the function
  \param center      Position of the center of the function

  Constructs an instance for the data given and allocates the memory for
  the response function.

******************************************************************************/

class DynamicFIRInstance : public FIRInstance
{
protected:
  AutoBuf<double> autobuf; ///< Buffer for the response function

public:
  double *buf; ///< Pointer to the memory allocated

  DynamicFIRInstance(int sample_rate_, int length_, int center_):
  FIRInstance(sample_rate_, length_, center_, 0), autobuf(length_)
  {
    autobuf.zero();
    data = buf = autobuf;
  }
};

///////////////////////////////////////////////////////////////////////////////
// General FIR instance classes
//
// Several response types may be generated by any generator. And because of its
// importance these types are made as special classes.
///////////////////////////////////////////////////////////////////////////////

/// \brief Zero impulse response
class ZeroFIRInstance : public FIRInstance
{ 
public:
  ZeroFIRInstance(int sample_rate);
};

/// \brief Identity impulse response
class IdentityFIRInstance : public FIRInstance
{
public:
  IdentityFIRInstance(int sample_rate);
};

/// \brief Gain impulse response
class GainFIRInstance : public FIRInstance
{
public:
  double gain; ///< FIR gain
  GainFIRInstance(int sample_rate, double gain);
};

///////////////////////////////////////////////////////////////////////////////
// General FIR generators
///////////////////////////////////////////////////////////////////////////////

/// \brief Generator that retruns zero FIR
class FIRZero : public FIRGen
{
public:
  FIRZero() {}
  virtual const FIRInstance *make(int sample_rate) const;
  virtual int version() const { return 0; }
};

/// \brief Generator that retruns identity FIR
class FIRIdentity : public FIRGen
{
public:
  FIRIdentity() {}
  virtual const FIRInstance *make(int sample_rate) const;
  virtual int version() const { return 0; }
};

/**
  \class FIRGain
  \brief Generator that retruns gain FIR

  \fn void FIRGain::set_gain(double gain)
  \param gain FIR gain
  Sets FIR gain.

  \fn double FIRGain::get_gain() const
  \return Returns FIR gain
*/

class FIRGain : public FIRGen
{
protected:
  int ver;      ///< Current version
  double gain;  ///< FIR gain

public:
  FIRGain();
  FIRGain(double gain);

  virtual const FIRInstance *make(int sample_rate) const;
  virtual int version() const { return ver; }

  void set_gain(double gain);
  double get_gain() const;
};


/******************************************************************************
  These generators do no have any parameters and never change. So we can
  make it global and use everywhere.
******************************************************************************/

extern FIRZero fir_zero;         ///< Zero FIR generator.
extern FIRIdentity fir_identity; ///< Identity FIR generator.

/**************************************************************************//**
  \class FIRRef
  \brief Generator reference

  Sometimes it is convinient to treat change of FIR generator like generator's
  version change, so we can handle this change in a generalized way.

  The generator the reference points to must live all the time the reference
  points to this gererator.

  \fn FIRRef::FIRRef()
    Constructs an reference that does not point to any generator (unassigned
    reference).

  \fn FIRRef::FIRRef(const FIRGen *fir)
    \param fir Generator the reference will point to
    Constructs an reference that does not point to generator 'fir'.

  \fn FIRRef::FIRRef(const FIRRef &ref)
    \param ref Object to copy from
    Constructs a copy of another reference.

  \fn FIRRef &FIRRef::operator =(const FIRRef &ref)
    \param ref Object to assign from
    Assign a generator from another reference.

  \fn FIRRef &FIRRef::operator =(const FIRGen *new_fir)
    \param new_fir New generator
    Assigns the new gereator 'fir' to the reference.

  \fn void FIRRef::set(const FIRGen *new_fir)
    \param new_fir New generator
    Assigns the new gereator 'fir' to the reference.

  \fn const FIRGen *FIRRef::get() const
    \return Returns the generator the reference points to.

  \fn void FIRRef::release()
    Forget the gererator the reference points to. Reference becomes unassigned.

******************************************************************************/


class FIRRef : public FIRGen
{
protected:
  mutable int ver;     ///< Current version
  mutable int fir_ver; ///< Version of the referenced generator
  const FIRGen *fir;   ///< Referenced generator

public:
  FIRRef(): ver(0), fir_ver(0), fir(0)
  {};

  FIRRef(const FIRGen *fir_): ver(0), fir_ver(0), fir(fir_)
  {
    fir_ver = fir? fir->version(): 0;
  };

  FIRRef(const FIRRef &ref): ver(0), fir_ver(0), fir(0)
  {
    fir = ref.fir;
    fir_ver = fir? fir->version(): 0;
  }

  FIRRef &operator =(const FIRRef &ref)
  {
    set(ref.fir);
    return *this;
  }

  FIRRef &operator =(const FIRGen *new_fir)
  {
    set(new_fir);
    return *this;
  }

  /////////////////////////////////////////////////////////
  // Handle generator changes

  void set(const FIRGen *new_fir)
  {
    if (fir == new_fir) return;

    fir = new_fir;
    fir_ver = fir? fir->version(): 0;
    ver++;
  }

  const FIRGen *get() const
  {
    return fir;
  }

  void release()
  {
    fir = 0;
    ver++;
  }

  /////////////////////////////////////////////////////////
  // FIRGen interface

  virtual int version() const
  {
    if (fir)
    { 
      int new_fir_ver = fir? fir->version(): 0;
      if (fir_ver != new_fir_ver)
      {
        fir_ver = new_fir_ver;
        ver++;
      }
    }
    return ver; 
  }

  virtual const FIRInstance *make(int sample_rate) const
  {
    return fir? fir->make(sample_rate): 0;
  }
};

#endif
