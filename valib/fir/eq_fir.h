/**************************************************************************//**
  \file eq_fir.h
  \brief EqFIR: Graphic equalizer
******************************************************************************/

#ifndef VALIB_EQ_FIR_H
#define VALIB_EQ_FIR_H

#include "../fir.h"
#include "../auto_buf.h"

/**************************************************************************//**
  \class EqBand
  \brief Equalizer band definition.
******************************************************************************/

struct EqBand
{
  int freq;    //!< Center frequency
  double gain; //!< Gain on center frequency
};

/**************************************************************************//**
  \class EqFIR
  \brief Graphic equalizer filter.

  Builds a linear-phase equalization filter. Filter response at the band's
  center frequency is guaranteed to be equal to the band's gain.

  Filter is built as a set of step filters. Band in between of center
  frequencies of adjacent bands is considered to be a transition band. Ripple
  at pass-bands is limited and may be adjusted (less ripple, longer the filter).
  Steps less than ripple are ignored, so ripple also acts as a minimum step
  size.

  Example of 4-bands equalizer:

  \verbatim
     A
     ^
     |           v
  g2 +        *-----*    passband ripple 
     |       /   ^   \   |
     |   v  /         \  v
  g1 +  ---*           *---
     |   ^               ^
     |
     +-----+--+-----+--+--------> freq
           f1 f2    f3 f4
  \endverbatim

  Special cases:
  \li Equalizer with zero bands is passthrough filter.
  \li Equalizer with one band is gain filter.
  \li Equalizer with two bands is a single step filter.

  Frequency response at zero frequency equals to the first band's gain.
  Response at nyquist frequency equals to the last band's gain.

  Following changes are done on bands:
  \li Bands are sorted by frequency.
  \li Bands with zero or negative frequency are dropped.
  \li Maximum gain is limited by 200dB

  \fn size_t EqFIR::get_nbands() const
    Returns number of equalizer bands.

  \fn size_t EqFIR::set_bands(const EqBand *bands, size_t nbands)
    \param bands Bands array.
    \param nbands Number of bands in array.

    Set equalizer bands.

  \fn size_t EqFIR::get_bands(EqBand *bands, size_t first_band, size_t nbands) const
    \param bands Array to return bands to.
    \param first_band Index fo the first band to return.
    \param nbands Number of bands to return.

    Fills \c bands array with equalizer bands starting from \c first_band.

    Returns number of bands filled.

  \fn void EqFIR::clear_bands()
    Drop all bands from the filter.

  \fn double EqFIR::get_ripple() const
    Returns passband ripple in dB.

  \fn void EqFIR::set_ripple(double ripple_db)
    \param ripple_db Passband ripple in dB.

    Set passband ripple.

  \fn bool EqFIR::is_equalized() const
    Return true when any band has gain <> 1.0 (+- ripple)

******************************************************************************/

class EqFIR : public FIRGen
{
protected:
  int ver; // response version

  // bands info
  size_t nbands;
  AutoBuf<EqBand> bands;
  double ripple;

public:
  EqFIR();
  EqFIR(const EqBand *bands, size_t nbands);

  /////////////////////////////////////////////////////////
  // Equalizer interface

  size_t get_nbands() const;
  size_t set_bands(const EqBand *bands, size_t nbands);
  size_t get_bands(EqBand *bands, size_t first_band, size_t nbands) const;
  void   clear_bands();

  double get_ripple() const;
  void   set_ripple(double ripple_db);

  bool   is_equalized() const;

  /////////////////////////////////////////////////////////
  // FIRGen interface

  virtual int version() const;
  virtual const FIRInstance *make(int sample_rate) const;
};

#endif
