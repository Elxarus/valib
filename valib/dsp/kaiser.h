#ifndef KAISER_H
#define KAISER_H

#include <math.h>
#include "dbesi0.h"
  
/******************************************************************************

Kaiser window functions

* kaiser_alpha(a)
  a - attenuation in dB

  returns alpha parameter of a kaiser window for a given stopband attenuation

* kaiser_a(n, df)
  n - window length
  df - transition band width (normalized) 

  returns maximum possible attenuation for given window length and bandwidth

* kaiser_n(a, df)
  a - attenuation in dB
  df - transition band width (normalized)

  returns minimum number of bins for the window to fulfil the given specs.

* kaiser_window(i, n, alpha)
  i - bin number
  n - window length
  alpha - window parameter

  returns i-th bin of a window

******************************************************************************/

inline double kaiser_alpha(double a);
inline double kaiser_a(int n, double df);
inline int    kaiser_n(double a, double df);
inline double kaiser_window(double i, int n, double alpha);

/*****************************************************************************/

inline double kaiser_alpha(double a)
{
  if (a <= 21) return 0; // rectangle window
  if (a <= 50) return 0.5842 * pow(a - 21, 0.4) + 0.07886 * (a - 21);
  return 0.1102 * (a - 8.7);
}

inline double kaiser_a(int n, double df)
{
  if (double(n) * df <= 0.9) return 21; // rectangle window
  return 14.36 * df * double(n) + 7.95;
}

inline int kaiser_n(double a, double df)
{
  if (fabs(a) <= 21)
    return int(0.9 / df); // rectangle window
  else
    return int((fabs(a) - 7.95) / (14.36 * df) + 1);
}

inline double kaiser_window(double i, int n, double alpha)
{
  if (alpha == 0.0) return 1.0; // rectangle window
  double n1 = n - 1;
  return dbesi0(alpha * sqrt(1 - 4*i*i / (n1 * n1))) / dbesi0(alpha);
}

#endif
