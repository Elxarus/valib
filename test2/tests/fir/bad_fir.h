/*
  BadFIR: FIRGen that always returns null instance
*/

#ifndef BAD_FIR_H
#define BAD_FIR_H

#include "fir.h"

class BadFIR : public FIRGen
{
public:
  BadFIR()
  {}

  virtual const FIRInstance *make(int sample_rate) const
  { return 0; }

  virtual int version() const
  { return 0; }
};

#endif
