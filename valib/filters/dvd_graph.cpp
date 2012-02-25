#include <sstream>
#include "dvd_graph.h"

DVDGraph::DVDGraph(int _nsamples, const Sink *_sink)
:proc(_nsamples)
{
  user_spk = Speakers(FORMAT_PCM16, 0, 0);

  use_spdif = false;
  use_detector = false;
  spdif_pt = FORMAT_MASK_AC3;
  spdif_as_pcm = false;
  spdif_encode = true;
  spdif_stereo_pt = true;
  spdif_bitrate = 640000;

  spdif_check_sr = false;
  spdif_allow_48 = true;
  spdif_allow_44 = false;
  spdif_allow_32 = false;

  spdif_status = SPDIF_MODE_NONE;
  spdif_err = SPDIF_MODE_NONE;

  sink = _sink;
  query_sink = true;
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

  if (user_spk != _user_spk)
  {
    user_spk = _user_spk;
    rebuild_node(state_proc);
    rebuild_node(state_proc_enc);
    rebuild_node(state_detector);
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

bool
DVDGraph::get_query_sink() const
{
  return query_sink;
}

void
DVDGraph::set_query_sink(bool _query_sink)
{
  query_sink = _query_sink;
  invalidate();
  rebuild_node(state_detector);
}


///////////////////////////////////////////////////////////
// SPDIF options

bool
DVDGraph::get_use_detector() const
{
  return use_detector;
}

void
DVDGraph::set_use_detector(bool _use_detector)
{
  use_detector = _use_detector;
  invalidate();
}

///////////////////////////////////////////////////////////
// SPDIF options

void
DVDGraph::set_spdif(bool _use_spdif, int _spdif_pt, bool _spdif_as_pcm, bool _spdif_encode, bool _spdif_stereo_pt)
{
  use_spdif = _use_spdif;
  spdif_pt = _spdif_pt;
  spdif_as_pcm = _spdif_as_pcm;
  spdif_encode = _spdif_encode;
  spdif_stereo_pt = _spdif_stereo_pt;
  invalidate();
  rebuild_node(state_detector);
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
  invalidate();
  rebuild_node(state_detector);
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
  invalidate();
  rebuild_node(state_detector);
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
  invalidate();
  rebuild_node(state_detector);
}

bool
DVDGraph::get_spdif_encode() const
{
  return spdif_encode;
}

void
DVDGraph::set_spdif_encode(bool _spdif_encode)
{
  spdif_encode = _spdif_encode;
  invalidate();
  rebuild_node(state_detector);
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
  invalidate();
  rebuild_node(state_detector);
}

int
DVDGraph::get_spdif_bitrate() const
{
  return spdif_bitrate;
}

void
DVDGraph::set_spdif_bitrate(int _spdif_bitrate)
{
  spdif_bitrate = _spdif_bitrate;
  invalidate();
  rebuild_node(state_encode);
}

///////////////////////////////////////////////////////////
// SPDIF sample rate check

void
DVDGraph::set_spdif_sr(bool _spdif_check_sr, bool _spdif_allow_48, bool _spdif_allow_44, bool _spdif_allow_32)
{
  spdif_check_sr = _spdif_check_sr;
  spdif_allow_48 = _spdif_allow_48;
  spdif_allow_44 = _spdif_allow_44;
  spdif_allow_32 = _spdif_allow_32;
  invalidate();
  rebuild_node(state_detector);
}

bool
DVDGraph::get_spdif_check_sr() const
{
  return spdif_check_sr;
}
void
DVDGraph::set_spdif_check_sr(bool _spdif_check_sr)
{
  spdif_check_sr = _spdif_check_sr;
  invalidate();
  rebuild_node(state_detector);
}

bool
DVDGraph::get_spdif_allow_48() const
{
  return spdif_allow_48;
}
void
DVDGraph::set_spdif_allow_48(bool _spdif_allow_48)
{
  spdif_allow_48 = _spdif_allow_48;
  invalidate();
  rebuild_node(state_detector);
}

bool
DVDGraph::get_spdif_allow_44() const
{
  return spdif_allow_44;
}
void
DVDGraph::set_spdif_allow_44(bool _spdif_allow_44)
{
  spdif_allow_44 = _spdif_allow_44;
  invalidate();
  rebuild_node(state_detector);
}

bool
DVDGraph::get_spdif_allow_32() const
{
  return spdif_allow_32;
}
void
DVDGraph::set_spdif_allow_32(bool _spdif_allow_32)
{
  spdif_allow_32 = _spdif_allow_32;
  invalidate();
  rebuild_node(state_detector);
}

///////////////////////////////////////////////////////////
// SPDIF/DTS mode

int
DVDGraph::get_dts_mode() const
{
  return spdifer_pt.get_dts_mode();
}
void
DVDGraph::set_dts_mode(int _dts_mode)
{
  spdifer_pt.set_dts_mode(_dts_mode);
  spdifer_enc.set_dts_mode(_dts_mode);
  invalidate();
  rebuild_node(state_detector);
}

int
DVDGraph::get_dts_conv() const
{
  return spdifer_pt.get_dts_conv();
}
void
DVDGraph::set_dts_conv(int _dts_conv)
{
  spdifer_pt.set_dts_conv(_dts_conv);
  spdifer_enc.set_dts_conv(_dts_conv);
  invalidate();
  rebuild_node(state_detector);
}

///////////////////////////////////////////////////////////
// SPDIF status

int 
DVDGraph::get_spdif_status() const
{
  return spdif_status;
}

string
DVDGraph::info() const
{
  std::stringstream result;

  result << "Input format: " << get_input().print() << nl;
  result << "User format: " << user_spk.print() << nl;
  result << "Output format: " << get_output().print() << nl;

  if (use_spdif)
  {
    result << "\nUse SPDIF\n";

    // SPDIF status

    result << "SPDIF status: ";
    switch (spdif_status)
    {
      case SPDIF_MODE_NONE:        result << "No data"; break;
      case SPDIF_MODE_PASSTHROUGH: result << "SPDIF passthrough"; break;
      case SPDIF_MODE_ENCODE:      result << "AC3 encode"; break;
      case SPDIF_MODE_DISABLED:
        result << "Disabled ";
        switch (spdif_err)
        {
          case SPDIF_ERR_STEREO_PCM:       result << "(Do not encode stereo PCM)"; break;
          case SPDIF_ERR_FORMAT:           result << "(Format is not allowed for passthrough)"; break;
          case SPDIF_ERR_SAMPLE_RATE:      result << "(Disallowed sample rate)"; break;
          case SPDIF_ERR_SINK:             result << "(SPDIF output is not supported)"; break;
          case SPDIF_ERR_ENCODER_DISABLED: result << "(AC3 encoder disabled)"; break;
          case SPDIF_ERR_PROC:             result << "(Cannot determine format to encode)"; break;
          case SPDIF_ERR_ENCODER:          result << "(Encoder does not support the format given)"; break;
        }
        break;

      default: result << "Unknown"; break;
    }
    result << nl;

    // SPDIF passthrough formats

    result << "  SPDIF passthrough for:";
    if (spdif_pt & FORMAT_MASK_MPA) result << " MPA";
    if (spdif_pt & FORMAT_MASK_AC3) result << " AC3";
    if (spdif_pt & FORMAT_MASK_DTS) result << " DTS";
    if (spdif_pt == 0) result << " -";
    result << nl;

    // Encode to AC3 option

    if (spdif_encode)
      result << "  Use AC3 encoder " << 
        spdif_stereo_pt? "(do not encode stereo PCM)": "(encode stereo PCM)";
    else
      result << "  Do not use AC3 encoder";
    result << nl;

    // SPDIF as PCM option

    if (spdif_as_pcm)
      result << "  SPDIF as PCM output" << nl;

    // Check SPDIF sample rate

    if (spdif_check_sr)
    {
      if (!spdif_allow_48 && !spdif_allow_44 && !spdif_allow_32)
        result << "  Check SPDIF sample rate: NO ONE SAMPLE RATE ALLOWED!";
      else
      {
        result << "  Check SPDIF sample rate (allow:";
        if (spdif_allow_48) result << " 48kHz";
        if (spdif_allow_44) result << " 44.1kHz";
        if (spdif_allow_32) result << " 32kHz";
        result << ")";
      }
    }
    else
      result << "  Do not check SPDIF sample rate";
    result << nl;

    // Query sink option

    if (query_sink)
      result << "  Query for SPDIF output support";
    else
      result << "  Do not query for SPDIF output support";
    result << nl;
  }

  result << "\nFilter chain:\n" << FilterGraph::info();
  return result.str();
}


/////////////////////////////////////////////////////////////////////////////
// Filter overrides

void 
DVDGraph::reset()
{
  spdif_status = use_spdif? SPDIF_MODE_NONE: SPDIF_MODE_DISABLED;
  spdif_err = use_spdif? SPDIF_MODE_NONE: SPDIF_MODE_DISABLED;

  FilterGraph::reset();
}

/////////////////////////////////////////////////////////////////////////////
// FilterGraph overrides

Filter *
DVDGraph::init_filter(int node, Speakers spk)
{
  switch (node)
  {
    case state_demux:
      return &demux;

    case state_detector:
      return &detector;

    case state_spdif_pt:    
      // reset AudioProcessor to indicate no processing 
      // activity in spdif passthrough mode
      proc.reset();

      spdif_status = SPDIF_MODE_PASSTHROUGH;
      return &spdifer_pt;

    case state_decode:
      return &dec;

    case state_proc:
    {
      spdif_status = SPDIF_MODE_DISABLED;

      // Setup audio processor
      Speakers agreed_spk = agree_output_pcm(spk, user_spk);
      if (!proc.set_user(agreed_spk))
        return 0;

      return &proc;
    }

    case state_proc_enc:
    {
      spdif_status = SPDIF_MODE_ENCODE;

      // Setup audio processor
      Speakers proc_user = user_spk;
      proc_user.format = FORMAT_LINEAR;
      if (!proc.set_user(proc_user))
        return 0;

      return &proc;
    }

    case state_encode:
      if (enc.set_bitrate(spdif_bitrate))
        return &enc;
      else
        return 0;

    case state_spdif_enc:
      return &spdifer_enc;

    case state_spdif2pcm:
      return &spdif2pcm;

    case state_dejitter:
      return &dejitter;
  }
  return 0;
}

int 
DVDGraph::next_id(int node, Speakers spk) const
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
    // input -> state_detector
    // input -> state_despdif
    // input -> state_spdif_pt
    // input -> state_decode
    // input -> state_proc
    // input -> state_proc_enc

    case node_start:
      if (demux.can_open(spk)) 
        return state_demux;

      if (use_detector && spk.format == FORMAT_PCM16 && spk.mask == MODE_STEREO)
        return state_detector;

      spdif_err = check_spdif_passthrough(spk);
      if (spdif_err == SPDIF_MODE_PASSTHROUGH)
        return state_spdif_pt;

      if (dec.can_open(spk))
        return state_decode;

      if (proc.can_open(spk))
      {
        spdif_err = check_spdif_encode(spk);
        if (spdif_err == SPDIF_MODE_ENCODE)
          return state_proc_enc;
        else
          return state_proc;
      }

      return node_err;

    /////////////////////////////////////////////////////
    // state_detector -> state_despdif
    // state_detector -> state_spdif_pt
    // state_detector -> state_decode
    // state_detector -> state_proc
    // state_detector -> state_proc_enc

    case state_detector:
      // PCM16 may be detected as SPDIF
      // so we have to demux it.
      if (demux.can_open(spk))
        return state_demux;

      spdif_err = check_spdif_passthrough(spk);
      if (spdif_err == SPDIF_MODE_PASSTHROUGH)
        return state_spdif_pt;

      if (dec.can_open(spk))
        return state_decode;

      if (proc.can_open(spk))
      {
        spdif_err = check_spdif_encode(spk);
        if (spdif_err == SPDIF_MODE_ENCODE)
          return state_proc_enc;
        else
          return state_proc;
      }

      return node_err;

    /////////////////////////////////////////////////////
    // state_demux -> state_spdif_pt
    // state_demux -> state_decode
    // state_demux -> state_proc
    // state_demux -> state_proc_enc

    case state_demux:
      spdif_err = check_spdif_passthrough(spk);
      if (spdif_err == SPDIF_MODE_PASSTHROUGH)
        return state_spdif_pt;

      if (dec.can_open(spk))
        return state_decode;

      if (proc.can_open(spk))
      {
        spdif_err = check_spdif_encode(spk);
        if (spdif_err == SPDIF_MODE_ENCODE)
          return state_proc_enc;
        else
          return state_proc;
      }

      return node_err;

    /////////////////////////////////////////////////////
    // state_spdif_pt -> state_dejitter
    // state_spdif_pt -> state_decode

    case state_spdif_pt:
      // Spdifer may return return high-bitrare DTS stream
      // that is impossible to passthrough
      if (spk.format != FORMAT_SPDIF)
        return state_decode;

      if (spdif_as_pcm)
        return state_spdif2pcm;
      else
        return state_dejitter;

    /////////////////////////////////////////////////////
    // state_decode -> state_proc
    // state_decode -> state_proc_enc

    case state_decode:
      if (proc.can_open(spk))
      {
        spdif_err = check_spdif_encode(spk);
        if (spdif_err == SPDIF_MODE_ENCODE)
          return state_proc_enc;
        else
          return state_proc;
      }

      return node_err;

    /////////////////////////////////////////////////////
    // state_proc -> state_dejitter

    case state_proc:
      return state_dejitter;

    /////////////////////////////////////////////////////
    // state_proc_enc -> state_encode

    case state_proc_enc:
      return state_encode;

    /////////////////////////////////////////////////////
    // state_encode -> state_spdif_enc

    case state_encode:
      return state_spdif_enc;

    /////////////////////////////////////////////////////
    // state_spdif_enc -> state_decode
    // state_spdif_enc -> state_dejitter

    case state_spdif_enc:
      if (spdif_as_pcm)
        return state_spdif2pcm;
      else
        return state_dejitter;

    /////////////////////////////////////////////////////
    // state_spdif2pcm -> state_dejitter

    case state_spdif2pcm:
      return state_dejitter;

    /////////////////////////////////////////////////////
    // state_dejitter -> output

    case state_dejitter:
      return node_end;

  }

  // never be here...
  return node_err;
}

