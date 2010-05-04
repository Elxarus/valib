/*
  Detector

  Format autodetection. Does not alter data, but detects compressed stream and
  its format. Main purpose is to detect SPDIF stream at PCM16 data.
*/

#ifndef VALIB_DETECTOR_H
#define VALIB_DETECTOR_H

#include "../filter.h"
#include "../parser.h"
#include "../sync.h"
#include "../parsers/multi_header.h"



class Detector : public SimpleFilter
{
protected:
  MultiHeader spdif_dts_header;
  MultiHeader uni_header;

  enum state_t { state_load, state_frame };

  StreamBuffer stream;    // stream buffer
  SyncHelper   sync;      // synchronization helper

  Speakers out_spk;  // output format
  state_t  state;    // filter state
  bool     do_flush; // need flushing
  bool     is_new_stream;

  const HeaderParser *find_parser(Speakers spk) const;
  void load(Chunk2 &in);

public:
  Detector();
  ~Detector();

  /////////////////////////////////////////////////////////
  // Own interface

  int  get_frames() const { return stream.get_frames(); }
  size_t get_info(char *buf, size_t len) const { return stream.stream_info(buf, len); }
  HeaderInfo header_info() const { return stream.header_info(); }

  /////////////////////////////////////////////////////////
  // SimpleFilter overrides

  virtual bool can_open(Speakers spk) const;
  virtual bool init();
  virtual void reset();

  virtual bool process(Chunk2 &in, Chunk2 &out);
  virtual bool flush(Chunk2 &out);

  virtual bool new_stream() const
  { return is_new_stream; }

  virtual bool is_ofdd() const
  { return true; }

  virtual Speakers get_output() const
  { return out_spk; }
};

#endif
