/*
  Abstract parser interface
*/

#ifndef PARSER_H
#define PARSER_H

#include "spk.h"
#include "data.h"

///////////////////////////////////////////////////////////////////////////////
// Parser
//
// Abstract interface for all parsers.
// 
// reset()
//   Resets parser. Parser should go to sync state (may receive new stream or 
//   stream from new position). But also it should be able to report old stream
//   information before new stream frame is loaded.
//
// frame()
//   Loads and decodes frame.
// 
// load_frame()
//   Loads frame into internal frame buffers. It may parse some part of frame 
//   on the fly but it should work fast because it may be used for fast 
//   stream scanning. 
// 
//   Returns frame size afer successful frame load. After successful frame load
//   parser should report new stream information. get_frame() should return 
//   either 0 (if parser does not allow direct access to its frame buffer) or 
//   correct pointer to frame buffer of size reported by get_frame_size().
//   get_samples() result is undefined. decode_frame() call is allowed. 
//
//   Afer load_frame() call resync is always possible, and it is no need to 
//   call reset() to switch on new stream. It is applied for both successful 
//   and unsuccessful frame load.
//
//   Returns 0 if frame is still not loaded or if it was errors during frame
//   load. Parser should report old stream information correctly. get_frame()
//   and get_samples() are underfined. decode_frame() call is prohibited. 
//   Resync is not possible so to terminate frame loading and start to decode
//   stream from other point (or start other stream) it is required to call 
//   reset() first.
//
// decode_frame()
//   Decode frame. Should be called only after successful load_frame() call.
//   All CPU-consuming operations should be done here. After successful 
//   decoding get_samples() returns correct pointers to decoded samples.
//
// get_frame()
//   Should be called only after successful load_frame() call. Returns pointer
//   to raw frame buffer if parser provides access to it. Buffer size is
//   reported by get_frame_size(). 
//
//   Returns 0 if filter does not provide access to frame buffer. 
//   (But get_frame_size() should report correct frame size anyway.)
//
//   Parsers for formats allowed for SPDIF transmission should provide access
//   to raw frame buffer.
//
// get_samples()
//   Should be called only after successful decode_frame() call. Returns
//   pointers to decoded samples buffers. Buffer size is reported by 
//   get_nsamples(). Number of correct buffer pointers returned should be
//   equal to number of channels (reported by get_spk.nch() ).
//
// Following functions may be called asyncronously (and possibly from another
// thread/process) and should be designed carefully. All returned values may 
// change only after successful load_frame() call. Unsuccessful load_frame() 
// call (partially loaded frame or frame load error) should not change 
// anything with 2 exceptions:
// 1) get_info() may be changed in case of errors but it is undesirable
// 2) get_errors() is increased in case of errors
//
// get_spk()         Returns speaker configuration of current stream.
// get_frame_size()  Retruns size of last frame loaded.
// get_nsamples()    Returns number of samples in the last loaded frame.
// get_info()        Detailed text report about the stream.
// get_frames()      Counter of decoded frames.
// get_errors()      Counter of errors happen.
//
///////////////////////////////////////////////////////////////////////////////


class Parser
{
public:
  virtual void reset() = 0;

  // Frame decode
  inline  bool     frame(uint8_t **buf, uint8_t *end);
  virtual unsigned load_frame(uint8_t **buf, uint8_t *end) = 0;
  virtual bool     decode_frame() = 0;

  // Buffers
  virtual uint8_t *get_frame() const = 0;
  virtual samples_t get_samples() const = 0;

  // Stream information.
  virtual Speakers get_spk() const = 0;
  virtual unsigned get_frame_size() const = 0;
  virtual unsigned get_nsamples() const = 0;
  virtual void     get_info(char *buf, unsigned len) const = 0;

  virtual unsigned get_frames() const = 0;
  virtual unsigned get_errors() const = 0;
};

///////////////////////////////////////////////////////////////////////////////
// BaseParser
//
// Provide base functionality common for most of parsers.
//
// Following functions are required to override:
// sync()         Find sync and return frame size
// crc_check()    Checks CRC of fully loaded frame [optional]
// start_decode() Update stream information currently reported and 
//                prepare to decode frame
// decode_frame() Decode frame. This function should check 
//                is_frame_loaded() to proceed correctly and
//                return false otherwise.
//
///////////////////////////////////////////////////////////////////////////////

class BaseParser : public Parser
{
protected:
  // Data buffers
  DataBuf   frame;
  SampleBuf samples;

  unsigned  frame_data;
  unsigned  new_frame_size;
  
  // Stream information currently reported
  Speakers  spk;
  unsigned  frame_size;
  unsigned  nsamples;

  unsigned  frames;
  unsigned  errors;

  // Functions to override
  virtual unsigned sync(uint8_t **buf, uint8_t *end) = 0;
  virtual bool start_decode() = 0;

public:
  BaseParser():
    frame_data(0),
    new_frame_size(0),
    spk(FORMAT_UNKNOWN, MODE_UNDEFINED, 0, 0.0, NO_RELATION), // indicate unknown format
    frame_size(0),
    nsamples(0), 
    frames(0),
    errors(0)
  {
    // ancessor classes may set buffer sizes and other 
    // constant params as needed in constructor
  }

  inline bool is_frame_loaded() const { return frame_data == frame_size; }

  /////////////////////////////////////////////////////////
  // Parser interface

  void reset();

  // Frame decode
  unsigned load_frame(uint8_t **buf, uint8_t *end);

  // Buffers
  uint8_t *get_frame()      const { return frame;      }
  samples_t get_samples()   const { return samples;    }

  // Stream information
  Speakers get_spk()        const { return spk;        }
  unsigned get_frame_size() const { return frame_size; }
  unsigned get_nsamples()   const { return nsamples;   }

  unsigned get_frames()     const { return frames;     }
  unsigned get_errors()     const { return errors;     }
};


inline bool 
Parser::frame(uint8_t **buf, uint8_t *end)
{
  while (*buf < end)
    if (load_frame(buf, end) && decode_frame())
      return true;
  return false;
}

#endif
