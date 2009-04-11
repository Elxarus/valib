#include "equalizer_mch.h"

size_t
EqualizerMch::get_nbands(int ch_name) const
{
  if (ch_name < 0 || ch_name > NCHANNELS) return 0;
  return ch_eq[ch_name].get_nbands();
}

bool
EqualizerMch::set_bands(int ch_name, size_t nbands, const int *freq, const double *gain)
{
  if (ch_name < 0 || ch_name > NCHANNELS) return 0;
  return ch_eq[ch_name].set_bands(nbands, freq, gain);
}

void
EqualizerMch::get_bands(int ch_name, int *freq, double *gain, int first_band, int nbands) const
{
  int i;
  if (ch_name < 0 || ch_name > NCHANNELS)
  {
    if (freq) for (i = 0; i < nbands; i++) freq[i] = 0;
    if (gain) for (i = 0; i < nbands; i++) gain[i] = 0;
  }
  ch_eq[ch_name].get_bands(freq, gain, first_band, nbands);
}

void
EqualizerMch::reset_eq(int ch_name)
{
  if (ch_name < 0 || ch_name > NCHANNELS) return;
  ch_eq[ch_name].reset();
}