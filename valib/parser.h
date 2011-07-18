/**************************************************************************//**
  \file parser.h
  \brief Abstract parser interface
******************************************************************************/


#ifndef VALIB_PARSER_H
#define VALIB_PARSER_H

#include "buffer.h"
#include "syncscan.h"
#include "spk.h"

struct HeaderInfo;
class HeaderParser;
class StreamBuffer;

/**************************************************************************//**
  \struct HeaderInfo
  \brief Header information structure.

  We distinguish 2 stream types:
  \li Stream with known frame size. For this kind of strean we can determine
    the frame size by parsing the header. Variable bitrate stream with
    different frame sizes is also possible, because size of each frame is known
    form the frame itself.
  \li Stream with unknown frame size (free-format). For this type of stream
    frame size is unknown from the header. So we must determine inter-frame
    distance. After that we suppose that frame size is constant and expect
    next header right after the end of the frame. Threfore, free-format stream
    is always constant-bitrate.

  Sparse stream is the stream where some gap (or padding) is present in 
  between of two frames. To locate next syncpoint we should scan input data
  after the end of the first frame to locate the next. To limit such scanning
  we should know how much data to scan. Parser can specify the exact amount
  of data to scan after the current frame.

  Only known frame size stream is allowed to be sparse (because free-format
  stream is known to have the same frame size). But free-format stream can
  specify scan_size to limit scanning during inter-frame interval detection.
  If scan_size for free-format stream is unspecified we should scan up to 
  max_frame_size.

  \sa HeaderParser

  \var Speakers HeaderInfo::spk;
    Format of the stream. FORMAT_UNKNOWN indicates a parsing error.

  \var size_t HeaderInfo::frame_size;
    Frame size (including the header):
    \li \b 0   frame size is unknown (free-format stream)
    \li \b >0  known frame size

  \var size_t HeaderInfo::scan_size;
    Use scanning to locate next syncpoint
    \li \b 0   do not use scanning
    \li \b >0  maximum distance between syncpoints

  \var size_t HeaderInfo::nsamples;
    Number of samples at the frame.

    We can derive current bitrate for known frame size stream as follows:

    bitrate = spk.sample_rate * frame_size * 8 / nsamples;

    But note that actual bitrate may be larger for sparce stream.

  \var int HeaderInfo::bs_type;
    Bitstream type. BITSTREAM_XXXX constants.

  \var uint16_t HeaderInfo::spdif_type;
    If given format is spdifable it defines spdif packet type (Pc burst-info).
    Zero otherwise. This field may be used to determine spdifable format.

  \fn void HeaderInfo::clear();
    Clear header information.
******************************************************************************/
 
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
    bs_type(BITSTREAM_NONE),
    spdif_type(0)
  {}

  inline void clear()
  {
    spk        = spk_unknown;
    frame_size = 0;
    scan_size  = 0;
    nsamples   = 0;
    bs_type    = BITSTREAM_NONE;
    spdif_type = 0;
  }
};



