/*
  Abstract parser interface
*/

#ifndef PARSER_H
#define PARSER_H

#include "spk.h"
#include "data.h"
#include "syncscan.h"
#include "bitstream.h"

struct HeaderInfo;
class HeaderParser;
class FrameParser;

class StreamBuffer;
class ParserBuffer;

///////////////////////////////////////////////////////////////////////////////
// HeaderParser
// HeaderInfo
//
// Abstract interface for scanning and detecting compressed stream and header
// information structure.
//
// Sometimes we need to scan the stream without actually decoding. Of course, 
// parser can do this, but we don't need most of its features, buffers, tables,
// etc in this case. For example, application that just detects format of
// compressed stream will contain all code required to decode it and create
// unneeded memory buffers. To avoid this we need a lightweight interface to
// work only with frame headers.
//
// Therefore implementation of HeaderParser should be separated from other
// parser parts so we can use it alone. Ideally it should be 2 files (.h and
// .cpp) without other files required.
//
// Header parser is a class without internal state, just a set of functoins.
// Therefore we may create one constant class and use it everywhere. For
// example frame parser should return header parser that corresponds to given
// frame parser.
//
// General syncronization scan procedure:
// * load enough data to parse header (header_size() call)
// * check that this syncword points to correct frame header (frame_size() call)
// * determine frame size and find next syncword after (frame_size() call)
// * ensure that both syncwords belong to the same stream (compare_headers())
//
///////////////////////////////////////////////////////////////////////////////
// HeaderInfo
//
// spk
//   Format of the stream. If header was parsed correctly it must not be equal
//   to FORMAT_UNKNOWN.
//
// frame_size
//   Frame size (including the header):
//   0  - frame size is unknown (free format stream)
//   >0 - correct header, frame size is returned
//
// nsamples
//   Number of samples at the given frame.
//
//   We can derive current bitrate from frame size, stream format and number of
//   samples: bitrate = spk.sample_rate * frame_size * 8 / nsamples;
//
// bs_type
//   Bitstream type. BITSTREAM_XXXX constants (see bitstream.h).
//
// spdif_type
//   If given format is spdifable it defines spdif packet type (Pc burst-info).
//   Zero otherwise. This field may be used to determine spdifable format.
//
///////////////////////////////////////////////////////////////////////////////
// HeaderParser
//
// header_size()
//   Minimum number of bytes required to parse header.
//
// min_frame_size()
//   Minimum frame size possible. Must be >= header size.
//
// max_frame_size()
//   Maximum frame size possible. Must be >= minimum frame size.
//   Note that for spdifable formats we must take in account maximum spdif
//   frame size to be able to parse spdif-padded format.
//
// can_parse()
//   Deternine that we can parse the format given. (Or, that parser can detect
//   this format). For example if some_parser.can_parse(FORMAT_AC3) == true
//   this parser can parse ac3 format, or can detect in in raw data.
//   
// parse_header()
//   Parse header and write header information.
//
//   Size of header buffer given must be >= header_size() (it is not verified
//   and may lead to memory fault).
//
// compare_headers()
//   Veryfy that both headers belong to the same stream. Some compressed formats
//   may determine stream changes with some additional header info (not only
//   frame size and stream format). Given headers must be correct headers
//   (checked with frame_size() call). This call may not do all headers checks.
//   Note that when headers are equal, format of both headers must be same:
//
//   parse_header(phdr1, hdr1);
//   parse_header(phdr2, hdr2);
//   if (compare_headers(phdr1, phdr2)
//   {
//     assert(hdr1.spk == hdr2.spk);
//     assert(hdr1.bs_type == hdr2.bs_type);
//     assert(hdr1.spdif_type == hdr2.spdif_type);
//     // frame size may differ for variable bitrate
//     // nsamples - ?
//   }
//
//   Size of header buffers given must be >= header_size() (it is not verified
//   and may lead to memory fault).
//
// header_info()
//   Dump stream information that may be useful to track problems. Default
//   implementation shows HeaderInfo parameters.
//
//   Size of header buffer given must be >= header_size() (it is not verified
//   and may lead to memory fault).

