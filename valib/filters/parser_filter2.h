/**************************************************************************//**
  \file parser_filter2.h
  \brief ParserFilter2: synchronize and process raw audio data.
******************************************************************************/

#ifndef VALIB_PARSER_FILTER2_H
#define VALIB_PARSER_FILTER2_H

#include "filter_graph.h"
#include "filter_switch.h"
#include "frame_splitter.h"
#include "../parsers/multi_header.h"

/**************************************************************************//**
  \class ParserFilter2
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
         HeaderParser 1           Decoder 1
         HeaderParser 2           Decoder 2
         HeaderParser 3           Decoder 3
  \endverbatim

******************************************************************************/

class ParserFilter2 : public FilterGraph
{
public:
  ParserFilter2()
  {}

  ParserFilter2(const HeaderParser *new_header, Filter *new_filter)
  { add(new_header, new_filter); }

  void add(const HeaderParser *new_header, Filter *new_filter);
  void add(const HeaderParser *new_header);
  void add(Filter *new_filter);
  void release();

protected:
  MultiHeader header;
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
