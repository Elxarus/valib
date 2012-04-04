/*
  Automatic Gain Control filter
  todo: remove master gain?

  Speakers: unchanged
  Input formats: Linear
  Output formats: Linear
  Buffer: +
  Inline: -
  Delay: nsamples
  Timing: unchanged
  Paramters:
    buffer       // processing buffer length in samples [offline]
    auto_gain    // automatic gain control [online]
    normalize    // one-pass normalize [online]
    master       // desired gain [online]
    gain         // current gain [online]
    release      // release speed (dB/s) [online]
*/

#ifndef VALIB_AGC_H
#define VALIB_AGC_H

#include "../buffer.h"
#include "../filter.h"
#include "../sync.h"

///////////////////////////////////////////////////////////////////////////////
// AGC class
///////////////////////////////////////////////////////////////////////////////

class AGC : public SamplesFilter
{
protected:
  vtime_t loudness_interval;      // loudness measurement interval

  SampleBuf w;
  SampleBuf buf[2];               // sample buffers
  SyncHelper sync;                // sync helper

  size_t    block;                // current block
  size_t    sample[2];            // number of samples filled
  size_t    nsamples;             // number of samples per block

  sample_t  level;                // previous block level (not scaled)

  inline size_t next_block();

  bool fill_buffer(Chunk &chunk);
  void process();

public:
  // Options
  bool auto_gain;                 // [rw] automatic gain control
  bool normalize;                 // [rw] one-pass normalize

  // Gain control
  sample_t master;                // [rw] desired gain
  sample_t gain;                  // [r]  current gain

  double attack;                  // [rw] attack speed (dB/s)
  double release;                 // [rw] release speed (dB/s)

  AGC();

  /////////////////////////////////////////////////////////
  // AGC interface

  // loudness measurement interval
  vtime_t get_loudness_interval() const;
  void    set_loudness_interval(vtime_t loudness_interval);

  /////////////////////////////////////////////////////////
  // Filter interface

  virtual bool init();
  virtual bool process(Chunk &in, Chunk &out);
  virtual bool flush(Chunk &out);
  virtual void reset();
  virtual string info() const;
};

#endif