struct HeaderInfo
{
  Speakers spk;
  size_t   frame_size;
  size_t   nsamples;
  int      bs_type;
  uint16_t spdif_type;

  HeaderInfo(): 
    spk(spk_unknown),
    frame_size(0),
    nsamples(0),
    bs_type(0),
    spdif_type(0) {};

  inline void drop()
  {
    spk        = spk_unknown;
    frame_size = 0;
    nsamples   = 0;
    bs_type    = BITSTREAM_NONE;
    spdif_type = 0;
  }
};

class HeaderParser
{
public:
  HeaderParser() {};
  virtual ~HeaderParser() {};

  virtual size_t   header_size() const = 0;
  virtual size_t   min_frame_size() const = 0;
  virtual size_t   max_frame_size() const = 0;
  virtual bool     can_parse(int format) const = 0;

  virtual bool     parse_header(const uint8_t *hdr, HeaderInfo *h = 0) const = 0;
  virtual bool     compare_headers(const uint8_t *hdr1, const uint8_t *hdr2) const = 0;
  virtual size_t   header_info(const uint8_t *hdr, char *buf, size_t size) const;
};


///////////////////////////////////////////////////////////////////////////////
// FrameParser
//
// Interface for frame decoding. Unlike HeaderParser interface FrameParser has
// an internal state and must receive frames in correct order because frames in
// a stream are almost always related with each other. To prepare parser to
// receive a new stream and set it into initial state we must call reset().
// We should reset frame parser with new stream detected by header parser.
//
// All other things are very strightforward: we load a frame with help of
// HeaderParser and give the frame loaded to FrameParser. FrameParser decodes
// it and stores decoded data at internal buffer accessible through its
// interface.
//
// parse_frame() should not fail with all detected stream errors if error
// detected is handled with a kind of restoration procedure that produces some
// output.
//
// header_parser()
//   Returns header parser corresponding to this parser
//
// reset()
//   Set parser into initial state. Drop all internal buffers and state
//   variables.
//
// parse_frame()
//   Parse next frame of a stream and load output buffer.
//
// get_spk()
//   Format of decoded stream. Must be correct linear format after successful
//   parse_frame() call.
//
// get_samples()
//   Returns pointers to samples.
//
// get_nsamples()
//   Numer of samples stored at samples buffer.
//
// stream_info()
//   Dump stream information. It may be more informative than header info but
//   should contain only stream-wide information (that does not change from
//   frame to frame).
//
// frame_info()
//   Dump frmae information. Most detailed information for sertain frame. May
//   not include info dumped with stream_info() call.

class FrameParser
{
public:
  FrameParser() {};
  virtual ~FrameParser() {};

  virtual const HeaderParser *header_parser() const = 0;

  virtual void reset() = 0;
  virtual bool parse_frame(uint8_t *frame, size_t size) = 0;

  virtual Speakers  get_spk() const = 0;
  virtual samples_t get_samples() const = 0;
  virtual size_t    get_nsamples() const = 0;
  virtual uint8_t  *get_rawdata() const = 0;
  virtual size_t    get_rawsize() const = 0;

  virtual size_t stream_info(char *buf, size_t size) const = 0;
  virtual size_t frame_info(char *buf, size_t size) const = 0;
};



