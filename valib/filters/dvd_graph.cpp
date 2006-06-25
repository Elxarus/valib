#include "dvd_graph.h"



DVDGraph::DVDGraph()
:FilterGraph(-1), proc(4096)
{
  user_spk = Speakers(FORMAT_PCM16, 0, 0, 32768);
  use_spdif = false;

  spdif_pt = FORMAT_MASK_AC3;
  spdif_stereo_pt = true;
  spdif_status = SPDIF_DISABLED;
};

int 
DVDGraph::get_spdif_status() const
{ 
  return spdif_status; 
};

const char *
DVDGraph::get_name(int node) const
{
  switch (node)
  {
    case state_demux:       return "Demux";
    case state_pt:          return "Spdifer (passthrough)";
    case state_dec:         return "Decoder";
    case state_proc_decode: return "Processor";
    case state_proc_spdif:  return "Processor";
    case state_proc_stereo: return "Processor";
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
    case state_proc_decode: return &proc;
    case state_proc_spdif:  return &proc;
    case state_proc_stereo: return &proc;
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
    {
      // If spdif is enabled we can get here only 
      // when encoder does not support current output format

      spdif_status = use_spdif? SPDIF_CANNOT_ENCODE: SPDIF_DISABLED;

      // Setup audio processor
      if (!proc.set_user(user_spk))
        return 0;

      return &proc;
    }

    case state_proc_stereo:
    {
      spdif_status = SPDIF_STEREO_PASSTHROUGH;

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
    // input -> state_proc_decode
    // input -> state_proc_spdif
    // input -> state_proc_stereo

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
    // state_demux -> state_proc_decode
    // state_demux -> state_proc_spdif
    // state_demux -> state_proc_stereo
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
    // state_dec -> state_proc_decode
    // state_dec -> state_proc_spdif
    // state_dec -> state_proc_stereo

    case state_dec:
      return decide_processor(spk);

    /////////////////////////////////////////////////////
    // state_proc_decode -> output
    // state_proc_stereo -> output

    case state_proc_decode:
    case state_proc_stereo:
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
  // and may refuse some formats.
  // We should not encode stereo PCM because it
  // reduces audio quality without any benefit.
  //
  // Because we need to distinguish different
  // processing mode and do different setup
  // of AudioProcessor it is 3 different states:
  // * state_proc_decode - just decode without spdif output
  // * state_proc_spdif  - setup processor for ac3 encoder
  // * stare_proc_stereo - stereo passthrough (do not encode)

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
    if (enc.query_input(enc_spk))
      return state_proc_spdif;
  }

  return state_proc_decode;
}