/**************************************************************************//**
  \class HeaderParser
  \brief Abstract interface for scanning and detecting compressed stream.

  Sometimes we need to scan the stream without actual decoding. Of course, 
  parser can do this, but we don't need most of its features, buffers, tables,
  etc in this case. For example, application that just detects format of a
  compressed stream will contain all the code required to decode it and create
  unneeded memory buffers. To avoid this we need a lightweight interface to
  work only with frame headers.

  Therefore implementation of HeaderParser should be separated from other
  parser parts so we can use it alone. Ideally it should be 2 files (.h and
  .cpp) without other files required.

  Header parser is a class without internal state, just a set of functoins.
  So we may create one constant class and use it everywhere. Therefore all
  pointers to HeaderParser objects must be const.

  Frame parser should return a pointer to a header parser that corresponds
  to the given frame parser.

  HeaderParser interface allows nesting, so several header parsers may be
  represented as one parser (see MultiHeader class). Threrfore  instead of 
  list of parsers we can use only one parser pointer everywhere.

  \sa HeaderInfo

  \fn size_t HeaderParser::header_size() const;
    Minimum number of bytes required to parse header.

  \fn size_t HeaderParser::min_frame_size() const;
    Minimum frame size possible. Must be >= header size.

  \fn size_t HeaderParser::max_frame_size() const;
    Maximum frame size possible. Must be >= minimum frame size.
    Note that for spdifable formats we must take in account maximum spdif
    frame size to be able to parse spdif-padded format.

  \fn bool HeaderParser::can_parse(int format) const;
    \param format Format to test

    Determine that we can parse the format given, or parser can detect
    this format. Example:

    \code
    if (parser.can_parse(FORMAT_AC3))
    {
      // Parser accepts AC3
    }
    \endcode

  \fn bool HeaderParser::parse_header(const uint8_t *hdr, HeaderInfo *info = 0) const = 0;
    \param hdr Pointer to the start of the header
    \param info Optional pointer to HeaderInfo structure that receives info
      about the header given
    \return True when header is successfully parsed and false otherwise.

    Parse header and (optionally) write header information.

    Size of header buffer given must be >= header_size() (it is not verified
    and may lead to memory fault).

    Note, that this method does not provide reliable synchronization, only the
    fast check.

  \fn bool HeaderParser::compare_headers(const uint8_t *hdr1, const uint8_t *hdr2) const;
    \param hdr1 First header
    \param hdr2 Second header
    \return True when both headers belong to the same stream and false
      otherwise.

    Check that both headers belong to the same stream.

    Some compressed formats may determine stream changes with some additional
    header info (not only frame size and stream format). Headers given must be
    correct headers checked with parse() call, so this call may not perform all
    headers checks.

    Note that when headers are equal, format of both headers must be same:

    \code
    parse_header(phdr1, hdr1);
    parse_header(phdr2, hdr2);
    if (compare_headers(phdr1, phdr2))
    {
      assert(hdr1.spk == hdr2.spk);
      assert(hdr1.bs_type == hdr2.bs_type);
      assert(hdr1.spdif_type == hdr2.spdif_type);
      // frame size may differ for variable bitrate
      // nsamples may differ
    }
    \endcode

    Size of header buffers given must be >= header_size() (it is not verified
    and may lead to memory fault).

  \fn string HeaderParser::header_info(const uint8_t *hdr) const;
    \param hdr Header to dump info for.

    Dump stream information that may be useful to track problems. Default
    implementation shows HeaderInfo parameters.

    Size of header buffer given must be >= header_size() (it is not verified
    and may lead to memory fault).
******************************************************************************/

class HeaderParser
{
public:
  HeaderParser() {};
  virtual ~HeaderParser() {};

  virtual SyncTrie sync_trie() const = 0;
  virtual size_t   header_size() const = 0;
  virtual size_t   min_frame_size() const = 0;
  virtual size_t   max_frame_size() const = 0;
  virtual bool     can_parse(int format) const = 0;

  virtual bool     parse_header(const uint8_t *hdr, HeaderInfo *info = 0) const = 0;
  virtual bool     compare_headers(const uint8_t *hdr1, const uint8_t *hdr2) const = 0;
  virtual string   header_info(const uint8_t *hdr) const;
};



