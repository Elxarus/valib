/**************************************************************************//**
  \file param_fir.h
  \brief ParamFIR: Parametric linear phase filter
******************************************************************************/

#ifndef VALIB_PARAM_FIR_H
#define VALIB_PARAM_FIR_H

#include "../fir.h"

/**************************************************************************//**
  \class ParamFIR
  \brief Parametric linear phase filter

  This filter implements one of the following:
  \li Low-pass filter
  \li High-pass filter
  \li Band-pass filter
  \li Band-stop filter

  \verbatim
          Lowpass                        Bandstop
    ^                           ^                      
    |                           |                      
  1 +---*      ---            1 +---*      ---       *------
    |    \      ^               |    \      ^       /
    |     \     |               |     \     |      /
    |      \    | a             |      \    | a   /
    |       \   |               |       \   |    /
    |        \  v               |        \  v   /
    |     df  *------           |     df  *----*  df
    |   <----->                 |   <----->    <----->        
  0 +---+--+--+---------> f   0 +---+--+--+-------+-----------> f
           f1                          f1         f2

  \endverbatim

  Filter is defined with:
  \li \c f1 - center frequency
  \li \c f2 - second center frequency (for bandpass/bandstop filters)
  \li \c df - trnasition band width
  \li \c a - stopband attenuation in dB

  Frequency may be specified in hertz or in normalized form.

  fn = f / sample_rate

  where
  \li fn - normalized frequency
  \li f - frequnesy in Hz
  \li sample_rate - sample rate in Hz

  Filter built is type 1 filter (odd length).

  Special cases:
  \li Negative frequency vales are prohibited. make() returns null in this
      case.
  \li Negative attenuation is prohibited. make() returns null in this case.
  \li When f2 < f1 these parameters are swapped automatically.
  \li When attenuation is zero, filter becomes passthrough filter.
  \li When center frequency is larger than nyquist frequnecy low-pass filter
      becomes passthrough and high-pass becomes gain filter with gain equals to
      \c a.
  \li When center frequency is 0, low-pass filter becomes gain filter with gain
      equals to \c a and high-pass becomes passthrough.
  \li When first center frequency is larger than nyquist frequnecy or second
      center frequency is 0, band-stop filter becomes passthrough and band-pass
      becomes gain filter with gain equals to \c a.
  \li When first center frequency is 0 and second frequency is larger than
      nyquist frequnecy, band-pass filter becomes passthrough and band-stop
      becomes gain filter with gain equals to \c a.

  Simply speaking, the range [0..nyquist] cuts the filter's frequency response.

  \fn void ParamFIR::set(filter_t type, double  f1, double  f2, double  df, double  a, bool  norm = false)
    \param type Filter type
    \param f1 First center frequency
    \param f2 Second center frequency
    \param df Transition band width
    \param a Stopband attenuation
    \param norm Frequencies are specified in normalized form

    Set filter parameters.

  \fn void ParamFIR::get(filter_t *type, double *f1, double *f2, double *df, double *a, bool *norm = 0)
    \param type Filter type
    \param f1 First center frequency
    \param f2 Second center frequency
    \param df Transition band width
    \param a Stopband attenuation
    \param norm Frequencies are specified in normalized form

   Return filter parameters. Any parameter may be null pointer, parameter
   is not returned in this case.

******************************************************************************/

class ParamFIR : public FIRGen
{
public:
  enum filter_t { low_pass = 0, high_pass, band_pass, band_stop };

  ParamFIR();
  ParamFIR(filter_t type, double f1, double f2, double df, double a, bool norm = false);

  void set(filter_t  type, double  f1, double  f2, double  df, double  a, bool  norm = false);
  void get(filter_t *type, double *f1, double *f2, double *df, double *a, bool *norm = 0);

  virtual int version() const;
  virtual const FIRInstance *make(int sample_rate) const;

protected:
  int    ver;    //!< version
  filter_t type; //!< filter type
  double f1;     //!< first center frequency
  double f2;     //!< second center frequency (not used in low/high pass filters)
  double df;     //!< transition band width
  double a;      //!< stopband attenuation (dB)
  bool norm;     //!< frequencies are specified in normalized form
};

#endif
