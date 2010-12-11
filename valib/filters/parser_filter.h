/*
  ParserFilter
  Uses parser to transform the stream.
*/

#ifndef VALIB_PARSER_FILTER_H
#define VALIB_PARSER_FILTER_H

#include "../filter.h"
#include "../parser.h"
#include "../sync.h"

class ParserFilter : public SimpleFilter
{
protected:
  FrameParser *parser;       // parser to use
  StreamBuffer stream;       // stream buffer
  SyncHelper   sync;         // syncronization helper

  Speakers out_spk;          // output format
  bool     is_new_stream;    // new stream found
  int      errors;           // number of parsing errors

  bool load_frame(Chunk &in);

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

  string get_info() const;
  HeaderInfo header_info() const { return stream.header_info(); }

  /////////////////////////////////////////////////////////
  // SimpleFilter overrides

  virtual bool can_open(Speakers spk) const;
  virtual void reset();

  virtual bool process(Chunk &in, Chunk &out);

  virtual bool new_stream() const
  { return is_new_stream; }

  virtual Speakers get_output() const
  { return out_spk; }
};

#endif
