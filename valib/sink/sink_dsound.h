/*
  DirectSound audio sink
  (win32)
*/

#ifndef SINK_DS_H
#define SINK_DS_H

#include <dsound.h>
#include <ks.h>
#include <ksmedia.h>
#include "sink.h"
#include "win32\thread.h"

// uncomment the following string to create primary audio buffer
//#define DSOUND_SINK_PRIMARY_BUFFER

class DSoundSink : public AudioSink
{
protected:
  IDirectSound        *ds;
#ifdef DSOUND_SINK_PRIMARY_BUFFER
  IDirectSoundBuffer  *ds_prim;
#endif
  IDirectSoundBuffer  *ds_buf;
  WAVEFORMATEXTENSIBLE wfx;

  HWND     hwnd;
  CritSec  lock;

  DWORD    buf_size_ms;
  DWORD    buf_size;
  DWORD    preload_ms;
  DWORD    preload_size;
  DWORD    cur;     // Cursor in sound buffer
  time_t   time;

  Speakers spk;

  bool     playing;
  bool     paused;
  
  double   vol;
  double   pan;

public:
  DSoundSink(HWND hwnd, int buf_size_ms = 2000, int preload_ms = 500);
  ~DSoundSink();

  // playback control
  virtual bool query(Speakers spk) const;
  virtual bool open(Speakers spk);
  virtual void close();
  virtual bool is_open() const;

  virtual void pause();
  virtual void unpause();
  virtual bool is_paused();

  virtual void flush();

  // timing
  virtual time_t get_output_time();
  virtual time_t get_playback_time();
  virtual time_t get_lag_time();

  // volume & pan
  void   set_vol(double vol);
  double get_vol();
  void   set_pan(double pan);
  double get_pan();

  // data write
  bool can_write_immediate(const Chunk *chunk);
  bool write(const Chunk *chunk);
};

#endif
