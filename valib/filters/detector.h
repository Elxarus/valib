/*
  Detector

  Format autodetection. Does not alter data, but detects compressed stream and
  its format. Does 2 jobs:
  * Detects SPDIF stream at PCM16 data.
  * Finds the full format specification (sample rate and channel configuration)
    using FrameParser. This allows the downstream to make decisions based on
    this info. For instance, do SPDIF passthrough only for 48kHz streams.
*/

#ifndef VALIB_DETECTOR_H
#define VALIB_DETECTOR_H

#include "../filter.h"
#include "../parser.h"
#include "../sync.h"
#include "../parsers/uni/uni_frame_parser.h"



class Detector : public SimpleFilter
{
protected:
  UniFrameParser uni_parser;

  enum state_t { state_load, state_frame };

  StreamBuffer stream;    // stream buffer
  SyncHelper   sync;      // synchronization helper

  Speakers out_spk;  // output format
  state_t  state;    // filter state
  bool     do_flush; // need flushing
  bool     is_new_stream;

  FrameParser *find_parser(Speakers spk);
  void load(Chunk &in);

public:
  Detector();

  /////////////////////////////////////////////////////////
  // Own interface

  int get_frames() const { return stream.get_frames(); }
  FrameInfo frame_info() const { return stream.frame_info(); }

  /////////////////////////////////////////////////////////
  // SimpleFilter overrides

  virtual bool can_open(Speakers spk) const;
  virtual bool init();
  virtual void reset();

  virtual bool process(Chunk &in, Chunk &out);
  virtual bool flush(Chunk &out);

  virtual bool new_stream() const
  { return is_new_stream; }

  virtual Speakers get_output() const
  { return out_spk; }

  string info() const
  { return stream.stream_info(); }
};

#endif
