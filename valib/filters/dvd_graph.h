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

  DVDGraph(const Sink *sink = 0);

  void set_sink(const Sink *sink);
  const Sink *get_sink() const;

  int get_spdif_status() const;

protected:
  Demux          demux;
  Spdifer        spdifer;
  AudioDecoder   dec;
  AudioProcessor proc;
  AC3Enc         enc;

  const Sink *sink;
  enum state_t 
  { 
    state_demux = 0,          // PES demux
    state_pt,                 // SPDIF passthrough
    state_dec,                // decode
    state_proc_decode,        // processing for no spdif: spdif disabled
    state_proc_stereo,        // processing for no spdif: stereo passthrough
    state_proc_cannot_encode, // processing for no spdif: cannot encode
    state_proc_sink_refused,  // processing for no spdif: sink refused
    state_proc_spdif,         // processing for spdif: ac3 encode
    state_enc,                // AC3 encode
    state_spdif               // wrap encoded AC3 to SPDIF
  };            
  int spdif_status;


  /////////////////////////////////////////////////////////////////////////////
  // FilterGraph overrides

  virtual const char *get_name(int node) const;
  virtual const Filter *get_filter(int node) const;
  virtual Filter *init_filter(int node, Speakers spk);
  virtual int get_next(int node, Speakers spk) const;

  int decide_processor(Speakers spk) const;
};



#endif
