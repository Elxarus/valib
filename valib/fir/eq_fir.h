/*
  Graphic equalizer
*/

#ifndef VALIB_EQ_FIR_H
#define VALIB_EQ_FIR_H

#include "../fir.h"

class EqFIR : public FIRGen
{
protected:
  int ver; // response version

  // bands info
  size_t nbands;
  int *freq;
  double *gain;

public:
  EqFIR();
  EqFIR(size_t nbands, int *freq, double *gain);
  ~EqFIR();

  /////////////////////////////////////////////////////////
  // Equalizer interface

  size_t get_nbands() const;
  bool set_bands(size_t nbands, const int *freq, const double *gain);
  void get_bands(int *freq, double *gain) const;
  void reset();

  /////////////////////////////////////////////////////////
  // FIRGen interface

  virtual int version() const;
  virtual const FIRInstance *make(int sample_rate) const;
};

#endif