///////////////////////////////////////////////////////////////////////////////
// StreamBuffer
//
// Implements stream scanning algorithm. This includes reliable syncronization
// algorithm with 3 consecutive syncpoints and frame load algorithn. May be
// used when we need:
// * reliable stream syncronization
// * frame-based stream walk
//
// It is a difference between the size of frame and frame interval. Frame
// intercal is a distance between consecutive syncpoints. Frame size is size of
// frame data. Frame interval may be larger than frame size because of SPDIF
// padding for example.
//
// We can parse 2 types of streams: streams with frame size known from the
// header and streams with unknown frame size.
//
// For known frame size we load only known frame data and all other data is
// skipped. Frame interval is known only after successful frame load and it is
// a distance between *previous* frame header and current frame's header.
//
// To detect stream changes we search for new stream header after the end of
// the frame loaded. If we cannot find a header of the same stream before
// we reach maximum frame size we do resync.
//
// For unknown frame size frame interval is a constant determined at stream
// syncronization procedure. We always load all frame interval data therefore
// frame size and frame interval are always equal in this case and stream is
// known to be of constant bitrate.
//
// If header of the same stream is not found exactly after the end of loaded
// frame we do resync.
//
// Important note!!!
// =================
//
// For unknown frame size and SPDIF stream we cannot load the last frame of
// the stream correctly!
//
// If stream ends after the last frame the last frame is not loaded at all.
// It is because we must load full inter-stream interval but we cannot do this
// because SPDIF header is not transmitted after the last frame:
//
//                +------------- frame interval ---------+
//                V                                      V
// +--------------------------------------+--------------------------------------+
// | SPDIF header | Frame1 data | padding | SPDIF header | Frame2 data | padding |
// +--------------------------------------+--------------------------------------+
//                                                       ^                       ^
//                         Less than frame interval -----+-----------------------+
//
// We can correctly handle switch between different types of SPDIF stream, but
// we cannot correctly handle switch between SPDIF with unknown frame size to
// non-SPDIF stream (we loose at least one frame of a new stream):
//
//                +------------ frame interval --------+
//                V                                    V
// +------------------------------------+------------------------------------+
// | SPDIF header | DTS frame | padding | SPDIF header | AC3 frame | padding |
// +------------------------------------+------------------------------------+
//                                                     ^
//                                                     +-- correct stream switch
//
//                +------------ frame interval --------+
//                V                                    V
// +------------------------------------+----------------------------------+
// | SPDIF header | DTS frame | padding | AC3 frame | AC3Frame | AC3 frame |
// +------------------------------------+----------------------------------+
//                                      ^                      ^
//                                      +---- lost frames -----+--- stream switch
//
// Therefroe in spite of the fact that we CAN work with SPDIF stream it is
// recommended to demux SPDIF stream before working with contained stream.
// We can demux SPDIF stream correctly because SPDIF header contains real
// frame size of the contained stream.

class StreamBuffer
{
protected:
  DataBuf       buf;

  // Header-related info

  const HeaderParser *parser;    // header parser
  HeaderInfo    hdr;             // header info

  size_t        header_size;     // cached header size
  size_t        min_frame_size;  // cached min frame size
  size_t        max_frame_size;  // cached max frame size

  // Flags

  bool in_sync;                  // we're in sync with the stream
  bool new_stream;               // frame loaded belongs to a new stream
  bool frame_loaded;             // frame is loaded

  // Frame-related info

  uint8_t *frame;                // pointer to the start of the frame buffer
  size_t   frame_data;           // data loaded to the frame buffer
  size_t   frame_size;           // size of the frame loaded
  size_t   frame_interval;       // frame interval

  bool sync(uint8_t **data, uint8_t *data_end);

public:
  StreamBuffer();
  StreamBuffer(const HeaderParser *hparser);
  virtual ~StreamBuffer();

  bool set_parser(const HeaderParser *parser);
  const HeaderParser *get_parser() const { return parser; }

  void reset();
  bool load_frame(uint8_t **data, uint8_t *end);

  bool is_in_sync()             const { return in_sync;      }
  bool is_new_stream()          const { return new_stream;   }
  bool is_frame_loaded()        const { return frame_loaded; }

  Speakers get_spk()            const { return hdr.spk;        }
  uint8_t *get_frame()          const { return frame;          }
  size_t   get_frame_size()     const { return frame_size;     }
  size_t   get_frame_interval() const { return frame_interval; }

  const HeaderInfo &header_info() const { return hdr; }

  size_t   stream_info(char *buf, size_t size) const;
};












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
  virtual ~Parser() {};

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
  virtual int      get_info(char *buf, size_t len) const = 0;

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
