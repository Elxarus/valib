/*
  A set of Linkwitz-Riley - based crossover filters
  See http://ac3filter.net/wiki/Bass_redirection_(math) for more info.
*/

#ifndef VALIB_LINKWITZ_RILEY_H
#define VALIB_LINKWITZ_RILEY_H

#include "../iir.h"

class IIRCrossover : public IIRGen
{
public:
  enum type_t { lowpass, highpass, allpass };
  IIRCrossover(int order, int freq, type_t type): n(order), f(freq), t(type)
  {}

  virtual IIRInstance *make(int sample_rate) const;
  virtual int version() const { return ver; }

  void set(int order, int freq, type_t type)
  {
    if (f != freq || n != order || t != type)
    { n = order; f = freq; t = type; ver++; }
  }
  void set_order(int order)  { if (n != order) n = order, ver++; }
  void set_freq(int freq)    { if (f != freq) f = freq, ver++; }
  void set_type(type_t type) { if (t != type) t = type, ver++; }

  int get_order() const { return n; }
  int get_freq()  const { return f; }
  int get_type()  const { return t; }

  static IIRInstance *lowpass_proto(int m);
  static IIRInstance *highpass_proto(int m);
  static IIRInstance *allpass_proto(int m);

protected:
  int ver;
  int n, f;
  type_t t;
};

#endif
