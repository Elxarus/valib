/*
  NullSink
  Just drop all data
*/

#ifndef VALIB_SINK_NULL_H
#define VALIB_SINK_NULL_H

#include "../sink.h"

class NullSink2 : public SimpleSink
{
public:
  NullSink2()
  {}

  virtual bool can_open(Speakers spk) const
  { return true; }

  virtual void process(const Chunk2 &in)
  {}
};

#endif
