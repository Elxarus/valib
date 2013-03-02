/*
  MPEG Audio parser using libmpa123
*/

#ifndef VALIB_MPA_MPG123_H
#define VALIB_MPA_MPG123_H

#include "../../buffer.h"
#include "../../filter.h"
#include "../../sync.h"

class MPG123Parser : public SimpleFilter
{
public:
  int frames;
  int errors;

  MPG123Parser();
  ~MPG123Parser();

  /////////////////////////////////////////////////////////
  // SimpleFilter overrides

  bool can_open(Speakers spk) const;
  bool init();

  void reset();
  bool process(Chunk &in, Chunk &out);

  bool new_stream() const
  { return new_stream_state == new_stream_now; }

  Speakers get_output() const
  { return out_spk; }

  string info() const;

private:
  void *mh; // mpg123_handle

  enum { state_feed, state_decode } state;
  enum { new_stream_next, new_stream_now, no_new_stream} new_stream_state;

  SyncHelper sync;
  Speakers out_spk; // output format
};

#endif
