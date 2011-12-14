/**************************************************************************//**
  \file parser_filter.h
  \brief ParserFilter: synchronize and process raw audio data.
******************************************************************************/

#ifndef VALIB_PARSER_FILTER_H
#define VALIB_PARSER_FILTER_H

#include "filter_graph.h"
#include "filter_switch.h"
#include "frame_splitter.h"
#include "../parsers/multi_header.h"

/**************************************************************************//**
  \class ParserFilter
  \brief Synchronize and process raw audio data.

  Uses StreamBuffer to synchronize raw audio data and process it with an
  appropriate filter. Number of parsers may be added to choose from.

  \verbatim
   +----------------------+
   |                      |
   |   +---------------+  V   +--------------+
  -+-> | FrameSplitter | ---> | FilterSwitch | -->
       +---------------+      +--------------+
               |                      |
               V                      V
         FrameParser 1            Decoder 1
         FrameParser 2            Decoder 2
         FrameParser 3            Decoder 3
  \endverbatim

******************************************************************************/

class ParserFilter : public FilterGraph
{
public:
  ParserFilter()
  {}

  ParserFilter(FrameParser *new_header, Filter *new_filter)
  { add(new_header, new_filter); }

  void add(FrameParser *new_header, Filter *new_filter);
  void add(FrameParser *new_header);
  void add(Filter *new_filter);
  void release();

  /////////////////////////////////////////////////////////
  // FrameSplitter wrapper

  int get_frames() const         { return sync.get_frames();  }
  string stream_info() const     { return sync.stream_info(); }
  FrameInfo frame_info() const   { return sync.frame_info(); }

protected:
  MultiFrameParser parser;
  FrameSplitter sync;
  FilterSwitch filter;

  enum state_t
  {
    node_sync,
    node_filter,
  };            

  /////////////////////////////////////////////////////////
  // FilterGraph overrides

  virtual int next_id(int id, Speakers spk) const;
  virtual Filter *init_filter(int id, Speakers spk);
};

#endif
