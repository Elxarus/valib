/*
  Dynamic range compression
*/

#ifndef VALIB_DRC_H
#define VALIB_DRC_H

#include "../buffer.h"
#include "../filter.h"
#include "../sync.h"

///////////////////////////////////////////////////////////////////////////////
// DRC class
///////////////////////////////////////////////////////////////////////////////

class DRC : public SamplesFilter
{
protected:
  vtime_t loudness_interval;      // loudness measurement interval

  SampleBuf w;
  SampleBuf buf[2];               // sample buffers
  SyncHelper sync;                // sync helper

  size_t    block;                // current block
  size_t    sample[2];            // number of samples filled
  size_t    nsamples;             // number of samples per block

  sample_t  factor;               // previous block factor
  sample_t  level;                // previous block level (not scaled)

  inline size_t next_block();

  bool fill_buffer(Chunk &chunk);
  void process();

public:
  bool     drc;        // [rw] DRC enabled
  sample_t drc_power;  // [rw] DRC power (dB)
  sample_t drc_level;  // [r]  current DRC gain level (read-only)

  sample_t gain;       // [rw] desired gain
  double   attack;     // [rw] attack speed (dB/s)
  double   release;    // [rw] release speed (dB/s)

  DRC();

  /////////////////////////////////////////////////////////
  // DRC interface

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