/**************************************************************************//**
  \class StreamBuffer
  \brief Stream syncronization and scanning algorithm.

  Implements stream scanning algorithm. This includes reliable syncronization
  algorithm with 3 consecutive syncpoints and frame load algorithn. May be
  used when we need:
  \li reliable stream syncronization
  \li frame-based stream walk

  It is a difference between frame size and frame interval. Frame interval
  is a distance between consecutive syncpoints. Frame size is size of
  frame data. Frame interval may be larger than frame size because of padding
  for example.

  We can parse 2 types of streams: streams with frame size known from the
  header and streams with unknown frame size.

  For known frame size we load only known frame data and all other data is
  skipped. Frame interval is known only after successful frame load and it is
  a distance between *previous* frame header and current frame's header.

  To detect stream changes we search for new stream header after the end of
  the frame loaded. If we cannot find a header of the same stream before
  we reach maximum frame size we do resync.

  For unknown frame size frame interval is a constant determined at stream
  syncronization procedure. We always load the whole frame interval, therefore
  frame size and frame interval are always equal in this case and stream is
  known to be of constant bitrate.

  If header of the same stream is not found exactly after the end of the frame
  loaded we do resync. Therefore this type of the stream cannot contain
  inter-frame debris (see below).
 
  Debris is all data that do not belong to frames. This includes all data that
  that we cannot sync on and all inter-frame data.

  Debris is the mechanism to maintain bit-to-bit correctness, so we may
  construct the original stream back from the parsed one. Also it allows us to
  create stream detector, so we can process uncompressed data in one way and
  compessed in another. For instance, it is required for PCM/SPDIF detection
  and helps to handle SPDIF <-> PCM transitions in a correct way.

  After load() call StreamBuffer may be in following states:

  \verbatim
  syn new deb frm
   -   -   -   -   no sync (not enough data) (1)
   -   -   +   -   no sync with debris output
   +   -   -   -   no frame loaded (not enough data) (1)
   +   -   -   +   frame without debris
   +   -   +   -   inter-frame debris (2)
   +   -   +   +   frame with debris
   +   +   -   +   sync on a new stream
   +   +   +   +   sync on a new stream with debris

  syn - see is_in_sync()
  new - see is_new_stream()
  deb - see has_debris()
  frm - see has_frame()

  (1) - all input buffer data is known to be processed after load() call
  (2) - state is possible but not used in curent implementation.
  \endverbatim

  Prohibited states:

  \verbatim
  syn new deb frm
   -   -   *   +   frame loaded without sync
   -   +   *   *   new stream without sync
   +   +   *   -   new stream detection without a frame loaded

  syn - see is_in_sync()
  new - see is_new_stream()
  deb - see has_debris()
  frm - see has_frame()
  \endverbatim

  load() call returns false in case when stream buffer was not loaded with
  debris or frame data. I.e. in states marked with (1).

  load_frame() skips all debris data and loads only frames. It returns false
  in all states without a frame loaded. So only following states are possible
  after load_frame():

  \verbatim
  syn new frm
   -   -   -   no sync (not enough data) (1)
   +   -   -   frame was not loaded (not enough data) (1)
   +   -   +   frame loaded
   +   +   +   sync on a new stream

  syn - see is_in_sync()
  new - see is_new_stream()
  deb - see has_debris()
  frm - see has_frema()

  (1) - all input buffer is known to be processed after load_frame() call
  \endverbatim

  flush() releases all buffered data as debris. Returns true if it is some
  data to flush.

  \section stream_buffer_internal Internal buffer structure

  StreamBuffer uses 3-point synchronization. This means that we need buffer
  space for 2 full frames and 1 header more. Also, we need some space for a
  copy of a header to load each frame.

  \verbatim
  +-- header pointer
  V
  +--------------+-----------------------+-----------------------+---------+
  | Header1 copy | Frame1                | Frame2                | Header3 |
  +--------------+-----------------------+-----------------------+---------+
                 ^
                 +-- frame pointer
  \endverbatim

  And total buffer size equals to:
  buffer_size = max_frame_size * 2 + header_size * 2;

  <b>Important note!!!</b>

  For unknown frame size and SPDIF stream we cannot load the last frame of
  the stream correctly! (14bit DTS falls into this condition)

  If stream ends after the last frame the last frame is not loaded at all.
  It is because we must load full inter-stream interval but we cannot do this
  because SPDIF header is not transmitted after the last frame:

  \verbatim
                 +------------- frame interval ---------+
                 V                                      V
  +--------------------------------------+--------------------------------------+
  | SPDIF header | Frame1 data | padding | SPDIF header | Frame2 data | padding |
  +--------------------------------------+--------------------------------------+
                                                        ^                       ^
                          Less than frame interval -----+-----------------------+
  \endverbatim

  We can correctly handle switch between different types of SPDIF stream, but
  we cannot correctly handle switch between SPDIF with unknown frame size to
  non-SPDIF stream, and we loose at least one frame of a new stream:

  \verbatim
                 +------------ frame interval --------+
                 V                                    V
  +------------------------------------+------------------------------------+
  | SPDIF header | DTS frame | padding | SPDIF header | AC3 frame | padding |
  +------------------------------------+------------------------------------+
                                                      ^
                                                      +-- correct stream switch
  \endverbatim

  \verbatim
                 +------------ frame interval --------+
                 V                                    V
  +------------------------------------+----------------------------------+
  | SPDIF header | DTS frame | padding | AC3 frame | AC3Frame | AC3 frame |
  +------------------------------------+----------------------------------+
                                       ^                      ^
                                       +---- lost frames -----+--- stream switch
  \endverbatim

  Therefore in spite of the fact that we CAN work with SPDIF stream directly, it
  is recommended to demux SPDIF stream before working with the contained stream.

  We can demux SPDIF stream correctly because SPDIF header contains real
  frame size of the contained stream.

  \name Init

  \fn void StreamBuffer::set_parser(const HeaderParser *parser)
    \param parser Header parser

    Defines the header parser to use for stream parsing. Passing null is
    equivalent to release_parser();

  \fn const HeaderParser *StreamBuffer::get_parser() const
    Returns currently used header parser.

  \fn void StreamBuffer::release_parser()
    Forgets the parser set with set_parser().

  \name Processing

  \fn void StreamBuffer::reset()
    Reset internal buffers and prepare to receive a new stream.

  \fn bool StreamBuffer::load(uint8_t **data, uint8_t *end)
    \param data Pointer to the beginning of the input buffer
    \param end Pointer to the end of the input buffer

    Load next piece of data (frame or debris). When data was loaded, it returns
    true and moves \c data pointer. Otherwise it returns false and moves
    \c data pointer to the end of the input buffer.

    When buffer is loaded, you can:
    \li determine the kind of data buffered with help of has_frame() and
        has_debris()
    \li get access to frame buffer with get_frame() and get_frame_size()
    \li get access to debris buffer with get_debris() and get_debris_size()
    \li determine the format of the frame loaded with get_spk()

    Frame and debris buffers are allowed to be modified inplace. So you are not
    obligated to copy data away from these buffers for inplace processing.

    \code
      StreamBuffer stream(header_parser);
      // .....
      while (stream.load(data, end))
      {
        if (stream.has_debris())
          // It is safe to modify debris buffer
          memset(stream.get_debris(), stream.get_debris_size(), 0);

        if (stream.has_frame())
          // It is safe to modify frame buffer
          bs_convert(
            stream.get_frame(), stream.get_frame_size(), stream.header_info().bs_type,
            stream.get_frame(), stream.get_frame_size(), BITSTREAM_8);
      }
    \endcode

    You can reconstruct the original stream \b exactly. To do this, place the
    debris buffer before the frame buffer. The following code shows how to
    parse a file and write an exact copy.

    \code
      AutoFile in(input_file_name);
      AutoFile out(output_file_name, "wb");
      Rawdata buf(buf_size);
      StreamBuffer stream(&ac3_header);

      uint8_t *buf_pos = buf.begin();
      uint8_t *buf_end = buf.begin();
      while (!in.eof() || buf_pos < buf_end)
      {
        if (buf_pos >= buf_end)
        {
          size_t read_size = in.read(buf.begin(), buf.size());
          buf_pos = buf.begin();
          buf_end = buf_pos + read_size;
        }

        while (stream.load(&buf_pos, buf_end))
        {
          if (stream.has_debris())
            out.write(stream.get_debris(), stream.get_debris_size());
          if (stream.has_frame())
            out.write(stream.get_frame(), stream.get_frame_size());
        }
      }

      // StreamBuffer may have some data buffered
      if (stream.flush())
      {
        if (stream.has_debris())
          out.write(stream.get_debris(), stream.get_debris_size());
        if (stream.has_frame())
          out.write(stream.get_frame(), stream.get_frame_size());
      }
    \endcode

  \fn bool StreamBuffer::load_frame(uint8_t **data, uint8_t *end)
    \param data Pointer to the beginning of the input buffer
    \param end Pointer to the end of the input buffer

    Loads a next frame. When the frame is loaded successfully, it returns true
    and moves the data pointer. Otherwise, it returns false and moves the
    pointer to the end of the input buffer. See load() for more info.

    This function is more convinient than load() when you need frame data only.

    Note, that you cannot reconstruct the stream exactly because inter-frame
    data may be lost.

  \fn bool StreamBuffer::flush()
    At the end of the stream processing some data may be still buffered. To be
    able to perfectly reconstruct the stream we must fetch this data too. This
    function makes the buffered data available with get_frame() and
    get_debris().

    Returns true when data is available and false otherwise.

  \name State flags

  \fn bool StreamBuffer::is_in_sync() const
    Returns true when synchronization sequence is found. The format of the
    stream is available using get_spk(). Both frame and debris output is
    possible.

    This flag becomes false when sync is lost (or still not found).

  \fn bool StreamBuffer::is_new_stream() const
    Returns true when new synchronization on a new stream was done. A frame
    of a new format is available immediately.

    Returns false when stream continues without change or not in sync.

  \fn bool StreamBuffer::has_frame() const
    Returns true when a frame is available. In this case get_frame() returns
    the pointer to the beginning of the frame and get_frame_size() returns
    non-zero frame size.

  \fn bool StreamBuffer::has_debris() const
    Returns true when non-frame data is available. In this case get_debris()
    returns the pointer to the beginning of the data and get_debris_size()
    returns non-zero data size.

  \name Data access

  \fn const uint8_t * StreamBuffer::get_buffer() const
    Returns pointer to the internal synchronization buffer. Not for use in most
    cases!

  \fn size_t StreamBuffer::get_buffer_size() const
    Returns the size of the data at the internal buffer. Not for use in most
    cases!

  \fn uint8_t * StreamBuffer::get_debris() const
    Returns pointer to non-frame data when has_debris() is true and null
    pointer otherwise. You can modify this buffer directly for inplace
    processing.

  \fn size_t StreamBuffer::get_debris_size() const
    Returns the size of non-frame data when has_debris() is true and zero
    otherwise. You can use this value directly instead of has_debris().

  \fn Speakers StreamBuffer::get_spk() const
    Returns the format of the stream when is_in_sync() returns true and
    FORMAT_UNKNOWN otherwise.

  \fn uint8_t * StreamBuffer::get_frame() const
    Returns pointer to frame data when has_frame() is true and null pointer
    otherwise. You can modify this buffer directly for inplace processing.

  \fn size_t StreamBuffer::get_frame_size() const
    Returns the size of frame data when has_frame() is true and zero otherwise.
    You can use this value directly instead of has_debris().

  \fn size_t StreamBuffer::get_frame_interval() const
    Returns inter-frame interval.

  \fn int StreamBuffer::get_frames() const
    Returns the total number of frames loaded since construction.

  \fn string StreamBuffer::stream_info() const
    Prints stream information.

  \fn HeaderInfo StreamBuffer::header_info() const
    Prints current frame information.

******************************************************************************/

