/*
  Abstract parser interface
*/

#ifndef PARSER_H
#define PARSER_H

#include "spk.h"
#include "data.h"

struct HeaderInfo;
class HeaderParser;
class FrameParser;

class StreamBuffer;

///////////////////////////////////////////////////////////////////////////////
// HeaderParser
// HeaderInfo
//
// Abstract interface for scanning and detecting compressed stream and header
// information structure.
//
// Sometimes we need to scan the stream without actually decoding. Of course, 
// parser can do this, but we don't need most of its features, buffers, tables,
// etc in this case. For example, application that just detects format of a
// compressed stream will contain all the code required to decode it and create
// unneeded memory buffers. To avoid this we need a lightweight interface to
// work only with frame headers.
//
// Therefore implementation of HeaderParser should be separated from other
// parser parts so we can use it alone. Ideally it should be 2 files (.h and
// .cpp) without other files required.
//
// Header parser is a class without internal state, just a set of functoins.
// So we may create one constant class and use it everywhere. Therefore all
// pointers to HeaderParser objects are const.
//
// Frame parser should return a pointer to a header parser that corresponds
// to the given frame parser.
//
// HeaderParser interface allows nesting, so several header parsers may be
// represented as one parser (see MultiHeader class). Threrfore  instead of 
// list of parsers we can use only one parser pointer everywhere.
//
///////////////////////////////////////////////////////////////////////////////
// HeaderInfo
//
// Syncronization
// ==============
//
// We distinguish 2 stream types:
// * Stream with known frame size. For this strean kind we can determine the
//   frame size by parsing the header. It may be variable bitrate stream with
//   different frame sizes.
// * Stream with unknown frame size (free-format). For this type of stream
//   frame size is unknown from the header. So we must determine inter-frame
//   distance. After that we suppose that frame size is constant and expect
//   next header right after the end of the frame. (Threfore free-format stream
//   is always constant-bitrate.)
//
// Sparse stream is the stream where some gap (or padding) is present in 
// between of two frames. To locate next syncpoint we should scan input data
// after the end of the first frame to locate the next. To limit such scanning
// we should know how much data to scan. Parser can specify the exact amount
// of data to scan after the current frame.
//
// Only known frame size stream is allowed to be sparse (because free-format
// stream is known to have the same frame size). But free-format stream can
// specify scan_size to limit scanning during inter-frame interval detection.
// If scan_size for free-format stream is unspecified we should scan up to 
// max_frame_size.
//
// spk
//   Format of the stream. FORMAT_UNKNOWN indicates a parsing error.
//
// frame_size
//   Frame size (including the header):
//   0  - frame size is unknown (free-format stream)
//   >0 - correct header, frame size is returned
//
// scan_size
//   Use scanning to locate next syncpoint
//   0  - do not use scanning
//   >0 - maximum distance between syncpoints
//
// nsamples
//   Number of samples at the given frame.
//
//   We can derive current bitrate for known frame size stream from frame size,
//   stream format and number of samples: 
//   (*) bitrate = spk.sample_rate * frame_size * 8 / nsamples;
//   But note actual bitrate may be larger for sparce stream.
//
// bs_type
//   Bitstream type. BITSTREAM_XXXX constants.
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
  size_t   scan_size;
  size_t   nsamples;
  int      bs_type;
  uint16_t spdif_type;

  HeaderInfo(): 
    spk(spk_unknown),
    frame_size(0),
    scan_size(0),
    nsamples(0),
    bs_type(0),
    spdif_type(0)
    {};

  inline void drop()
  {
    spk        = spk_unknown;
    frame_size = 0;
    scan_size  = 0;
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
// Frame transformation interface. Frame transformations include:
// * Decoders
// * Loseless frame transformations:
//   * Bitstream format change
//   * Gain change
// * Other frame transformations:
//   * Frame wrappers/unwrappers
//   * Bitrate change (without reencoding, aka transcoding)
//   * etc...
// * Stream validations
// * etc... 
//
// Unlike HeaderParser interface FrameParser has an internal state and must
// receive frames in correct order because frames in a stream are almost always
// related with each other. To prepare parser to receive a new stream and set
// it into initial state we must call reset().
//
// We should reset the transformer with each new stream detected by header
// parser.
//
// All other things are very strightforward: we load a frame with help of
// HeaderParser and give the frame loaded to FrameTransform. FrameTransforn
// does its job and provies access to the transformed data through its
// interface.
//
// FrameParser may do the job in-place at the buffer received with
// parse_frame() call.
//
// header_parser()
//   Returns header parser this tranformer works with.
//
// reset()
//   Set transformer into the initial state. Drop all internal buffers and
//   state variables.
//
// parse_frame()
//   Transform the next frame of a stream.
//   parse_frame() call should not fail if error detected is handled with some
//   kind of restoration procedure that produces some output.
//
// get_spk()
//   Format of a transformed data. FORMAT_UNKNOWN may indicate an error.
//
// get_samples()
//   For linear output format returns pointers to sample buffers.
//   Undefined for non-linear output format.
//
// get_nsamples()
//   For linear output returns numer of samples stored at sample buffers.
//   For non-linear output should indicate number of samples at the output
//   frame or 0 if number of samples is unknown.
//
// get_rawdata()
//   For non-linear output returns pointers to transformed data buffer.
//   For linear output may return a pointer to the original frame data.
//
// get_rawsize()
//   For non-linear output returns size of transformed data.
//   For linear output may return number of bytes actually parsed. It is useful
//   to compact sparse stream.
//
// stream_info()
//   Dump stream information. It may be more informative than header info but
//   should contain only stream-wide information (that does not change from
//   frame to frame).
//
// frame_info()
//   Dump the frame information. Most detailed information for the certain
//   frame. May not include info dumped with stream_info() call.

class FrameParser
{
public:
  FrameParser() {};
  virtual ~FrameParser() {};

  virtual const HeaderParser *header_parser() const = 0;

  /////////////////////////////////////////////////////////
  // Frame transformation

  virtual void reset() = 0;
  virtual bool parse_frame(uint8_t *frame, size_t size) = 0;

  /////////////////////////////////////////////////////////
  // Transformed data access

  virtual Speakers  get_spk() const = 0;

  virtual samples_t get_samples() const = 0;
  virtual size_t    get_nsamples() const = 0;

  virtual uint8_t  *get_rawdata() const = 0;
  virtual size_t    get_rawsize() const = 0;

  /////////////////////////////////////////////////////////
  // Stream information

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
// It is a difference between the size of a frame and frame interval. Frame
// interval is a distance between consecutive syncpoints. Frame size is size of
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
// Internal buffer structure
// =========================
//
// StreamBuffer uses 3-point syncronization. This means that we need buffer
// space for 2 full frames and 1 header more. Alse we need space for a copy
// of a header to load each frame.
//
// +-- header pointer
// V
// +--------------+-----------------------+-----------------------+---------+
// | Header1 copy | Frame1                | Frame2                | Header3 |
// +--------------+-----------------------+-----------------------+---------+
//                ^
//                +-- frame pointer
// 
// And total buffer size equals to:
// buffer_size = max_frame_size * 2 + header_size * 2;
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
// Therefore in spite of the fact that we CAN work with SPDIF stream it is
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

  int  frames;                   // number of frames loaded

  // Pointer to the copy of the header of the last frame loaded.
  // We need this header to load next frame, but frame data of the current
  // frame loaded may be changed by in-place frame processing. Threrfore we
  // have to keep a copy of the header...

  uint8_t *header;               
                                 
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
  void release_parser();

  void reset();
  bool load_frame(uint8_t **data, uint8_t *end);

  bool is_in_sync()             const { return in_sync;       }
  bool is_new_stream()          const { return new_stream;    }
  bool is_frame_loaded()        const { return frame_loaded;  }

  Speakers get_spk()            const { return hdr.spk;        }
  uint8_t *get_frame()          const { return frame;          }
  size_t   get_frame_size()     const { return frame_size;     }
  size_t   get_frame_interval() const { return frame_interval; }

  int  get_frames() const { return frames; }
  size_t stream_info(char *buf, size_t size) const;
  HeaderInfo header_info() const { return hdr; }
};

#endif
