#include <stdio.h>
#include "decoder_graph.h"

DecoderGraph::DecoderGraph()
:proc(4096)
{}

///////////////////////////////////////////////////////////
// User format

size_t
DecoderGraph::get_info(char *_buf, size_t _len) const
{
  Speakers spk;
  static const size_t buf_size = 2048;
  char buf[buf_size];
  size_t pos = 0;

  spk = get_input();
  pos += sprintf(buf + pos, "Input format: %s %s %i\n", spk.format_text(), spk.mode_text(), spk.sample_rate);

  spk = proc.get_user();
  pos += sprintf(buf + pos, "User format: %s %s %i\n", spk.format_text(), spk.mode_text(), spk.sample_rate);

  spk = get_output();
  pos += sprintf(buf + pos, "Output format: %s %s %i\n", spk.format_text(), spk.mode_text(), spk.sample_rate);
/*
  if (next_id(node_start) != node_end)
  {
    pos += sprintf(buf + pos, "\nDecoding chain:\n");
    pos += chain_text(buf + pos, buf_size - pos);

    pos += sprintf(buf + pos, "\n\nFilters info (in order of processing):\n\n");
    int node = next_id(node_start);
    while (node != node_end)
    {
      const char *filter_name = get_name(node);
      if (!filter_name) filter_name = "Unknown filter";
      pos += sprintf(buf + pos, "%s:\n", filter_name);

      switch (node)
      {
      case node_decode:
        pos += dec.get_info(buf + pos, buf_size - pos);
        break;

      case node_proc:
        pos += proc.get_info(buf + pos, buf_size - pos);
        pos += sprintf(buf + pos, "\n");
        break;

      default:
        pos += sprintf(buf + pos, "-\n");
        break;
      }
      pos += sprintf(buf + pos, "\n");
      node = next_id(node);
    }
  }
*/
  if (pos + 1 > _len) pos = _len - 1;
  memcpy(_buf, buf, pos + 1);
  _buf[pos] = 0;
  return pos;
}


/////////////////////////////////////////////////////////////////////////////
// Filter overrides

void 
DecoderGraph::reset()
{
  FilterGraph::reset();
}

/////////////////////////////////////////////////////////////////////////////
// FilterGraph overrides

std::string
DecoderGraph::get_name(int node) const
{
  switch (node)
  {
    case node_despdif: return "Despdif";
    case node_decode:  return "Decoder";
    case node_proc:    return "Processor";
  }
  return 0;
}

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
