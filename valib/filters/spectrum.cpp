#include <math.h>
#include <string.h>
#include "../dsp/fftsg.h"
#include "../dsp/kaiser.h"
#include "spectrum.h"

inline unsigned int clp2(unsigned int x)
{
  // smallest power-of-2 >= x
  x = x - 1;
  x = x | (x >> 1);
  x = x | (x >> 2);
  x = x | (x >> 4);
  x = x | (x >> 8);
  x = x | (x >> 16);
  return x + 1;
}

Spectrum::Spectrum():
length(0), pos(0), is_ok(true)
{}

unsigned Spectrum::get_length() const
{
  return length;
}

bool Spectrum::set_length(unsigned length_)
{
  if (length == length_)
    return length == 0 || is_ok;

  length = length_;
  if (length == 0)
  {
    pos = 0;
    is_ok = true;
    return true;
  }
  else
    return init();
}

void Spectrum::get_spectrum(int ch, sample_t *out, double *bin2hz)
{
  if (!is_ok)
    return;

  if (out)
  {
    size_t i;

    // copy samples to spectrum buffer;
    if (ch < 0 || ch >= CH_NAMES)
    {
      memcpy(spectrum, data[0], length * 2 * sizeof(sample_t));
      for (ch = 1; ch < spk.nch(); ch++)
        for (i = 0; i < 2 * length; i++)
          spectrum[i] += data[ch][i];
    }
    else if (CH_MASK(ch) & spk.mask)
      memcpy(spectrum, data[ch], length * 2 * sizeof(sample_t));
    else
      memset(spectrum, 0, length * 2 * sizeof(sample_t));

    // apply the window and normalize
    double norm = 1.0 / (spk.level * length);
    for (i = 0; i < 2 * length; i++)
      spectrum[i] *= win[i] * norm;

    // FFT
    fft.rdft(spectrum);

    // amplitude
    for (i = 0; i < length; i++)
      out[i] = sqrt(spectrum[i*2]*spectrum[i*2] + spectrum[i*2+1]*spectrum[i*2+1]);
  }

  if (bin2hz)
  {
    if (length && spk.sample_rate)
      *bin2hz = double(spk.sample_rate) / double(2 * length);
    else
      *bin2hz = 0.0;
  }
}

bool
Spectrum::init()
{
  int nch = spk.nch();
  length = clp2(length);

  data.allocate(nch, length * 2);
  data.zero();

  spectrum.allocate(length * 2);
  win.allocate(length * 2);
  fft.set_length(length * 2);

  if (!data.is_allocated() ||
      !spectrum.is_allocated() ||
      !win.is_allocated() ||
      !fft.is_ok())
  {
    is_ok = false;
    return false;
  }

  // build the window
  double alpha = kaiser_alpha(100); // 100dB attenuation
  int odd_length = length-1;
  for (int i = 0; i < 2 * odd_length + 1; i++)
    win[i] = (sample_t) kaiser_window(i - odd_length, 2 * odd_length + 1, alpha);
  win[length * 2 - 1] = 0;
    
  pos = 0;
  is_ok = true;
  return true;
}

bool
Spectrum::process(Chunk2 &in, Chunk2 &out)
{
  out = in;
  in.clear();
  if (out.is_dummy())
    return false;

  if (!is_ok || out.size == 0)
    return true;

  if (out.size >= 2*length)
  {
    size_t pos = out.size - 2 * length;
    for (int ch = 0; ch < spk.nch(); ch++)
      memcpy(data[ch], out.samples[ch] + pos, 2*length*sizeof(sample_t));
  }
  else
  {
    size_t pos = 2 * length - out.size;
    for (int ch = 0; ch < spk.nch(); ch++)
    {
      memmove(data[ch], data[ch] + out.size, pos * sizeof(sample_t));
      memcpy(data[ch] + pos, out.samples[ch], out.size * sizeof(sample_t));
    }
  }
  return true;
}

void
Spectrum::reset()
{
  data.zero();
  pos = 0;
}
