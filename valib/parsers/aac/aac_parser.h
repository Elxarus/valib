#ifndef AAC_PARSER_H
#define AAC_PARSER_H

#include "../../filter.h"
#include "../../buffer.h"

class AACParser : public SimpleFilter
{
protected:
  bool new_stream_flag;
  Speakers out_spk;
  SampleBuf buf;

  bool init_decoder();
  void* h_aac;
  bool is_fresh;

public:
  struct EDecoderInit : public Filter::Error {};

  AACParser();
  ~AACParser();

  bool can_open(Speakers spk) const;
  bool init();
  void uninit();

  void reset();
  bool process(Chunk &in, Chunk &out);

  bool new_stream() const
  { return new_stream_flag; }

  Speakers get_output() const
  { return out_spk; }
};

#endif
