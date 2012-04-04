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

  DRC(size_t nsamples);

  /////////////////////////////////////////////////////////
  // DRC interface

  // buffer size
  size_t get_buffer() const;
  void   set_buffer(size_t nsamples);

  /////////////////////////////////////////////////////////
  // Filter interface

  virtual bool process(Chunk &in, Chunk &out);
  virtual bool flush(Chunk &out);
  virtual void reset();
  virtual string info() const;
};

#endif
