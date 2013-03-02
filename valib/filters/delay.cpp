#include <string.h>
#include "delay.h"

// optimize: use circular buffering instead of block-switching
// it should work better for large delays
// optimize: use larger buffer delay to gain advantages of circualr 
// buffer? (2 memcpy() calls instead of 2 memcpy and 1 memmove())
// how large should it be? use something like set_prebuffer()?
// do not forget about time lag in this case...

const float sonic_speed = 330; // [m/s]

Delay::Delay()
{
  enabled = false;
  units = DELAY_SP;
  memset(delays, 0, sizeof(delays));

  reset();
}

double     
Delay::units2samples(int _units)
{
  double sample_rate = spk.is_unknown()? 48000: spk.sample_rate;
  switch (_units)
  {
    case DELAY_SP: return 1.0;
    case DELAY_MS: return sample_rate /*[Hz]*/ / 1000.0; /*[ms/sec]*/
    case DELAY_M:  return -1.0 * sample_rate /*[Hz]*/ / sonic_speed; /*[m/s]*/
    case DELAY_CM: return -1.0 * sample_rate /*[Hz]*/ / sonic_speed  /*[m/s]*/ / 100.0; /*[cm/m]*/
    case DELAY_FT: return -1.0 * sample_rate /*[Hz]*/ / sonic_speed  /*[m/s]*/ / 3.28;  /*[ft/m]*/
    case DELAY_IN: return -1.0 * sample_rate /*[Hz]*/ / sonic_speed  /*[m/s]*/ / 39.37; /*[in/m]*/
  }
  return 1.0;
}

int  
Delay::get_units() const
{
  return units;
}

void 
Delay::set_units(int _units)
{
  double factor = units2samples(units) / units2samples(_units);

  for (int ch_name = 0; ch_name < CH_NAMES; ch_name++)
    delays[ch_name] = float(delays[ch_name] * factor);
  units = _units;
}

void    
Delay::get_delays(float _delays[CH_NAMES]) const
{
  memcpy(_delays, delays, sizeof(delays));
}

void    
Delay::set_delays(const float _delays[CH_NAMES])
{
  memcpy(delays, _delays, sizeof(delays));
  init();
}

bool
Delay::init()
{
  int ch;
  const int nch = spk.nch();
  const double factor = units2samples(units);

  order_t order;
  spk.get_order(order);

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
  return true;
}

void 
Delay::reset()
{
  buf.zero();
  first_half = true;
}

bool 
Delay::process(Chunk &in, Chunk &out)
{
  // Passthrough
  out = in;
  in.clear();
  if (out.is_dummy())
    return false;

  if (!enabled)
    return true; 

  if (out.sync)
    out.time += vtime_t(lag) / spk.sample_rate;

  size_t delay;
  sample_t *ptr1;
  sample_t *ptr2;
  sample_t *s;

  const size_t size = out.size;
  for (int ch = 0; ch < spk.nch(); ch++) 
    if (ch_delays[ch])
    {
      s = out.samples[ch];
      delay = ch_delays[ch];

      if (first_half)
      {
        ptr1 = buf[ch] + delay;
        ptr2 = buf[ch];
      }
      else
      {
        ptr1 = buf[ch];
        ptr2 = buf[ch] + delay;
      }

      if (size > delay)
      {
        memcpy(ptr2, s + size - delay, delay * sizeof(sample_t));
        memmove(s + delay, s, (size - delay) * sizeof(sample_t));
        memcpy(s, ptr1, delay * sizeof(sample_t));
      }
      else
      {
        memcpy(ptr2 + delay - size, s, size * sizeof(sample_t));
        memcpy(s, ptr1, size * sizeof(sample_t));
        memcpy(ptr2, ptr1 + size, (delay - size) * sizeof(sample_t));
      }
    }
  first_half = !first_half;
  return true;
}

string
Delay::info() const
{
  const char *units_text = "";
  switch (units)
  {
    case DELAY_SP: units_text = "samples"; break;
    case DELAY_MS: units_text = "ms"; break;
    case DELAY_M:  units_text = "m"; break;
    case DELAY_CM: units_text = "cm"; break;
    case DELAY_FT: units_text = "ft"; break;
    case DELAY_IN: units_text = "in"; break;
  }

  std::stringstream s;
  s << std::boolalpha << std::fixed << std::setprecision(1);
  s << "Enabled: " << enabled << nl;
  s << "Units: " << units_text << nl;
  for (int ch_name = 0; ch_name < CH_NAMES; ch_name++)
    if (delays[ch_name] != 0)
      s << ch_name_short(ch_name) << ": " << delays[ch_name] << " " << units_text << nl;
  return s.str();
}
