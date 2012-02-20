#include <sstream>
#include "decoder_graph.h"

DecoderGraph::DecoderGraph()
:proc(4096)
{}

/////////////////////////////////////////////////////////////////////////////
// Filter overrides

void 
DecoderGraph::reset()
{
  FilterGraph::reset();
}

string
DecoderGraph::info() const
{
  using std::endl;
  std::stringstream result;

  result << "Input format: " << get_input().print() << endl;
  result << "User format: " << proc.get_user().print() << endl;
  result << "Output format: " << get_output().print() << endl;

  result << endl << "Filter chain:" << endl << FilterGraph::info();
  return result.str();
}

/////////////////////////////////////////////////////////////////////////////
// FilterGraph overrides

Filter *
DecoderGraph::init_filter(int node, Speakers spk)
{
  switch (node)
  {
    case node_despdif: return &despdifer;
    case node_decode:  return &dec;
    case node_proc:    return &proc;
  }
  return 0;
}

int 
DecoderGraph::next_id(int id, Speakers spk) const
{
  switch (id)
  {
    /////////////////////////////////////////////////////
    // input -> state_despdif
    // input -> state_decode
    // input -> state_proc

    case node_start:
      if (despdifer.can_open(spk))
        return node_despdif;

      if (dec.can_open(spk))
        return node_decode;

      if (proc.can_open(spk))
        return node_proc;

      return node_err;

    /////////////////////////////////////////////////////
    // state_despdif -> state_decode

    case node_despdif:
      if (dec.can_open(spk))
        return node_decode;

      return node_err;

    /////////////////////////////////////////////////////
    // state_decode -> state_proc

    case node_decode:
      if (proc.can_open(spk))
        return node_proc;

      return node_err;

    /////////////////////////////////////////////////////
    // state_proc -> output

    case node_proc:
      return node_end;
  }

  // never be here...
  return node_err;
}
