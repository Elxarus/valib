#include <string.h>
#include "delay.h"

const float sonic_speed = 330; // [m/s]

Delay::Delay()
{
  enabled = true;
  units = DELAY_SP;
  delay_ms = 0;
  memset(delays, 0, sizeof(delays));
  reset();
}

double     
Delay::units2samples(int _units)
{
  switch (_units)
  {
    case DELAY_SP: return 1.0;
    case DELAY_MS: return double(spk.sample_rate) /*[Hz]*/ / 1000.0; /*[ms/sec]*/
    case DELAY_M:  return -1.0 * double(spk.sample_rate) /*[Hz]*/ / sonic_speed; /*[m/s]*/
    case DELAY_CM: return -1.0 * double(spk.sample_rate) /*[Hz]*/ / sonic_speed  /*[m/s]*/ / 100.0; /*[cm/m]*/
    case DELAY_FT: return -1.0 * double(spk.sample_rate) /*[Hz]*/ / sonic_speed  /*[m/s]*/ / 3.28;  /*[ft/m]*/
    case DELAY_IN: return -1.0 * double(spk.sample_rate) /*[Hz]*/ / sonic_speed  /*[m/s]*/ / 39.37; /*[in/m]*/
  }
  return 1.0;
}


int  
Delay::get_units()
{
  return units;
}

void 
Delay::set_units(int _units)
{
  double factor = units2samples(units) / units2samples(_units);

  for (int ch = 0; ch < NCHANNELS; ch++)
    delays[ch] = float(delays[ch] * factor);
  units = _units;
}

void    
Delay::set_delays(float _delays[NCHANNELS])
{
  memcpy(delays, _delays, sizeof(delays));
  reset();
}

void    
Delay::get_delays(float _delays[NCHANNELS])
{
  memcpy(_delays, delays, sizeof(delays));
}

void 
Delay::reset()
{
  chunk.set_empty();

  int ch;
  int nch = spk.nch();
  const int *order = spk.order();

  double factor = units2samples(units);
  memset(ch_delays, 0, sizeof(ch_delays));
  for (ch = 0; ch < nch; ch++)
    ch_delays[ch] = int(delays[order[ch]] * factor);

  lag = ch_delays[0];
  for (ch = 1; ch < nch; ch++)
    if (lag > ch_delays[ch])
      lag = ch_delays[ch];

  for (ch = 0; ch < nch; ch++)
    ch_delays[ch] -= lag;

  int nsamples = ch_delays[0];
  for (ch = 1; ch < nch; ch++)
    if (nsamples < ch_delays[ch])
      nsamples = ch_delays[ch];

  buf.allocate(nch, nsamples * 2);
  buf.zero();
  first_half = true;
}

bool 
Delay::process(const Chunk *_chunk)
{
  if (!NullFilter::process(_chunk))
    return false;

  if (chunk.timestamp)
    chunk.time += lag + int(delay_ms * spk.sample_rate / 1000);

  if (!enabled)
    return true; 

  int size;
  sample_t *ptr1;
  sample_t *ptr2;
  sample_t *s;
  int nsamples = chunk.size;

  for (int ch = 0; ch < chunk.spk.nch(); ch++) 
    if (ch_delays[ch])
    {
      s = chunk.samples[ch];
      size = ch_delays[ch];
      if (first_half)
      {
        ptr1 = buf[ch] + size;
        ptr2 = buf[ch];
      }
      else
      {
        ptr1 = buf[ch];
        ptr2 = buf[ch] + size;
      }

      if (nsamples > size)
      {
        memcpy(ptr2, s + nsamples - size, size * sizeof(sample_t));
        memmove(s + size, s, (nsamples - size) * sizeof(sample_t));
        memcpy(s, ptr1, size * sizeof(sample_t));
      }
      else
      {
        // optimize: use circular buffering instead of block-switching
        // it should work better for large delays
        // optimize: use larger buffer size to gain advantages of circualr 
        // buffer? (2 memcpy() calls instead of 2 memcpy and 1 memmove())
        // how large should it be? use something like set_prebuffer()?
        // do not forget about time lag in this case...
        memcpy(ptr2 + size - nsamples, s, nsamples * sizeof(sample_t));
        memcpy(s, ptr1, nsamples * sizeof(sample_t));
        memcpy(ptr2, ptr1 + nsamples, (size - nsamples) * sizeof(sample_t));
      }
    }
  first_half = !first_half;
  return true;
}

