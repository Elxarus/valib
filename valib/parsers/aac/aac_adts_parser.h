#ifndef VALIB_AAC_ADTS_PARSER_H
#define VALIB_AAC_ADTS_PARSER_H

#include "../../filter.h"

class ADTSParser : public SimpleFilter
{
public:
  ADTSParser();

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
  Speakers out_spk;
  bool new_stream_flag;
  Rawdata header;
};

#endif
