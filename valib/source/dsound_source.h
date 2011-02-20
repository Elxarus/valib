/*
  DirectSound capture source
*/

#ifndef VALIB_DSOUND_SOURCE_H
#define VALIB_DSOUND_SOURCE_H

#include <dsound.h>
#include "../buffer.h"
#include "../source.h"
#include "../vtime.h"

class DSoundSource : public Source
{
protected:
  //! Processing error exception
  struct Error : public Source::Error {};

  //! DirectSound error
  struct EDirectSound : public Source::Error {};

  /////////////////////////////////////////////////////////
  // Parameters set by user and
  // directly determined from thoose

  Speakers  spk;
  DWORD     buf_size;
  size_t    buf_size_ms;
  double    bytes2time;

  /////////////////////////////////////////////////////////
  // DirectSound

  LPDIRECTSOUNDCAPTURE       ds_capture;
  LPDIRECTSOUNDCAPTUREBUFFER ds_buf;

  /////////////////////////////////////////////////////////
  // Processing variables

  Rawdata out_buf;
  bool    capturing;
  vtime_t time;
  DWORD   cur;

  void zero_all();
  bool open(WAVEFORMATEX *wf, size_t buf_size_ms, LPCGUID device = 0);

public:
  size_t chunk_size_ms;

  DSoundSource();
  DSoundSource(Speakers spk, size_t buf_size_ms, LPCGUID device = 0);
  ~DSoundSource();

  bool is_open() const;
  bool open(Speakers spk, size_t buf_size_ms, LPCGUID device = 0);
  void close();

  bool is_started() const;
  bool start();
  void stop();

  size_t  captured_size() const;
  vtime_t captured_time() const;

  /////////////////////////////////////////////////////////
  // Source interface

  virtual bool get_chunk(Chunk &out);

  virtual void reset()
  {}

  virtual bool new_stream() const
  { return false; }

  virtual Speakers get_output() const
  { return spk; }
};

#endif
