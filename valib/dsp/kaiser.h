#ifndef KAISER_H
#define KAISER_H

#include <math.h>
#include "dbesi0.h"
  
/******************************************************************************

Kaiser window functions

* kaiser_alpha(a)
  a - attenuation in dB

  returns alpha parameter of a kaiser window for a given stopband attenuation

* kaiser_n(a, df)
  a - attenuation in dB
  df - transition band width (normalized)

  returns minimum number of bins for the window to fulfil the given specs.

* kaiser_window(i, n, alpha)
  i - bin number
  n - window langth
  alpha - window parameter

  returns i-th bin of a window

******************************************************************************/

inline double kaiser_alpha(double a);
inline int    kaiser_n(double a, double df);
inline double kaiser_window(double i, int n, double alpha);

/*****************************************************************************/

inline double kaiser_alpha(double a)
{
  if (a <= 21) return 0;
  if (a <= 50) return 0.5842 * pow(a - 21, 0.4) + 0.07886 * (a - 21);
  return 0.1102 * (a - 8.7);
}

inline int kaiser_n(double a, double df)
{
  return (int) ((a - 7.95) / (14.36 * df) + 1);
}

inline double kaiser_window(double i, int n, double alpha)
{
  double n1 = n - 1;
  return dbesi0(alpha * sqrt(1 - 4*i*i / (n1 * n1))) / dbesi0(alpha);
}

#endif
