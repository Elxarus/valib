/*
  ParserFilter
  Uses parser to transform the stream.
*/

#ifndef VALIB_PARSER_FILTER_H
#define VALIB_PARSER_FILTER_H

#include "../filter2.h"
#include "../parser.h"
#include "../sync.h"

class ParserFilter : public SimpleFilter
{
protected:
  enum state_t 
  {
    state_trans,
    state_format_change,
    state_next_frame
  };

  FrameParser *parser;       // parser to use
  StreamBuffer stream;       // stream buffer
  SyncHelper   sync;         // syncronization helper

  Speakers out_spk;          // output format
  state_t  state;            // filter state
  bool     new_stream;       // new stream found
  int      errors;           // number of parsing errors

  bool load_frame(Chunk2 &in);
  bool load_parse_frame(Chunk2 &in);
  void send_frame(Chunk2 &out);

public:
  ParserFilter();
  ParserFilter(FrameParser *parser);
  ~ParserFilter();

  /////////////////////////////////////////////////////////
  // Own interface

  bool set_parser(FrameParser *parser);
  const FrameParser *get_parser() const;

  int  get_frames() const { return stream.get_frames(); }
  int  get_errors() const { return errors; }

  size_t get_info(char *buf, size_t size) const;
  HeaderInfo header_info() const { return stream.header_info(); }

  /////////////////////////////////////////////////////////
  // SimpleFilter overrides

  virtual bool can_open(Speakers spk) const;
  virtual void reset();

  virtual bool process(Chunk2 &in, Chunk2 &out);

  virtual bool eos() const
  { return state == state_format_change; }

  virtual bool is_ofdd() const
  { return true; }

  virtual bool is_inplace() const
  { return false; }

  virtual Speakers get_output() const
  { return out_spk; }
};

#endif
