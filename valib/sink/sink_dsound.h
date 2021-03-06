/*
  DSoundSink
  ==========

  DirectSound audio renderer (win32)
  Implements PlaybackControl and Sink interfaces.

  PlaybackContol interface must be thread-safe. Therefore we need to serialize
  function calls. To serialize DirectSound usage 'dsound_lock' is used.

  Blocking functions (process() and flush()) cannot take DirectSound lock for a
  long time because it may block the control thread and lead to a deadlock.
  Therefore to serialize playback functions 'playback_lock' is used.

  stop() must force blocking functions to finish. To signal these functions to
  unblock 'ev_stop' is used. Blocking functions must wait on this event and
  stop execution immediately when signaled.
*/

#ifndef VALIB_SINK_DSOUND_H
#define VALIB_SINK_DSOUND_H

#include <dsound.h>
#include <mmreg.h>
#include <ks.h>
#include <ksmedia.h>
#include "../sink.h"
#include "../renderer.h"
#include "../win32/thread.h"

class DSoundSink : public SimpleSink, public PlaybackControl
{
protected:
  /////////////////////////////////////////////////////////
  // Init parameters set by user (must not clear)

  HWND     hwnd;
  LPCGUID  device;        // DirectSound device
  int      buf_size_ms;   // buffer size in ms
  int      preload_ms;    // preload size in ms

  /////////////////////////////////////////////////////////
  // Parameters that do not change on processing

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

  mutable CritSec dsound_lock;
  mutable CritSec playback_lock;
  HANDLE  ev_stop;

  /////////////////////////////////////////////////////////
  // Resource allocation

  bool open_wfx(WAVEFORMATEX *wfx);
  bool try_open(Speakers spk) const;
  bool try_open(WAVEFORMATEX *wf) const;

public:
  //! Processing error exception
  struct Error : public Sink::Error {};

  //! DirectSound error
  struct EDirectSound : public Sink::Error {};

  DSoundSink();
  ~DSoundSink();

  /////////////////////////////////////////////////////////
  // Own interface

  bool open_dsound(HWND _hwnd, int buf_size_ms = 2000, int preload_ms = 500, LPCGUID device = 0);
  void close_dsound();

  /////////////////////////////////////////////////////////
  // Playback control

  virtual void pause();
  virtual void unpause();
  virtual bool is_paused() const;

  virtual void stop();

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

  virtual bool can_open(Speakers new_spk) const;
  virtual bool init();
  virtual void uninit();

  virtual void process(const Chunk &chunk);
  virtual void flush();
};

#endif
