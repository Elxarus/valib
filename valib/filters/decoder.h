/*
  Unified audio decoder

  Input formats: AC3, MPA, DTS
  Ouptupt formats: Linear
  Format conversions:
    AC3 -> Linear
    MPA -> Linear
    DTS -> Linear
  Timing: apply input timestamp to the first syncpoint found at the input data
  Buffering: no
*/

#ifndef DECODER_H
#define DECODER_H

#include "sync.h"
#include "convert.h"
#include "parsers\ac3\ac3_parser.h"
#include "parsers\mpa\mpa_parser.h"
#include "parsers\dts\dts_parser.h"

class AudioDecoder : public NullFilter
{
protected:
  Sync   sync_helper;
  Parser *parser; // current parser;

public:
  MPAParser mpa;
  AC3Parser ac3;
  DTSParser dts;

  AudioDecoder();

  /////////////////////////////////////////////////////////
  // AudioDecoder interface

  void get_info(char *buf, int len);

  inline int get_frames();
  inline int get_errors();

  /////////////////////////////////////////////////////////
  // Filter interface

  virtual void reset();
  virtual bool query_input(Speakers spk) const;
  virtual bool set_input(Speakers _spk);

  virtual Speakers get_output() const;
  virtual bool get_chunk(Chunk *chunk);
};


inline int 
AudioDecoder::get_frames()
{
  return mpa.get_frames() + ac3.get_frames() + dts.get_frames();
}

inline int 
AudioDecoder::get_errors()
{
  return mpa.get_errors() + ac3.get_errors() + dts.get_errors();
}


#endif
