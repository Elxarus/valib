#include "dvd_graph.h"



DVDGraph::DVDGraph(const Sink *_sink)
:FilterGraph(-1), proc(4096)
{
  user_spk = Speakers(FORMAT_PCM16, 0, 0, 32768);
  use_spdif = false;

  spdif_pt = FORMAT_MASK_AC3;
  spdif_stereo_pt = true;
  spdif_status = SPDIF_DISABLED;

  sink = _sink;
};

void 
DVDGraph::set_sink(const Sink *_sink)
{
  sink = _sink;
}

const Sink *
DVDGraph::get_sink() const
{
  return sink;
}

int 
DVDGraph::get_spdif_status() const
{ 
  return spdif_status; 
};

/////////////////////////////////////////////////////////////////////////////
// FilterGraph overrides

const char *
DVDGraph::get_name(int node) const
{
  switch (node)
  {
    case state_demux:       return "Demux";
    case state_pt:          return "Spdifer (passthrough)";
    case state_dec:         return "Decoder";
    case state_proc_decode:
    case state_proc_stereo:
    case state_proc_cannot_encode:
    case state_proc_sink_refused:
    case state_proc_spdif:
                            return "Processor";
    case state_enc:         return "Encoder";
    case state_spdif:       return "Spdifer";
  }
  return 0;
}

const Filter *
DVDGraph::get_filter(int node) const
{
  switch (node)
  {
    case state_demux:       return &demux;
    case state_pt:          return &spdifer;
    case state_dec:         return &dec;
    case state_proc_decode:
    case state_proc_stereo:
    case state_proc_cannot_encode:
    case state_proc_sink_refused:
    case state_proc_spdif:
                            return &proc;
    case state_enc:         return &enc;
    case state_spdif:       return &spdifer;
  }
  return 0;
}

Filter *
DVDGraph::init_filter(int node, Speakers spk)
{
  switch (node)
  {
    case state_demux:
      return &demux;

    case state_pt:    
      spdif_status = SPDIF_PASSTHROUGH;
      return &spdifer;

    case state_dec:
      return &dec;

    case state_proc_decode:
    case state_proc_stereo:
    case state_proc_cannot_encode:
    case state_proc_sink_refused:
    {
      switch (node)
      {
        case state_proc_decode:        spdif_status = SPDIF_DISABLED; break;
        case state_proc_stereo:        spdif_status = SPDIF_STEREO_PASSTHROUGH; break;
        case state_proc_cannot_encode: spdif_status = SPDIF_CANNOT_ENCODE; break;
        case state_proc_sink_refused:  spdif_status = SPDIF_SINK_REFUSED; break;
      }

      // Setup audio processor
      if (!proc.set_user(user_spk))
        return 0;

      return &proc;
    }

    case state_proc_spdif:
    {
      // We can get here only when we're in ac3 encode mode
      spdif_status = SPDIF_ENCODE;

      // Setup audio processor
      Speakers proc_user = user_spk;
      proc_user.format = FORMAT_LINEAR;
      if (!proc.set_user(proc_user))
        return 0;

      return &proc;
    }

    case state_enc:   
      return &enc;

    case state_spdif: 
      return &spdifer;
  }
  return 0;
}

int 
DVDGraph::get_next(int node, Speakers spk) const
{
  ///////////////////////////////////////////////////////
  // When get_next() is called graph must guarantee
  // that all previous filters was initialized
  // so we may use upstream filters' status
  // (here we use spdif_status updated at init_filter() )
  // First filter in the chain must not use any
  // information from other filters (it has no upstream)

  switch (node)
  {
    /////////////////////////////////////////////////////
    // input -> state_demux
    // input -> state_pt
    // input -> state_dec
    // input -> state_proc_xxxx

    case node_end:
      if (demux.query_input(spk)) 
        return state_demux;

      if (use_spdif && (spdif_pt & FORMAT_MASK(spk.format)))
        return state_pt;

      if (dec.query_input(spk))
        return state_dec;

      if (proc.query_input(spk))
        return decide_processor(spk);

      return node_err;

    /////////////////////////////////////////////////////
    // state_demux -> state_pt
    // state_demux -> state_proc_xxxx
    // state_demux -> state_dec

    case state_demux:
      if (use_spdif && (spdif_pt & FORMAT_MASK(spk.format)))
        return state_pt;

      if (dec.query_input(spk))
        return state_dec;

      if (proc.query_input(spk))
        return decide_processor(spk);

      return node_err;

    /////////////////////////////////////////////////////
    // state_pt -> output
    // state_pt -> state_dec

    case state_pt:

      // Spdifer may return return high-bitrare DTS stream
      // that is impossible to passthrough
      if (spk.format != FORMAT_SPDIF)
        return state_dec;
      else
        return node_end;

    /////////////////////////////////////////////////////
    // state_dec -> state_proc_xxxx

    case state_dec:
      return decide_processor(spk);

    /////////////////////////////////////////////////////
    // state_proc_decode -> output
    // state_proc_stereo -> output
    // state_proc_cannot_encode -> output
    // state_proc_sink_refused  -> output

    case state_proc_decode:
    case state_proc_stereo:
    case state_proc_cannot_encode:
    case state_proc_sink_refused:
      return node_end;

    /////////////////////////////////////////////////////
    // state_proc_spdif -> state_enc

    case state_proc_spdif:
      return state_enc;


    /////////////////////////////////////////////////////
    // state_enc -> state_spdif

    case state_enc:
      return state_spdif;

    /////////////////////////////////////////////////////
    // state_spdif -> state_dec
    // state_spdif -> output

    case state_spdif:
      if (spk.format != FORMAT_SPDIF)
        return state_dec;
      else
        return node_end;
  }

  // never be here...
  return node_err;
}

int 
DVDGraph::decide_processor(Speakers spk) const
{
  // AC3Encoder accepts only linear format at input
  // and may refuse some channel configs and sample rates.
  // We should not encode stereo PCM because it
  // reduces audio quality without any benefit.
  // We should query sink about spdif support.
  //
  // We MUST change state in when conditions change 
  // because we may need different setup of AudioPorcessor
  // and we must update spdif_state. So there're 5 states:
  // * state_proc_decode - just decode without spdif output
  // * state_proc_spdif  - setup processor for ac3 encoder
  // * stare_proc_stereo - stereo passthrough (no spdif)
  // * stare_proc_cannot_encode - encoder cannot encode given format (no spdif)
  // * stare_proc_sink_refused  - sink refuses spdif format (no spdif)

  if (use_spdif)
  {
    // Determine encoder's input format
    Speakers enc_spk = proc.user2output(spk, user_spk);
    if (enc_spk.is_unknown())
      return node_err;
    enc_spk.format = FORMAT_LINEAR;
    enc_spk.level = 1.0;

    // Do not encode stereo PCM
    if (spdif_stereo_pt && enc_spk.mask == MODE_STEREO)
      return state_proc_stereo;

    // Query encoder
    if (!enc.query_input(enc_spk))
      return state_proc_cannot_encode;

    // Query sink
    if (!sink)
      return state_proc_spdif;

    if (sink->query_input(Speakers(FORMAT_SPDIF, spk.mask, spk.sample_rate)))
      return state_proc_spdif;
    else
      return state_proc_sink_refused;
  }
  else
    return state_proc_decode;
}
