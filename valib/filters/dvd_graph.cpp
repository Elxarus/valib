#include "dvd_graph.h"

DVDGraph::DVDGraph(const Sink *_sink)
:FilterGraph(-1), proc(4096)
{
  user_spk = Speakers(FORMAT_PCM16, 0, 0, 32768);

  use_spdif = false;
  spdif_pt = FORMAT_MASK_AC3;
  spdif_stereo_pt = true;
  spdif_as_pcm = false;
  spdif_status = SPDIF_DISABLED;

  sink = _sink;
};

///////////////////////////////////////////////////////////
// User format

bool
DVDGraph::query_user(Speakers _user_spk) const
{
  if (!user_spk.format)
    return false;

  return proc.query_user(_user_spk);
}

bool
DVDGraph::set_user(Speakers _user_spk)
{
  if (!query_user(_user_spk))
    return false;

  _user_spk.sample_rate = 0;
  if (user_spk != _user_spk)
  {
    user_spk = _user_spk;
    rebuild_node(state_proc);
    rebuild_node(state_proc_encode);
  }

  return true;
}

Speakers              
DVDGraph::get_user() const
{
  return user_spk;
}

///////////////////////////////////////////////////////////
// Sink to query

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

///////////////////////////////////////////////////////////
// SPDIF options

void
DVDGraph::set_spdif(bool _use_spdif, int _spdif_pt, bool _spdif_stereo_pt, bool _spdif_as_pcm)
{
  use_spdif = _use_spdif;
  spdif_pt = _spdif_pt;
  spdif_stereo_pt = _spdif_stereo_pt;
  spdif_as_pcm = _spdif_as_pcm;
  invalidate_chain();
}

bool
DVDGraph::get_use_spdif() const
{
  return use_spdif;
}

void
DVDGraph::set_use_spdif(bool _use_spdif)
{
  use_spdif = _use_spdif;
  invalidate_chain();
}

int
DVDGraph::get_spdif_pt() const
{
  return spdif_pt;
}

void
DVDGraph::set_spdif_pt(int _spdif_pt)
{
  spdif_pt = _spdif_pt;
  invalidate_chain();
}

bool
DVDGraph::get_spdif_stereo_pt() const
{
  return spdif_stereo_pt;
}

void
DVDGraph::set_spdif_stereo_pt(bool _spdif_stereo_pt)
{
  spdif_stereo_pt = _spdif_stereo_pt;
  invalidate_chain();
}

bool
DVDGraph::get_spdif_as_pcm() const
{
  return spdif_as_pcm;
}

void
DVDGraph::set_spdif_as_pcm(bool _spdif_as_pcm)
{
  spdif_as_pcm = _spdif_as_pcm;
  invalidate_chain();
}

int 
DVDGraph::get_spdif_status() const
{ 
  return spdif_status; 
};

void
DVDGraph::get_info(char *_buf, int _len) const
{
  if (spdif_status == SPDIF_PASSTHROUGH)
    spdifer.get_info(_buf, _len);
  else
    dec.get_info(_buf, _len);
}


/////////////////////////////////////////////////////////////////////////////
// FilterGraph overrides