int
DVDGraph::check_spdif_passthrough(Speakers _spk) const
{
  // SPDIF-1 check (passthrough)

  if (!use_spdif)
    return SPDIF_MODE_DISABLED;

  // check format
  if ((spdif_pt & FORMAT_MASK(_spk.format)) == 0)
    return SPDIF_ERR_FORMAT;

  // check sample rate
  if (spdif_check_sr && _spk.sample_rate)
    if ((!spdif_allow_48 || _spk.sample_rate != 48000) && 
        (!spdif_allow_44 || _spk.sample_rate != 44100) && 
        (!spdif_allow_32 || _spk.sample_rate != 32000))
      return SPDIF_ERR_SAMPLE_RATE;

  // check sink
  if (sink && query_sink && !spdif_as_pcm)
    if (!sink->can_open(Speakers(FORMAT_SPDIF, _spk.mask, _spk.sample_rate)))
      return SPDIF_ERR_SINK;

  return SPDIF_MODE_PASSTHROUGH;
}

int
DVDGraph::check_spdif_encode(Speakers _spk) const
{
  // SPDIF-2 check (encode)

  if (!use_spdif)
    return SPDIF_MODE_DISABLED;

  if (!spdif_encode)
    return SPDIF_ERR_ENCODER_DISABLED;

  // determine encoder's input format
  Speakers enc_spk = proc.user2output(_spk, user_spk);
  if (enc_spk.is_unknown())
    return SPDIF_ERR_PROC;
  enc_spk.format = FORMAT_LINEAR;
  enc_spk.level = 1.0;

  // do not encode stereo PCM
  if (spdif_stereo_pt && (enc_spk.mask == MODE_STEREO || enc_spk.mask == MODE_MONO))
    return SPDIF_ERR_STEREO_PCM;

  // check sample rate
  if (spdif_check_sr && enc_spk.sample_rate)
    if ((!spdif_allow_48 || enc_spk.sample_rate != 48000) && 
        (!spdif_allow_44 || enc_spk.sample_rate != 44100) && 
        (!spdif_allow_32 || enc_spk.sample_rate != 32000))
      return SPDIF_ERR_SAMPLE_RATE;

  // check encoder
  if (!enc.can_open(enc_spk))
    return SPDIF_ERR_ENCODER;

  // check sink
  if (sink && query_sink && !spdif_as_pcm)
    if (!sink->can_open(Speakers(FORMAT_SPDIF, enc_spk.mask, enc_spk.sample_rate)))
      return SPDIF_ERR_SINK;

  return SPDIF_MODE_ENCODE;
}

