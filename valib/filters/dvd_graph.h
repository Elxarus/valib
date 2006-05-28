#ifndef DVD_GRAPH_H
#define DVD_GRAPH_H

#include "filter_graph.h"
#include "filters\demux.h"
#include "filters\decoder.h"
#include "filters\proc.h"
#include "filters\spdifer.h"
#include "parsers\ac3\ac3_enc.h"

///////////////////////////////////////////////////////////
// SPDIF status constants
// Zero spdif status means that spdif is disabled
// Positive spdif status means spdif work status
// Negative spdif status means that something prevents
// spdif operation

#define SPDIF_DISABLED            0
#define SPDIF_PASSTHROUGH         1
#define SPDIF_ENCODE              2
#define SPDIF_STEREO_PASSTHROUGH (-1)
#define SPDIF_CANNOT_ENCODE      (-2)
#define SPDIF_SINK_REFUSED       (-3)

class DVDGraph : public FilterGraph
{
public:
  Speakers user_spk;
  bool     use_spdif;
  int      spdif_pt;
  bool     spdif_stereo_pt;

  DVDGraph()
  :proc(4096)
  {
    user_spk = Speakers(FORMAT_PCM16, 0, 0, 32768);
    use_spdif = false;

    spdif_pt = FORMAT_MASK_AC3;
    spdif_stereo_pt = true;
    spdif_status = SPDIF_DISABLED;
  };

  int get_spdif_status() { return spdif_status; };

protected:
  Demux          demux;
  Spdifer        spdifer;
  AudioDecoder   dec;
  AudioProcessor proc;
  AC3Enc         enc;

  enum state_t { state_demux = 0, state_pt, state_dec, state_proc, state_enc, state_spdif };
  int spdif_status;


  /////////////////////////////////////////////////////////////////////////////
  // FilterGraph overrides

  virtual const char *get_name(int node) const
  {
    switch (node)
    {
      case state_demux: return "Demux";
      case state_pt:    return "Spdifer (passthrough)";
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
      case state_pt:    
        spdif_status = SPDIF_PASSTHROUGH;
        return &spdifer;

      case state_dec:   return &dec;
      case state_proc:
      {
        Speakers proc_user = user_spk;

        // AC3Encoder accepts only linear format at input
        // and may refuse some formats...
        // So we must check if encoding is possible and
        // force AudioProcessor to produre linear output

        if (use_spdif)
        {
          // Determine encoder's input format

          Speakers enc_spk = proc.user2output(spk, user_spk);
          if (enc_spk.is_unknown())
            return 0;
          enc_spk.format = FORMAT_LINEAR;
          enc_spk.level = 1.0;

          if (spdif_stereo_pt && enc_spk.mask == MODE_STEREO)
          {
            // Do not encode stereo PCM
            spdif_status = SPDIF_STEREO_PASSTHROUGH;
          }
          else
          {
            // Is encoder can encode processor's output?
            if (enc.query_input(enc_spk))
            {
              spdif_status = SPDIF_ENCODE;
              proc_user.format = FORMAT_LINEAR;
              proc_user.level = 1.0;
            }
            else
              spdif_status = SPDIF_CANNOT_ENCODE;
          }
        }
        else // if (use_spdif)
        {
          spdif_status = SPDIF_DISABLED;
        }

        // Setup audio processor

        if (!proc.set_input(spk))
          return 0;

        if (!proc.set_user(proc_user))
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
    ///////////////////////////////////////////////////////
    // When get_next() is called graph must guarantee
    // that all previous filters was initialized
    // So we may use upstream filters' status
    // (here we use spdif_status updated at init_filter)
    // First filter in the chain must not use any
    // information from other filters (it has no upstream)

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

      /////////////////////////////////////////////////////
      // state_pt -> finish
      // state_pt -> state_dec

      case state_pt:

        // Spdifer may return return high-bitrare DTS stream
        // that is impossible to passthrough

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
      // state_proc -> finish

      case state_proc:
      {
        Speakers proc_user = user_spk;
        if (use_spdif)
        {
          Speakers enc_spk = proc.user2output(spk, user_spk);
          if (enc_spk.is_unknown()) return 0;
          enc_spk.format = FORMAT_LINEAR;
          enc_spk.level = 1.0;
          if (!spdif_stereo_pt || enc_spk.mask != MODE_STEREO)
            if (enc.query_input(enc_spk))
              return state_enc;
        }

        return node_end;
      }

      /////////////////////////////////////////////////////
      // state_enc -> state_spdif

      case state_enc:
        return state_spdif;

      /////////////////////////////////////////////////////
      // state_spdif -> state_dec
      // state_spdif -> finish

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
