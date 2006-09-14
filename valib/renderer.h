#ifndef VALIB_RENEDERER_H
#define VALIB_RENEDERER_H

#include "vtime.h"

///////////////////////////////////////////////////////////////////////////////
// Interface to control audio playback.
//
// This interface is proposed to control audio playback. Implement it if you
// do audio output and want to control it. Primary purpose is to use it in
// conjunction with Sink interface in objects that implements audio playback.
//
// Playback control functions
// ==========================
//
// pause()
//   Pause audio playback. Do not close anything, just stop the playback. This
//   call may block working thread if called from another thread. It must not
//   block the thread it was called from therefore it may be also called from
//   the working thread.
//
//   It may be called when playback was not actually started. In this case
//   audio output shuold fill output buffer and stop without actually starting
//   the playback. unpause() will actually start playback in this case.
//
//   If pausing is not supported is_paused() must report unpaused state after
//   this call.
//
// unpause()
//   Continue paused playback. Do nothing if playback was not paused.
//
// is_paused()
//   Report about current pause status. If pausing is not it must report
//   unpaused state after pause().
//
// stop()
//   Immediately stop playback and drop buffered data. This function should
//   not block either working nor control threads. If working theread
//  (Sink interface) continues to receive audio data it should proceed and
//  restart audio playback in the normal way. This function may not actually
//  close audio output and free allocated resources. Primary purpose of this
//  function is to drop buffered data and prepare to continue playback of a
//  new stream and/or from a new position.
//
// flush()
//   Wait until all buffered data is played and stop playback. This function
//   blocks until the end of playback. This function may use stop() to finish
//   audio playback. Sink::process() should use this function after receiving
//   eos-chunk. Implementation example:
//
//   void Renderer::flush()
//   {
//     int buffered_ms = (int)(get_buffered_time() * 1000);
//     Sleep(buffered_ms);
//     stop();
//   }
//
//   void Renderer::process(const *Chunk _chunk)
//   {
//     // Do audio output
//     ......
//
//     // Flushing
//     if (_chunk->eos)
//       flush();
//   }
//
// Timing and buffering
// ====================
//
// get_playback_time()
//   Reports about current playback time (position of current playback cursor)
//   in the stream. It should take in account timestamps received with chunks
//   at Sink::process().
//
// get_buffer_size()
// get_buffer_time()
//   Report buffer size in bytes and time units. Must return 0 when object
//   does not do buffering or buffer size is unknown.
//
// get_data_size()
// get_data_time()
//   Report size of buffered data in bytes and time units. Must return 0 when
//   object does not do buffering or buffered data size is unknown.
//
// Volume and pan controls
// ===========================
//
// get_vol()
// set_vol()
//   Control current output volume. Volume is interpreted as amplification 
//   in dB. Therefore 0 is maximum level (no amplification) negative values
//   represent levels less than maximum and positiva valuse represent
//   over-maximum amplification (generally unsupported by most APIs).
//   set_vol() should check the volume value and limit it to supported range.
//   get_vol() reports actual volume level. Therefore if level control is
//   unsupported it should always return 0.
//
// get_pan()
// set_pan()
//   Control panning. Panning value is interpreted as elvel difference between
//   left and right channels in dB. Therefore 0 is no panning, positive values
//   means that left channel(s) is louder, negative values means that right
//   channle(s) is louder
//   set_pan() should check the panning value and limit it to supported range.
//   get_pan() reports actual panning level. Therefore if pan ontrol is
//   unsupported it should always return 0.


class PlaybackControl
{
public:
  PlaybackControl() {};
  virtual ~PlaybackControl() {};

  virtual void pause()     {};
  virtual void unpause()   {};
  virtual bool is_paused() const { return false; };

  virtual void stop()  {};
  virtual void flush() {};

  virtual vtime_t get_playback_time() const { return 0; };

  virtual size_t  get_buffer_size()   const { return 0; };
  virtual vtime_t get_buffer_time()   const { return 0; };
  virtual size_t  get_data_size()     const { return 0; };
  virtual vtime_t get_data_time()     const { return 0; };

  virtual double get_vol()            const { return 0; };
  virtual void   set_vol(double vol)  {};

  virtual double get_pan()            const { return 0; };
  virtual void   set_pan(double pan)  {};
};

#endif
