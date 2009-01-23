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
  length(0),
  buf(0), data(0), spectrum(0), win(0),
  fft_ip(0), fft_w(0),
  pos(0), converted(false),
  NullFilter(FORMAT_MASK_LINEAR)
{
}

unsigned Spectrum::get_length() const
{
  return length;
}

bool Spectrum::set_length(unsigned length_)
{
  if (length == length_)
    return length == 0 || buf != 0;

  length = length_;
  if (length == 0)
  {
    uninit();
    return true;
  }
  else
    return init();
}

void Spectrum::get_spectrum(sample_t *data_, double *bin2hz)
{
  if (!buf)
    return;

  if (data_ && !converted)
  {
    size_t i;
    double norm = 1.0 / (spk.level * length);
    for (i = 0; i < 2 * length; i++)
      spectrum[i] = data[i] * win[i] * norm;

    rdft(length * 2, 1, spectrum, fft_ip, fft_w);

    for (i = 0; i < length; i++)
      spectrum[i] = sqrt(spectrum[i*2]*spectrum[i*2] + spectrum[i*2+1]*spectrum[i*2+1]);

    converted = true;
  }

  if (data_)
    memcpy(data_, spectrum, length * sizeof(sample_t));

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
  uninit();

  length = clp2(length);
  
  buf      = new sample_t[length * 6];
  fft_ip   = new int[(int)(2 + sqrt(double(length * 2)))];
  fft_w    = new sample_t[length];

  if (buf == 0 || fft_ip == 0 || fft_w == 0)
  {
    safe_delete(buf);
    safe_delete(fft_ip);
    safe_delete(fft_w);
    return false;
  }

  data     = buf;
  spectrum = buf + 2 * length;
  win      = buf + 4 * length;

  fft_ip[0] = 0;

  memset(buf, 0, 6 * length * sizeof(sample_t));

  // build the window
  double alpha = kaiser_alpha(100); // 100dB attenuation
  int odd_length = (int)length-1;
  for (int i = 0; i < 2 * odd_length + 1; i++)
    win[i] = (sample_t) kaiser_window(i - odd_length, 2 * odd_length + 1, alpha);
    
  pos = 0;
  converted = true;

  return true;
}

void
Spectrum::uninit()
{
  safe_delete(buf);
  safe_delete(fft_ip);
  safe_delete(fft_w);

  data     = 0;
  spectrum = 0;
  win      = 0;

  pos = 0;
  converted = true;
}

bool
Spectrum::on_process()
{
  if (!buf)
    return true;

  if (size == 0)
    return true;

  if (size >= 2*length)
  {
    size_t pos = size - 2 * length;
    memcpy(data, samples[0] + pos, 2*length*sizeof(sample_t));
    for (int ch = 1; ch < spk.nch(); ch++)
      for (size_t s = 0; s < 2*length; s++)
        data[s] += samples[ch][s + pos];
  }
  else
  {
    size_t pos = 2 * length - size;
    memmove(data, data + size, pos * sizeof(sample_t));
    memcpy(data + pos, samples[0], size * sizeof(sample_t));
    for (int ch = 1; ch < spk.nch(); ch++)
      for (size_t s = pos; s < 2*length; s++)
        data[s] += samples[ch][s - pos];
  }
  converted = false;
  return true;
}

void
Spectrum::on_reset()
{
  if (buf)
  {
    memset(data, 0, 2 * length * sizeof(sample_t));
    memset(spectrum, 0, 2 * length * sizeof(sample_t));
  }

  pos = 0;
  converted = true;
}
