#ifndef DVD_GRAPH_H
#define DVD_GRAPH_H

#include "filter_graph.h"
#include "filters\demux.h"
#include "filters\decoder.h"
#include "filters\proc.h"
#include "filters\spdifer.h"
#include "parsers\ac3\ac3_enc.h"

class DVDGraph : public FilterGraph
{
public:
  Speakers user_spk;
  bool     use_spdif;
  int      spdif_pt;

  DVDGraph()
  :proc(4096)
  {
    user_spk = spk_unknown;
    use_spdif = false;

    spdif_pt = FORMAT_MASK_AC3;
  };

protected:
  Demux          demux;
  Spdifer        spdifer;
  AudioDecoder   dec;
  AudioProcessor proc;
  AC3Enc         enc;

  enum state_t { state_input = 1, state_demux, state_pt, state_dec, state_proc, state_enc, state_spdif };

  virtual const char *get_name(int node) const
  {
    switch (node)
    {
      case state_demux: return "Demux";
      case state_pt:    return "Spdifer";
      case state_dec:   return "Decoder";
      case state_proc:  return "Processor";
      case state_enc:   return "Encoder";
      case state_spdif: return "Spdifer";
    }
    return 0;
  }

  virtual const Filter *get_filter(int node) const
  {
    switch (node)
    {
      case state_demux: return &demux;
      case state_pt:    return &spdifer;
      case state_dec:   return &dec;
      case state_proc:  return &proc;
      case state_enc:   return &enc;
      case state_spdif: return &spdifer;
    }
    return 0;
  }

  virtual Filter *init_filter(int node, Speakers spk)
  {
    switch (node)
    {
      case state_demux: return &demux;
      case state_pt:    return &spdifer;
      case state_dec:   return &dec;
      case state_proc:
      {
        Speakers proc_out = user_spk;
        if (proc_out.format == FORMAT_UNKNOWN)
          proc_out.format = spk.format;

        if (proc_out.mask == FORMAT_UNKNOWN)
          proc_out.mask = spk.mask;

        proc_out.sample_rate = spk.sample_rate;

        if (use_spdif)
        {
          Speakers enc_spk = proc_out;
          enc_spk.format = FORMAT_LINEAR;
          if (enc.query_input(enc_spk))
            proc_out = enc_spk;
        }

        if (!proc.set_input(spk)) 
          return 0;

        if (!proc.set_output(spk)) 
          return 0;

        return &proc;
      }

      case state_enc:   return &enc;
      case state_spdif: return &spdifer;
    }
    return 0;
  }

  virtual int get_next(int node, Speakers spk) const
  {
    switch (node)
    {
      /////////////////////////////////////////////////////
      // state_input -> state_demux
      // state_input -> state_pt
      // state_input -> state_dec
      // state_input -> state_proc

      case node_end:
        if (demux.query_input(spk)) 
          return state_demux;

        if (use_spdif && (spdif_pt & FORMAT_MASK(spk.format)))
          return state_pt;

        if (dec.query_input(spk))
          return state_dec;

        if (proc.query_input(spk))
          return state_proc;

        return node_err;

      /////////////////////////////////////////////////////
      // state_demux -> state_pt
      // state_demux -> state_proc
      // state_demux -> state_dec

      case state_demux:
        if (use_spdif && (spdif_pt & FORMAT_MASK(spk.format)))
          return state_pt;

        if (proc.query_input(spk))
          return state_proc;

        if (dec.query_input(spk))
          return state_dec;

        return node_err;

      case state_pt:
        if (spk.format != FORMAT_SPDIF)
          return state_dec;
        else
          return node_end;

      /////////////////////////////////////////////////////
      // state_dec -> state_proc

      case state_dec:
        return state_proc;

      /////////////////////////////////////////////////////
      // state_proc -> state_enc
      // state_proc -> state_output

      case state_proc:
        if (use_spdif)
          if (enc.query_input(spk))
            return state_enc;

        return node_end;

      /////////////////////////////////////////////////////
      // state_enc -> state_spdif

      case state_enc:
        return state_spdif;

      /////////////////////////////////////////////////////
      // state_spdif -> state_dec
      // state_spdif -> state_output

      case state_spdif:
        if (spk.format != FORMAT_SPDIF)
          return state_dec;
        else
          return node_end;
    }

    // never be here...
    return node_err;
  }
};

#endif
