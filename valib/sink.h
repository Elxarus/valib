/*
  Abstract audio output device (renderer) interface
  NullRenderer implementation
*/

#ifndef SINK_H
#define SINK_H

#include "spk.h"
#include "filter.h"

class AudioRenderer;
class NullRenderer;


///////////////////////////////////////////////////////////////////////////////
// AudioRenderer class
//
// Abstract base for audio playback devices.
//
// Open/close output device
// ========================
//
// query() [thread-safe, fast]
//   Check if we can open audio output with given format right now. Returns 
//   true if we can and false if we can't. Audio device may have some internal
//   state that may prevent opening. For example only one SPDIF audio output
//   is allowed and we cannot open second SPDIF output until closing of 
//   previous. Device state may be changed between query() and open() calls, 
//   so it is always nessesery to check result of open() call.
//   This call should work as fast as possible, because it may be used to try 
//   numerous formats to find acceptable conversion. Also, this function may
//   be called asynchronously from other thread, so it should be thread-safe.
//
// open() [working thread, blocking]
//   Open audio output and allocate nessesary resources. Returns true on 
//   success and false otherwise. It may require some time to initialize the 
//   device and allocate resources.
//
// close() [working thread, blocking]
//   Close audio output and free all resources. It may require some time to do
//   it.
//
// is_open() [working thread]
//   Check open status.
//
// Playback control
// ================
//
// stop() [working thread]
//   Stop playback immediately and drop buffered data. Playback will continue 
//   after receiving of new data (if device is not in paused state).
//
// flush() [working thread, blocking]
//   Play until the end of buffered data and stop. This function blocks the
//   thread until the end of playback. Playback will continue after receiving
//   of new data (if device is not in paused state).
//
// pause() [thread-safe]
//   Pause playback. Unlike stop() call playback will not ever continue until
//   unpause() call.
//
// unpause() [thread-safe]
//   Unpause playback. Playback will continue from the point when pause() was
//   called (if it was no stop() call after pause() call).
//
// is_paused() [thread-safe]
//   Check paused status.
//
// Timing
// ======
//
// is_time() [thread-safe]
//   Device may track timing information so it may act as time generator.
//   If this call returns true then following timing information is correct.
//   Otherwise all timing information is fully based on input timestamps.
//
// get_time() [thread-safe]
//   Current playback time. If is_time() is true then this is the real ouptut
//   time. If is_time() is false then call returns just last timestamp 
//   received.
//
// Volume & panning
// ================
//
// [following functions should be thread-safe]
//
// is_vol()  - device supports volume control
// set_vol() - set volume [0..1]
// get_vol() - returns current volume
//
// is_pan()  - device supports panning control
// set_pan() - set panning [-1..1]
// get_pan() - returns current panning
//
// Data write
// ==========
//
// get_buffer_size() [thread-safe]
//   Free buffer size in bytes/samples. 0 means that device does not support 
//   buffering. This call should return full buffer size after open(), 
//   stop() and flush() calls. 
//
// write() [working thread, blocking, critical path]
//   Write data block to device. This function blocks until all data is
//   writted to playback buffer. If device is in paused state and playback 
//   buffer is full this call also blocks. This call should not block if free
//   playback buffer size >= size of received data and format is unchanged.
//

class AudioRenderer : public Sink
{
public:
  /////////////////////////////////////////////////////////
  // AudioRenderer interface

  // Open/close output device
  virtual bool query(Speakers spk) const = 0;
  virtual bool open(Speakers spk) = 0;
  virtual void close() = 0;
  virtual bool is_open() const = 0;
  virtual Speakers get_spk() const = 0;

  // playback control
  virtual void stop() = 0;
  virtual void flush() = 0;

  virtual void pause() = 0;
  virtual void unpause() = 0;
  virtual bool is_paused() const = 0;
 
  // timing
  virtual bool is_time() const = 0;
  virtual vtime_t get_time() const = 0;

  // volume & pan
  virtual bool   is_vol() const = 0;
  virtual double get_vol() const = 0;
  virtual void   set_vol(double vol) = 0;

  virtual bool   is_pan() const = 0;
  virtual double get_pan() const = 0;
  virtual void   set_pan(double pan) = 0;

  // data write
  virtual size_t get_buffer_size() const = 0;
  virtual bool   write(const Chunk *chunk) = 0;

protected:
  /////////////////////////////////////////////////////////
  // Sink interface

  virtual bool query_input(Speakers _spk) const
  {
    return query(_spk);
  }

  virtual Speakers get_input() const
  {
    return get_spk();
  }

  virtual bool set_input(Speakers _spk)
  {
    if (is_open())
    {
      if (_spk != get_spk())
      {
        flush();
        return open(_spk);
      }
      else
        return true;
    }
    else
      return open(_spk);
  }

  virtual bool process(const Chunk *_chunk)
  {
    if (_chunk->spk == spk_unknown)
      return true;

    return write(_chunk);
  }
};


class NullRenderer : public AudioRenderer
{
protected:
  Speakers spk;

  bool opened;
  bool paused;

  double vol;
  double pan;

  vtime_t time;

  inline bool receive_chunk(const Chunk *_chunk)
  {
    // Open device
    if (!opened && !open(_chunk->spk))
      return false;

    // Format change
    if (spk != _chunk->spk)
    {
      if (opened)
        flush();
      
      if (!open(_chunk->spk))
        return false;
    }

    // Synchronization
    if (_chunk->sync)
      time = _chunk->time;

    return true;
  }

public:
  NullRenderer()
  {
    spk    = spk_unknown;

    opened = false;
    paused = false;

    vol    = 1.0;
    pan    = 0;

    time   = 0;
  }
   
  // Open/close device
  virtual bool query(Speakers _spk) const 
  {
    return true;
  }

  virtual bool open(Speakers _spk)
  {
    opened = query(spk);

    if (opened)
      spk = _spk;

    return opened;
  }

  virtual void close()                { opened = false; }
  virtual bool is_open() const        { return opened;  }
  virtual Speakers get_spk() const    { return spk;     }

  // Playback control
  virtual void stop()                 { /* does nothing */ }
  virtual void flush()                { /* does nothing */ }

  virtual void pause()                { paused = true;  }
  virtual void unpause()              { paused = false; }
  virtual bool is_paused() const      { return paused;  }

  // Timing
  virtual bool    is_time() const     { return false;   }
  virtual vtime_t get_time() const    { return time;    }

  // Volume & panning
  virtual bool   is_vol() const       { return false;   }
  virtual double get_vol() const      { return vol;     }
  virtual void   set_vol(double _vol) { vol = _vol;     }

  virtual bool   is_pan() const       { return false;   }
  virtual double get_pan() const      { return pan;     }
  virtual void   set_pan(double _pan) { pan = _pan;     }

  // data write
  virtual size_t get_buffer_size() const
  {
    return (size_t)(-1);
  }

  virtual bool write(const Chunk *_chunk)
  {
    // todo: block on pause
    return receive_chunk(_chunk);
  }
};

#endif
