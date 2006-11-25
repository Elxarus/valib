/*
  ParserFilter
  Uses parser to trabsform the stream
*/

#ifndef PARSER_FILTER_H
#define PARSER_FILTER_H

#include "parser.h"
#include "sync.h"

class ParserFilter : public NullFilter
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
  int      errors;        // number of parsing errors

  bool load_frame();
  bool parse_frame();
  bool load_parse_frame();

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

  size_t get_info(char *buf, size_t len) const { return stream.stream_info(buf, len); }
  HeaderInfo header_info() const { return stream.header_info(); }

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

#endif
