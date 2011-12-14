/**************************************************************************//**
  \file frame_splitter.h
  \brief FrameSplitter: Synchronize and split raw stream into frames.
******************************************************************************/

#ifndef VALIB_FRAME_SPLITTER_H
#define VALIB_FRAME_SPLITTER_H

#include "../filter.h"
#include "../parser.h"
#include "../sync.h"

/**************************************************************************//**
  \class FrameSplitter
  \brief Synchronize and split raw stream into frames.

  Uses StreamBuffer and FrameParser to do reliable synchronization and split
  raw stream.

  Accepts FORMAT_RAWDATA and formats passed FrameParser::can_parse() check.

  Output is done one frame per chunk. Output format is data-dependent and
  determined by StreamBuffer after synchronization.

  <b>Example</b>
  \code
  FrameSplitter framer(ac3_header);
  framer.open(Speakers(FORMAT_AC3, 0, 0));
  ...
  while (framer.process(ac3_raw, ac3_frame))
  {
    // do something with a single frame at ac3_frame
  }
  \endcode

  \fn bool FrameSplitter::set_parser(FrameParser *parser)
    Sets parser to use.

  \fn const FrameParser *FrameSplitter::get_parser() const
    Returns parser currently used.

  \fn int FrameSplitter::get_frames() const
    Returns number of frames decoded since creation.

  \fn string FrameSplitter::stream_info() const
    Prints stream information.

  \fn FrameInfo FrameSplitter::frame_info() const
    Returns last frame info.
******************************************************************************/

class FrameSplitter : public SimpleFilter
{
protected:
  StreamBuffer stream;        //!< stream buffer
  SyncHelper   sync;          //!< syncronization helper

  bool load_frame(Chunk &in); //!< loads a frame

public:
  FrameSplitter();
  FrameSplitter(FrameParser *parser);

  /////////////////////////////////////////////////////////
  // Own interface

  void set_parser(FrameParser *parser);
  const FrameParser *get_parser() const;

  int get_frames() const         { return stream.get_frames();  }
  string stream_info() const     { return stream.stream_info(); }
  FrameInfo frame_info() const   { return stream.frame_info();  }

  /////////////////////////////////////////////////////////
  // SimpleFilter overrides

  virtual bool can_open(Speakers spk) const;
  virtual void reset();

  virtual bool process(Chunk &in, Chunk &out);
  virtual bool flush(Chunk &out);

  virtual bool new_stream() const
  { return stream.is_new_stream(); }

  virtual Speakers get_output() const
  { return stream.get_spk(); }

  virtual string info() const
  { return stream.stream_info(); }
};

#endif
