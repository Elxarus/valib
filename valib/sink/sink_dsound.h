/*
  DirectSound audio sink (win32)
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

  DWORD    buf_size_ms;   // buffer size in ms
  DWORD    buf_size;      // buffer size in bytes
  DWORD    preload_ms;    // preload size in ms
  DWORD    preload_size;  // preload size in bytes

  DWORD    cur;           // Cursor in sound buffer

  // AudioSink
  Speakers spk;           // Configuration

  bool     playing;       // playing state
  bool     paused;        // paused state
  
  double   vol;           // volume
  double   pan;           // panning

  vtime_t  time;          // time of last sample received

public:
  DSoundSink(HWND hwnd, int buf_size_ms = 2000, int preload_ms = 500);
  ~DSoundSink();

public:
  /////////////////////////////////////////////////////////
  // AudioSink interface

  // Open/close output device
  virtual bool query(Speakers spk) const;
  virtual bool open(Speakers spk);
  virtual void close();
  virtual bool is_open() const;
  virtual Speakers get_spk() const;

  // playback control
  virtual void stop();
  virtual void flush();

  virtual void pause();
  virtual void unpause();
  virtual bool is_paused() const;
 
  // timing
  virtual bool    is_time() const;
  virtual vtime_t get_time() const;

  // volume & pan
  virtual bool   is_vol() const;
  virtual double get_vol() const;
  virtual void   set_vol(double vol);

  virtual bool   is_pan() const;
  virtual double get_pan() const;
  virtual void   set_pan(double pan);

  // data write
  virtual size_t get_buffer_size() const;
  virtual bool   write(const Chunk *chunk);
};

#endif
