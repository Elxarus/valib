/*
  DirectSound audio renderer (win32)
*/

#ifndef SINK_DS_H
#define SINK_DS_H

#include <dsound.h>
#include <ks.h>
#include <ksmedia.h>
#include "filter.h"
#include "vtime.h"
#include "renderer.h"
#include "win32\thread.h"

class DSoundSink : public Sink, public PlaybackControl
{
protected:
  /////////////////////////////////////////////////////////
  // Init parameters set by user (must not clear)

  HWND     hwnd;
  LPCGUID  device;        // DirectSound device
  int      buf_size_ms;   // buffer size in ms
  int      preload_ms;    // preload size in ms

  /////////////////////////////////////////////////////////
  // Parameters that depend on initialization
  // (do not change on processing)

  Speakers spk;           // playback format
  DWORD    buf_size;      // buffer size in bytes
  DWORD    preload_size;  // preload size in bytes
  double   bytes2time;    // factor to convert bytes to seconds

  /////////////////////////////////////////////////////////
  // DirectSound

  IDirectSound        *ds;
  IDirectSoundBuffer  *ds_buf;

  /////////////////////////////////////////////////////////
  // Processing parameters

  DWORD    cur;           // cursor in sound buffer
  vtime_t  time;          // time of last sample received
  bool     playing;       // playing state
  bool     paused;        // paused state

  /////////////////////////////////////////////////////////
  // Threading

  CritSec  lock;

  /////////////////////////////////////////////////////////
  // Resource allocation

  void zero_all();
  bool open(Speakers spk);
  bool open(WAVEFORMATEX *wfx);
  bool try_open(Speakers spk) const;
  bool try_open(WAVEFORMATEX *wf) const;
  void close();

public:
  DSoundSink(HWND hwnd, int buf_size_ms = 2000, int preload_ms = 500, LPCGUID device = 0);
  ~DSoundSink();

  /////////////////////////////////////////////////////////
  // Own interface

  bool init(int buf_size_ms = 2000, int preload_ms = 500, LPCGUID device = 0);

  /////////////////////////////////////////////////////////
  // Playback control

  virtual void pause();
  virtual void unpause();
  virtual bool is_paused() const;

  virtual void stop();
  virtual void flush();

  virtual vtime_t get_playback_time() const;

  virtual size_t  get_buffer_size()   const;
  virtual vtime_t get_buffer_time()   const;
  virtual size_t  get_data_size()     const;
  virtual vtime_t get_data_time()     const;

  virtual double get_vol()            const;
  virtual void   set_vol(double vol);

  virtual double get_pan()            const;
  virtual void   set_pan(double pan);

  /////////////////////////////////////////////////////////
  // Sink interface

  virtual bool query_input(Speakers _spk) const;
  virtual bool set_input(Speakers _spk);
  virtual Speakers get_input() const;
  virtual bool process(const Chunk *_chunk);
};

#endif
