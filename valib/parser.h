/*
  Abstract parser interface
*/

#ifndef PARSER_H
#define PARSER_H

#include "spk.h"
#include "data.h"
#include "syncscan.h"
#include "bitstream.h"

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
//   equal to the number of channels reported by get_spk.nch().
//
// Following functions may be called asyncronously (and possibly from another
// thread/process) and should be designed carefully. All returned values may 
// change only after successful load_frame() call. Unsuccessful load_frame() 
// call (partially loaded frame or frame load error) should not change 
// anything with 2 exceptions:
// 1) get_info() may be changed in case of errors but it is undesirable
// 2) get_errors() is increased in case of errors
//
// get_spk()         Returns speaker configuration of compresed stream.
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

  // Frame load operations
  virtual size_t load_frame(uint8_t **buf, uint8_t *end) = 0;
  virtual bool   is_frame_loaded() const = 0;
  virtual void   drop_frame() = 0;

  // Frame decode
  inline  bool   frame(uint8_t **buf, uint8_t *end);
  virtual bool   decode_frame() = 0;

  // Buffers
  virtual uint8_t *get_frame() const = 0;
  virtual samples_t get_samples() const = 0;

  // Stream information.
  virtual Speakers get_spk() const = 0;
  virtual unsigned get_frame_size() const = 0;
  virtual unsigned get_nsamples() const = 0;
  virtual void     get_info(char *buf, size_t len) const = 0;

  virtual unsigned get_frames() const = 0;
  virtual unsigned get_errors() const = 0;
};

///////////////////////////////////////////////////////////////////////////////
// BaseParser
//
// Provide base functionality common for most of parsers.
//
// Following functions are required to override:
//
// header_size()  
//   Minimum data size required to decode header. Used by load_frame() to load
//   nessesary data for load_header() function.
//
// load_header()
//   Decode stream info and return frame size. Used by load_frame() to ensure
//   that frame can be loaded with given syncpoint. It must fill 'spk', 
//   'frame_size', 'nsamples' and 'bs_type' fields with correct values.
//   Returns true on successful header check and false otherwise.
//
// prepare() (placeholder)
//   Called after successful frame load to prepare decoder. Here we can parse
//   additional information, do some initializations, etc. But we must do it 
//   as fast as possible because load_frame() may be used for fast stream 
//   scanning so only publically avalable data should be loaded here (for 
//   example BSI info for ac3). Returns true on success and false otherwise.
//
// crc_check() (placeholder)
//   Called to check crc of loaded frame. Only called if do_crc flag is set.
//   You may leave this function empty and implement crc check on frame decode
//   (use the same do_crc flag to determine if crc check is required).
//   This function is used at load_frame() so it allows error detection at an
//   earlier stage (before decoding).
//
// decode_frame() Decode frame. This function should check is_frame_loaded()
//   to proceed correctly and return false otherwise.
//
///////////////////////////////////////////////////////////////////////////////

class BaseParser : public Parser
{
protected:
  // Data buffers
  DataBuf   frame;
  SampleBuf samples;

  size_t    frame_data;

  // Syncronization
  SyncScan  scanner;

  // Stream information currently reported
  // must be filled by load_header()
  Speakers  spk;
  size_t    frame_size;
  size_t    nsamples;
  int       bs_type;

  size_t    frames;
  size_t    errors;
/*
  // functions to override
  virtual size_t header_size() const = 0;
  virtual size_t frame_size(uint8_t *hdr) const = 0;
  virtual bool   compare_headers(uint8_t *hdr1, uint8_t *hdr2) const = 0;
  virtual bool   load_header() = 0;
*/
  // Functions to override
  virtual size_t header_size() const = 0;
  virtual bool   load_header(uint8_t *_buf) = 0;
  virtual bool   prepare() { return true; };
  virtual bool   crc_check() { return true; };

public:
  bool do_crc;

  BaseParser():
    frame_data(0),
    spk(spk_unknown), // indicate unknown format
    frame_size(0),
    nsamples(0), 
    bs_type(0),
    frames(0),
    errors(0),
    do_crc(true)
  {
    ///////////////////////////////////////////////////////
    // Descendant class must:
    // * Set buffer size for frame buffer.
    //   It must be larger or equal to the largest frame 
    //   size possible (used by load_frame()).
    // * Initialize syncronization scanner.
    //
    // Descendant class may:
    // * Set buffer size for samples buffer
    //   (or it may set it on decoding)
    // * Set constant stream info variables
    //   (for ac3 nsamples = 1536)
  }

  /////////////////////////////////////////////////////////
  // Parser interface

  void reset()
  {
    frame_data = 0;
  };

  // Frame load operations
  size_t load_frame(uint8_t **buf, uint8_t *end);
  bool   is_frame_loaded()  const { return frame_size && (frame_data >= frame_size); }
  void   drop_frame()             
  { 
    if (frame_data)
    {
      frame_data -= frame_size;
      memmove(frame, frame + frame_size, frame_data);
    }
  }

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
  if (is_frame_loaded())
    drop_frame();

  while (*buf < end)
    if (load_frame(buf, end) && decode_frame())
      return true;

  return false;
}

#endif
