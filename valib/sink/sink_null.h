/*
  NullSink
  Just drop all data
*/

#ifndef VALIB_SINK_NULL_H
#define VALIB_SINK_NULL_H

#include "../sink.h"

class NullSink : public SimpleSink
{
public:
  NullSink()
  {}

  virtual bool can_open(Speakers spk) const
  { return true; }

  virtual void process(const Chunk &in)
  {}
};

#endif
