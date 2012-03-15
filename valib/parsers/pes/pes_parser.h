#ifndef PES_PARSER_H
#define PES_PARSER_H

#include "../../filter.h"

class PESParser : public SimpleFilter
{
public:
  PESParser();

  /////////////////////////////////////////////////////////
  // SimpleFilter overrides

  bool can_open(Speakers spk) const;
  bool init();

  void reset();
  bool process(Chunk &in, Chunk &out);

  bool new_stream() const
  { return new_stream_flag; }

  Speakers get_output() const
  { return out_spk; }

  string info() const;

protected:
  bool sync;
  vtime_t time;
  Speakers out_spk;
  bool new_stream_flag;

  int stream;
  int substream;
};

#endif
