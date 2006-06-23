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
#include "win32\thread.h"

class DSoundSink : public Sink, public Clock
{
protected:
  /////////////////////////////////////////////////////////
  // Init parameters set by user (must not clear)

  HWND     hwnd;
  LPCGUID  device;        // DirectSound device
  int      buf_size_ms;   // buffer size in ms
  int      preload_ms;    // preload size in ms

  Clock   *sync_source;   // Clock we must sync with

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

  // This functions report output buffer fullness
  size_t  buffered_size() const;
  vtime_t buffered_time() const;

  // Playback control functions
  // * we should not normally call start() manually because
  //   playback is started automatically after prebuffering
  // * we may call either flush() or stop() to stop playback.
  //   Both functions stops playback but flush() does flushing
  //   and blocks until end of buffered playback when stop() 
  //   stops playback immediately.
  // * flush() is called from process() and stops playback
  //   so normally we may not call flush() before stop()
  // * stop() should be called after the end of porcessing
  //   just to ensure that playback is stopped.
  // * pause() must be called from control thread,
  //   deadlock is possible if called from working thread)
  // * we can pause when playing but cannot unpause when not
  //   playing.

  bool is_playing() const;
  void start();
  void flush();
  void stop();

  bool is_paused() const;
  void pause();
  void unpause();

  /////////////////////////////////////////////////////////
  // Sink interface

  virtual bool query_input(Speakers _spk) const;
  virtual bool set_input(Speakers _spk);
  virtual Speakers get_input() const;
  virtual bool process(const Chunk *_chunk);

  /////////////////////////////////////////////////////////
  // TimeControl interface

  virtual bool is_counting() const;
  virtual vtime_t get_time() const;

  virtual bool can_sync() const;
  virtual void set_sync(Clock *);
};

#endif