class StreamBuffer
{
protected:
  // Parser info (constant)

  const HeaderParser *parser;    //!< header parser
  size_t header_size;            //!< cached header size
  size_t min_frame_size;         //!< cached min frame size
  size_t max_frame_size;         //!< cached max frame size

  // Buffers
  // We need a header of a previous frame to load next one, but frame data of
  // the frame loaded may be changed by in-place frame processing. Therefore
  // we have to keep a copy of the header. So we need 2 buffers: header buffer
  // and sync buffer. Header buffer size is always header_size.

  Rawdata  buf;

  uint8_t *header_buf;           //!< header buffer pointer
  HeaderInfo hinfo;              //!< header info (parsed header buffer)

  uint8_t *sync_buf;             //!< sync buffer pointer
  size_t   sync_size;            //!< size of the sync buffer
  size_t   sync_data;            //!< size of data loaded at the sync buffer
  size_t   pre_frame;            //!< amount of pre-frame data allowed
                                 //!< (see comment to sync() function)

  // Data (frame data and debris)

  uint8_t   *debris;             //!< pointer to the start of debris data
  size_t     debris_size;        //!< size of debris data

  uint8_t   *frame;              //!< pointer to the start of the frame
  size_t     frame_size;         //!< size of the frame loaded
  size_t     frame_interval;     //!< frame interval

