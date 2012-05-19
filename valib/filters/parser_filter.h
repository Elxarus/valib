/**************************************************************************//**
  \file parser_filter.h
  \brief ParserFilter: synchronize and process raw audio data.
******************************************************************************/

#ifndef VALIB_PARSER_FILTER_H
#define VALIB_PARSER_FILTER_H

#include <vector>
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
  ParserFilter();
  ParserFilter(FrameParser *frame_parser, Filter *filter);

  void add(FrameParser *frame_parser, Filter *filter);
  void add(Filter *filter);
  void release();

  /////////////////////////////////////////////////////////
  // Filter overrides

  bool can_open(Speakers spk) const;
  bool open(Speakers spk);

  /////////////////////////////////////////////////////////
  // FrameSplitter wrapper

  int get_frames() const         { return sync.get_frames();  }
  string stream_info() const     { return sync.stream_info(); }
  FrameInfo frame_info() const   { return sync.frame_info();  }

protected:
  enum state_t
  {
    node_sync,
    node_filter,
  };

  struct Entry
  {
    FrameParser *frame_parser;
    Filter *filter;

    Entry():
    frame_parser(0), filter(0)
    {}

    Entry(FrameParser *frame_parser_, Filter *filter_):
    frame_parser(frame_parser_), filter(filter_)
    {}
  };

  std::vector<Entry> list;
  FrameSplitter sync;
  FrameParser *frame_parser;
  Filter *filter;

  /////////////////////////////////////////////////////////
  // FilterGraph overrides

  virtual int next_id(int id, Speakers spk) const;
  virtual Filter *init_filter(int id, Speakers spk);
};

#endif
