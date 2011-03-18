#ifndef VALIB_EAC3_PARSER_H
#define VALIB_EAC3_PARSER_H

#include "../../buffer.h"
#include "../../filter.h"

struct AVCodec;
struct AVCodecContext;

class EAC3Parser : public SimpleFilter
{
public:
  EAC3Parser();
  ~EAC3Parser();

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

protected:
  Rawdata buf;      // output buffer
  Speakers out_spk; // output format
  bool new_stream_flag;

  AVCodec *avcodec;
  AVCodecContext *avctx;
};

#endif