  // Flags

  bool in_sync;                  //!< we're in sync with the stream
  bool new_stream;               //!< frame loaded belongs to a new stream
  int  frames;                   //!< number of frames loaded

  inline bool load_buffer(uint8_t **data, uint8_t *end, size_t required_size);
  inline void drop_buffer(size_t size);
  bool sync(uint8_t **data, uint8_t *data_end);

public:
  StreamBuffer();
  StreamBuffer(const HeaderParser *hparser);
  virtual ~StreamBuffer();

  /////////////////////////////////////////////////////////
  // Init

  void set_parser(const HeaderParser *parser);
  const HeaderParser *get_parser() const { return parser; }
  void release_parser();

  /////////////////////////////////////////////////////////
  // Processing

  void reset();
  bool load(uint8_t **data, uint8_t *end);
  bool load_frame(uint8_t **data, uint8_t *end);
  bool flush();

  /////////////////////////////////////////////////////////
  // State flags

  bool is_in_sync()             const { return in_sync;         }
  bool is_new_stream()          const { return new_stream;      }
  bool has_frame()              const { return frame_size  > 0; }
  bool has_debris()             const { return debris_size > 0; }

  /////////////////////////////////////////////////////////
  // Data access

  const uint8_t *get_buffer()   const { return sync_buf;       }
  size_t   get_buffer_size()    const { return sync_data;      }

  uint8_t *get_debris()         const { return debris;         }
  size_t   get_debris_size()    const { return debris_size;    }

  Speakers get_spk()            const { return hinfo.spk;      }
  uint8_t *get_frame()          const { return frame;          }
  size_t   get_frame_size()     const { return frame_size;     }
  size_t   get_frame_interval() const { return frame_interval; }

  int get_frames() const { return frames; }
  string stream_info() const;
  HeaderInfo header_info() const { return hinfo; }
};

#endif
