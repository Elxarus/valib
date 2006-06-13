#ifndef DSOUND_SOURCE_H
#define DSOUND_SOURCE_H

#include <dsound.h>
#include "filter.h"
#include "data.h"

class DSoundSource : public Source
{
protected:
  Speakers                   spk;
  LPDIRECTSOUNDCAPTURE       ds_capture;
  LPDIRECTSOUNDCAPTUREBUFFER ds_buf;

  DataBuf out_buf;
  size_t  buf_size;
  DWORD   cur;

  void zero_all();
  bool open(WAVEFORMATEX *wf, size_t buf_size_ms, LPCGUID device = 0);

public:
  size_t chunk_size_ms;

  DSoundSource();
  DSoundSource(Speakers spk, size_t buf_size_ms, LPCGUID device = 0);
  ~DSoundSource();

  bool open(Speakers spk, size_t buf_size_ms, LPCGUID device = 0);
  void close();
  bool is_open() const;

  bool start();
  void stop();

  size_t captured_size() const;
  size_t captured_ms() const;

  // Source interface
  virtual Speakers get_output() const;
  virtual bool is_empty() const;
  virtual bool get_chunk(Chunk *chunk);
};

#endif
