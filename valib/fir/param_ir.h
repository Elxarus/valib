/*
  Parametric filter impulse response:
  * Low-pass
  * High-pass
  * Band-pass
  * Band-stop
*/

#ifndef VALIB_PARAM_IR_H
#define VALIB_PARAM_IR_H

#include "../fir.h"

#define IR_LOW_PASS  0
#define IR_HIGH_PASS 1
#define IR_BAND_PASS 2
#define IR_BAND_STOP 3

class ParamIR : public ImpulseResponse
{
protected:
  int ver;   // response version
  int type;  // filter type
  double f1; // first bound frequency
  double f2; // second bound frequency (not used in low/high pass filters)
  double df; // transition band width
  double a;  // stopband attenuation (dB)
  bool norm; // normalized frequencies

public:
  ParamIR();
  ParamIR(int _type, double _f1, double _f2, double _df, double _a, bool _norm = false);

  void set(int  _type, double  _f1, double  _f2, double  _df, double  _a, bool  _norm = false);;
  void get(int *_type, double *_f1, double *_f2, double *_df, double *_a, bool *_norm = 0);;

  virtual int     version() const;
  virtual ir_type get_type(int sample_rate) const;
  virtual int     min_length(int sample_rate) const;
  virtual int     get_filter(int sample_rate, int n, sample_t *filter) const;
};

class ParamFIR : public FIRGen
{
protected:
  int ver;   // response version
  int type;  // filter type
  double f1; // first bound frequency
  double f2; // second bound frequency (not used in low/high pass filters)
  double df; // transition band width
  double a;  // stopband attenuation (dB)
  bool norm; // normalized frequencies

public:
  ParamFIR();
  ParamFIR(int _type, double _f1, double _f2, double _df, double _a, bool _norm = false);

  void set(int  _type, double  _f1, double  _f2, double  _df, double  _a, bool  _norm = false);;
  void get(int *_type, double *_f1, double *_f2, double *_df, double *_a, bool *_norm = 0);;

  virtual int version() const;
  virtual const FIRInstance *make(int sample_rate) const;
};


#endif
