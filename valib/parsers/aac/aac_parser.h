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

  virtual bool can_open(Speakers spk) const;
  virtual bool init();
  virtual void uninit();

  virtual void reset();
  virtual bool process(Chunk &in, Chunk &out);

  virtual bool new_stream() const
  { return new_stream_flag; }

  virtual Speakers get_output() const
  { return out_spk; }

  virtual string info() const;
};

#endif