Speakers
DVDGraph::agree_output_pcm(Speakers _spk, Speakers _user_spk) const
{
  if (!sink || !query_sink)
    return _user_spk;

  // Apply user_spk to the input format

  if (_user_spk.format != FORMAT_UNKNOWN)
  {
    _spk.format = _user_spk.format;
    _spk.level = _user_spk.level;
  }

  if (_user_spk.mask)
    _spk.mask = _user_spk.mask;

  if (_user_spk.sample_rate)
    _spk.sample_rate = _user_spk.sample_rate;

  _spk.relation = _user_spk.relation;

  // Query direct user format

  if (sink->can_open(_spk) && proc.query_user(_spk))
    return _spk;

  // We're failed. 
  // Try to downgrade the format on the first pass.
  // Try do downgrage the format and the number of channels on the second pass.

  for (int i = 0; i < 2; i++)
  {
    Speakers enum_spk = _spk;
    if (i > 0) enum_spk.mask = MODE_STEREO;

    while (!enum_spk.is_unknown())
    {
      if (sink->can_open(enum_spk) && proc.query_user(enum_spk))
        return enum_spk;

      switch (enum_spk.format)
      {
        case FORMAT_LINEAR:   enum_spk.format = FORMAT_PCMFLOAT; break;
        case FORMAT_PCMFLOAT: enum_spk.format = FORMAT_PCM32; break;
        case FORMAT_PCM32:    enum_spk.format = FORMAT_PCM24; break;
        case FORMAT_PCM24:    enum_spk.format = FORMAT_PCM16; break;
        case FORMAT_PCM16:    enum_spk.format = FORMAT_UNKNOWN; break;

        case FORMAT_PCM32_BE: enum_spk.format = FORMAT_PCM24_BE; break;
        case FORMAT_PCM24_BE: enum_spk.format = FORMAT_PCM16_BE; break;
        case FORMAT_PCM16_BE: enum_spk.format = FORMAT_UNKNOWN; break;
        default:
          enum_spk.format = FORMAT_UNKNOWN; break;
      }
    }
  }

  // Failed to downgrade the format and number of channels.
  // Try to use format of the sink.

  _spk = sink->get_input();
  if (proc.query_user(_spk))
    return _spk;

  // Everything failed. Use user_spk in the hope that it still may work...
  return _user_spk;
}
