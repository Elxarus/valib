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
  bool is_lpf;

public:
  IIRButterworth(int order, int freq, bool lpf = true): n(order), f(freq), is_lpf(lpf) {};

  virtual IIRInstance *make(int sample_rate) const;
  virtual int version() const { return ver; }

  void set(int order, int freq, bool lpf = true)
  {
    if (f != freq || n != order || is_lpf != lpf)
    { n = order; f = freq; is_lpf = lpf; ver++; }
  }
  void set_order(int order) { if (n != order) n = order, ver++; }
  void set_freq(int freq)   { if (f != freq) f = freq, ver++; }
  void set_lpf(bool lpf)    { if (is_lpf != lpf) is_lpf = lpf, ver++; }

  int get_order() const { return n; }
  int get_freq()  const { return f; }
  int get_lpf()   const { return is_lpf; }
};

#endif
