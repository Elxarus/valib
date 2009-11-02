/*
  Butterworth IIR filter
*/

#ifndef VALIB_BUTTERWORTH_H
#define VALIB_BUTTERWORTH_H

#include "../iir.h"

IIRInstance *butterworth_proto(int sample_rate, int n);

class IIRButterworth : public IIRGen
{
protected:
  int ver;
  int n, f;

public:
  IIRButterworth(int order, int freq): n(order), f(freq) {};

  virtual IIRInstance *make(int sample_rate) const;
  virtual int version() const { return ver; }

  void set(int order, int freq) { if (f != freq || n != order) n = order, f = freq, ver++; }
  void set_order(int order)     { if (n != order) n = order, ver++; }
  void set_freq(int freq)       { if (f != freq) f = freq, ver++; }

  int get_order() const { return n; }
  int get_freq() const { return f; }
};

#endif
