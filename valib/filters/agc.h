/*
  Automatic Gain control filter

  Speakers: unchanged
  Input formats: Linear
  Buffering: yes
  Timing: unchanged
  Paramters:
    buffer       // buffer size
    auto_gain    // automatic gain control
    normalize    // one-pass normalize
    master       // desired gain
    gain         // current gain
    release      // release speed (dB/s)
    drc          // DRC enabled
    drc_power    // DRC power (dB)
    drc_level    // current DRC gain level (read-only)
*/


#ifndef AGC_H
#define AGC_H

#include "filter.h"
#include "levels.h"

class AGC : public NullFilter
{
protected:
  SampleBuf w;
  SampleBuf buffer[2];
  bool   buf_sync[2];
  time_t buf_time[2];

  LevelsCache input_levels;
  LevelsCache output_levels;

  int       nsamples;             // number of samples per block
  int       sample;               // current sample
  int       block;                // current block
  bool      empty;                // output sample buffer is empty

  sample_t  factor;               // previous block factor
  sample_t  level;                // previous block level (not scaled)

  inline int next_block();
  void process();

public:
  // Options
  bool auto_gain;                 // [rw] automatic gain control
  bool normalize;                 // [rw] one-pass normalize

  // Gain control
  sample_t master;                // [rw] desired gain
  sample_t gain;                  // [r]  current gain
  sample_t release;               // [rw] release speed (dB/s)

  // DRC
  bool     drc;                   // [rw] DRC enabled
  sample_t drc_power;             // [rw] DRC power (dB)
  sample_t drc_level;             // [r]  current DRC gain level (read-only)

  AGC(int nsamples = 1024);

  void set_buffer(int nsamples = 1024);
  int  get_buffer();

  inline void get_input_levels(time_t time, sample_t levels[NCHANNELS], bool drop = true);
  inline void get_output_levels(time_t time, sample_t levels[NCHANNELS], bool drop = true);

  // Filter interface
  virtual void reset();
  virtual bool get_chunk(Chunk *out);
};


int 
AGC::next_block()
{
  return (block + 1) & 1;
}

void 
AGC::get_input_levels(time_t _time, sample_t _levels[NCHANNELS], bool _drop)
{
  input_levels.get_levels(_time, _levels, _drop);
}

void 
AGC::get_output_levels(time_t _time, sample_t _levels[NCHANNELS], bool _drop)
{
  output_levels.get_levels(_time, _levels, _drop);
}

#endif
