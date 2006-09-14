#ifndef VALIB_RENEDERER_H
#define VALIB_RENEDERER_H

#include "vtime.h"

// Interface to control audio playback.

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