const char *
DVDGraph::get_name(int node) const
{
  switch (node)
  {
    case state_demux:       return "Demux";
    case state_passthrough: return "Spdifer";
    case state_decode:      return "Decoder";
    case state_proc:
    case state_proc_encode:
                            return "Processor";
    case state_encode:      return "Encoder";
    case state_spdif:       return "Spdifer";
    case state_spdif2pcm:   return "Spdif->PCM";
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

    case state_passthrough:    
      spdif_status = SPDIF_PASSTHROUGH;
      return &spdifer;

    case state_decode:
      return &dec;

    case state_proc:
    {
      spdif_status = SPDIF_DISABLED;

      // Setup audio processor
      if (!proc.set_user(user_spk))
        return 0;

      return &proc;
    }

    case state_proc_encode:
    {
      spdif_status = SPDIF_ENCODE;

      // Setup audio processor
      Speakers proc_user = user_spk;
      proc_user.format = FORMAT_LINEAR;
      if (!proc.set_user(proc_user))
        return 0;

      return &proc;
    }

    case state_encode:   
      return &enc;

    case state_spdif: 
      return &spdifer;

    case state_spdif2pcm:
      return &spdif2pcm;
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
    // input -> state_passthrough
    // input -> state_decode
    // input -> state_proc
    // input -> state_proc_encode

    case node_start:
      if (demux.query_input(spk)) 
        return state_demux;

      if (use_spdif && (spdif_pt & FORMAT_MASK(spk.format)))
      {
        if (spdif_as_pcm)
          return state_passthrough;

        if (query_sink(Speakers(FORMAT_SPDIF, spk.mask, spk.sample_rate)))
          return state_passthrough;
      }

      if (dec.query_input(spk))
        return state_decode;

      if (proc.query_input(spk))
        return decide_processor(spk);

      return node_err;

    /////////////////////////////////////////////////////
    // state_demux -> state_passthrough
    // state_demux -> state_decode
    // state_demux -> state_proc
    // state_demux -> state_proc_encode

    case state_demux:
      if (use_spdif && (spdif_pt & FORMAT_MASK(spk.format)))
      {
        if (spdif_as_pcm)
          return state_passthrough;

        if (query_sink(Speakers(FORMAT_SPDIF, spk.mask, spk.sample_rate)))
          return state_passthrough;
      }

      if (dec.query_input(spk))
        return state_decode;

      if (proc.query_input(spk))
        return decide_processor(spk);

      return node_err;

    /////////////////////////////////////////////////////
    // state_passthrough -> output
    // state_passthrough -> state_decode

    case state_passthrough:
      // Spdifer may return return high-bitrare DTS stream
      // that is impossible to passthrough
      if (spk.format != FORMAT_SPDIF)
        return state_decode;

      if (spdif_as_pcm)
        return state_spdif2pcm;
      else
        return node_end;

    /////////////////////////////////////////////////////
    // state_decode -> state_proc
    // state_decode -> state_proc_encode

    case state_decode:
      return decide_processor(spk);

    /////////////////////////////////////////////////////
    // state_proc -> output

    case state_proc:
      return node_end;

    /////////////////////////////////////////////////////
    // state_proc_encode -> state_encode

    case state_proc_encode:
      return state_encode;

    /////////////////////////////////////////////////////
    // state_encode -> state_spdif

    case state_encode:
      return state_spdif;

    /////////////////////////////////////////////////////
    // state_spdif -> state_decode
    // state_spdif -> output

    case state_spdif:
      if (spk.format != FORMAT_SPDIF)
        return state_decode;

      if (spdif_as_pcm)
        return state_spdif2pcm;
      else
        return node_end;

    /////////////////////////////////////////////////////
    // state_spdif2pcm -> output

    case state_spdif2pcm:
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
      return state_proc;

    // Query encoder
    if (!enc.query_input(enc_spk))
      return state_proc;

    // Query sink
    if (spdif_as_pcm)
      return state_proc_encode;

    if (query_sink(Speakers(FORMAT_SPDIF, spk.mask, spk.sample_rate)))
      return state_proc_encode;
    else
      return state_proc;
  }
  else
    return state_proc;
}

bool
DVDGraph::query_sink(Speakers _spk) const
{
  if (!sink)
    return true;
  else
    return sink->query_input(_spk);
}



///////////////////////////////////////////////////////////////////////////////
// Spdif2PCM class
///////////////////////////////////////////////////////////////////////////////

Spdif2PCM::Spdif2PCM(): NullFilter(-1) 
{};

Speakers
Spdif2PCM::get_output() const
{
  if (spk.format == FORMAT_SPDIF)
    return Speakers(FORMAT_PCM16, MODE_STEREO, spk.sample_rate);
  else
    return spk;
}
bool
Spdif2PCM::get_chunk(Chunk *_chunk)
{ 
  send_chunk_inplace(_chunk, size);
  if (_chunk->spk.format == FORMAT_SPDIF)
    _chunk->spk = Speakers(FORMAT_PCM16, MODE_STEREO, spk.sample_rate);
  return true;
}
