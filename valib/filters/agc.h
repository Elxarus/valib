/*
  Automatic Gain control filter
  todo: remove master gain


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

///////////////////////////////////////////////////////////////////////////////
// AGC class
///////////////////////////////////////////////////////////////////////////////

class AGC : public NullFilter
{
protected:
  SampleBuf w;
  SampleBuf buf[2];               // sample buffers

  size_t    block;                // current block
  size_t    sample[2];            // number of samples filled
  bool      buf_sync[2];          // beginning of the buffer is syncpoint
  time_t    buf_time[2];          // timestamp at beginning of the buffer

  size_t    nsamples;             // number of samples per block

  LevelsCache input_levels;
  LevelsCache output_levels;


  sample_t  factor;               // previous block factor
  sample_t  level;                // previous block level (not scaled)

  inline size_t next_block();

  bool fill_buffer();
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

  AGC(size_t nsamples = 1024);

  /////////////////////////////////////////////////////////
  // AGC interface

  // buffer size
  size_t get_buffer() const;
  void   set_buffer(size_t nsamples);

  // input/output levels
  inline void get_input_levels(time_t time, sample_t levels[NCHANNELS], bool drop = true);
  inline void get_output_levels(time_t time, sample_t levels[NCHANNELS], bool drop = true);

  /////////////////////////////////////////////////////////
  // Filter interface

  virtual void reset();
  virtual bool get_chunk(Chunk *out);
};


///////////////////////////////////////////////////////////////////////////////
// AGC inlines
///////////////////////////////////////////////////////////////////////////////

size_t 
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
