/*
  Delay class
  Per-channel delay buffer

  Speakers: unchanged
  Input formats: Linear
  Timing: unchanged
  Buffering: yes
  Parameters:
    units      - units in wich delay values are specified
    delays     - delay values
    [time_shift] - value of time shift should be applied at output:

  todo: time_shift parameter
*/


#ifndef DELAY_H
#define DELAY_H

#include "filter.h"


#define DELAY_SP 0 // samples
#define DELAY_MS 1 // milliseconds
#define DELAY_M  2 // meters
#define DELAY_CM 3 // centimeters
#define DELAY_FT 4 // feet 
#define DELAY_IN 5 // inches

class Delay : public NullFilter
{
public:
  Delay();

  inline bool   get_enabled();
  inline void   set_enabled(bool _enabled);

  int  get_units();
  void set_units(int units);

  void get_delays(float delays[NCHANNELS]);
  void set_delays(float delays[NCHANNELS]);

  inline float get_delay_ms();
  inline void  set_delay_ms(float delay_ms);

  // Filter interface
  virtual void reset();
  virtual bool process(const Chunk *chunk);

protected:
  bool  enabled;
  int   units;                    // delay units
  float delays[NCHANNELS];        // delay values

  SampleBuf buf;                  // sample buffer
  bool      first_half;           // first/second buffer half is used
  int       ch_delays[NCHANNELS]; // delay values in samples (reordered)
  int       lag;                  // time lag
  float     delay_ms;             // given time shift in ms

  double    units2samples(int _units);
};


inline bool Delay::get_enabled()
{ return enabled; }

inline void Delay::set_enabled(bool _enabled)
{ enabled = _enabled; }

float Delay::get_delay_ms()
{ return delay_ms; }

void  Delay::set_delay_ms(float _delay_ms)
{ delay_ms = _delay_ms; }

#endif
