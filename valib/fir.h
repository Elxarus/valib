#ifndef VALIB_FIR_H
#define VALIB_FIR_H

#include "defs.h"

///////////////////////////////////////////////////////////////////////////////
// ImpulseResponse class
//
// Interface for impulse response generator. It is used to generate an impulse
// response from a set of user-controllable parameters (if any) and a given
// sample rate.
//
// So ImpulseResponse descendant may act as parameters container and these
// parameters may change, resulting in change of the impulse response. For
// class clients to notice these changes version is used. When version changes
// this means that we have to regenerate the response.
//
// Sample rate may change during normal data flow and therefore we need to
// regenerate the response for a new sample rate. Sample rate change does not
// change the version because it is not a contained parameter, but an external
// one.
//
// Depending on user parameters impulse response may degenerate into identity
// or even zero response. Obviously, these cases require no computation and
// may be implemented more effective. To indicate these states response type
// is used. Also, response type is used when user parameters are invalid and
// generation may fail (divide by zero). So min_length() and get_filter() must
// not be called in this case.
//
///////////////////////////////////////////////////////////////////////////////

enum ir_type { ir_err, ir_zero, ir_identity, ir_custom };

class ImpulseResponse
{
public:
  ImpulseResponse() {};
  virtual ~ImpulseResponse() {};

  virtual int     version() const = 0;
  virtual ir_type get_type(int sample_rate) const = 0;
  virtual int     min_length(int sample_rate) const = 0;
  virtual int     get_filter(int sample_rate, int n, sample_t *filter) const = 0;
};

class ZeroIR : public ImpulseResponse
{
public:
  ZeroIR() {};

  virtual int     version() const { return 0; }
  virtual ir_type get_type(int sample_rate) const { return ir_zero; }
  virtual int     min_length(int sample_rate) const { assert(false); return 0; } // should not be called
  virtual int     get_filter(int sample_rate, int n, sample_t *filter) const { assert(false); return 0; } // should not be called
};

class IdentityIR : public ImpulseResponse
{
public:
  IdentityIR() {};

  virtual int     version() const { return 0; }
  virtual ir_type get_type(int sample_rate) const { return ir_identity; }
  virtual int     min_length(int sample_rate) const { assert(false); return 0; } // should not be called
  virtual int     get_filter(int sample_rate, int n, sample_t *filter) const { assert(false); return 0; } // should not be called
};

class ImpulseResponseRef : public ImpulseResponse
{
protected:
  mutable int ver;
  mutable int ir_ver;
  const ImpulseResponse *ir;

public:
  ImpulseResponseRef(): ver(0), ir_ver(0), ir(0)
  {};

  ImpulseResponseRef(const ImpulseResponse *_ir): ver(0), ir_ver(0), ir(_ir)
  {
    if (_ir) 
      ir_ver = ir->version();
  };

  ImpulseResponseRef(const ImpulseResponseRef &_ref): ver(0), ir_ver(0), ir(0)
  {
    ir = _ref.ir;
    if (ir) 
      ir_ver = ir->version();
  }

  ImpulseResponseRef &operator =(const ImpulseResponseRef &_ref)
  {
    ir = _ref.ir;
    if (ir) 
      ir_ver = ir->version();
    ver++;
    return *this;
  }

  /////////////////////////////////////////////////////////
  // Handle impulse response changes

  void set(const ImpulseResponse *_ir)
  {
    ir = _ir;
    if (ir) 
      ir_ver = ir->version();
    ver++;
  }

  const ImpulseResponse *get() const
  {
    return ir;
  }

  void release()
  {
    ir = 0;
    ver++;
  }

  /////////////////////////////////////////////////////////
  // ImpulseResponse interface

  virtual int     version() const { if (ir) if (ir_ver != ir->version()) ver++, ir_ver = ir->version(); return ver; }
  virtual ir_type get_type(int sample_rate) const { return ir? ir->get_type(sample_rate): ir_err; }
  virtual int     min_length(int sample_rate) const { return ir? ir->min_length(sample_rate): 0; }
  virtual int     get_filter(int sample_rate, int n, sample_t *filter) const { return ir? ir->get_filter(sample_rate, n, filter): 0; }
};

#endif
