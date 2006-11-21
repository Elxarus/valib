/*
  Decoder filter
  Uses parser to decode the stream


  Unified audio decoder

  Input formats: AC3, MPA, DTS, SPDIF
  Ouptupt formats: Linear
  Format conversions:
    AC3 -> Linear
    MPA -> Linear
    DTS -> Linear
    SPDIF/AC3 -> Linear
    SPDIF/MPA -> Linear
    SPDIF/DTS -> Linear
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

class Decoder : public NullFilter
{
protected:
  enum state_t 
  {
    state_transition, 
    state_frame_decoded, 
    state_frame_loaded, 
    state_no_data, 
    state_format_change, 
    state_flushing 
  };

  FrameParser *parser;    // parser to use
  StreamBuffer stream;    // stream buffer

  Speakers out_spk;       // output format
  state_t  state;         // filter state
  bool     new_stream;    // new stream found
  Sync     sync_helper;   // syncronization helper

  bool load_frame();
  bool decode_frame();
  bool load_decode_frame();

public:
  Decoder();
  Decoder(FrameParser *parser);
  ~Decoder();

  /////////////////////////////////////////////////////////
  // Own interface

  bool set_parser(FrameParser *parser);
  const FrameParser *get_parser() const;

  int  get_frames() const { return stream.get_frames(); }
  int  stream_info(char *buf, size_t len) const { return stream.stream_info(buf, len); }

  /////////////////////////////////////////////////////////
  // Filter interface

  virtual void reset();
  virtual bool is_ofdd() const;

  virtual bool query_input(Speakers spk) const;
  virtual bool process(const Chunk *chunk);

  virtual Speakers get_output() const;
  virtual bool is_empty() const;
  virtual bool get_chunk(Chunk *chunk);
};



class AudioDecoder : public NullFilter
{
protected:
  Speakers stream_spk;   // stream format
  Speakers out_spk;      // output format
  bool     out_flushing; // inter-stream flushing

  Sync     sync_helper;
  Parser  *parser; // current parser;

  bool load_frame();

public:
  MPAParser mpa;
  AC3Parser ac3;
  DTSParser dts;

  AudioDecoder();

  /////////////////////////////////////////////////////////
  // AudioDecoder interface

  int get_info(char *buf, size_t len) const;

  inline int get_frames();
  inline int get_errors();

  /////////////////////////////////////////////////////////
  // Filter interface

  virtual void reset();
  virtual bool is_ofdd() const;
  virtual bool set_input(Speakers _spk);
  virtual bool process(const Chunk *chunk);

  virtual Speakers get_output() const;
  virtual bool is_empty() const;
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
