/*
   DecoderGraph
   Simple decoding graph that accepts PCM/SPDIF/encoded formats at input and
   allows audio processing. May be used instead of DVDGraph when we don't need
   SPDIF output and output format agreement.
*/

#ifndef VALIB_DECODER_GRAPH_H
#define VALIB_DECODER_GRAPH_H

#include "filter_graph.h"
#include "decoder.h"
#include "spdifer.h"
#include "proc.h"

class DecoderGraph : public FilterGraph
{
public:
  Despdifer      despdifer;
  AudioDecoder   dec;
  AudioProcessor proc;

public:
  DecoderGraph();

  /////////////////////////////////////////////////////////////////////////////
  // Filter overrides

  virtual void reset();
  virtual string info() const;

protected:
  enum state_t 
  { 
    node_despdif,
    node_decode,
    node_proc
  };            

  /////////////////////////////////////////////////////////////////////////////
  // FilterGraph overrides

  virtual int next_id(int id, Speakers spk) const;
  virtual Filter *init_filter(int id, Speakers spk);
};

#endif
