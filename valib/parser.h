/**************************************************************************//**
  \file parser.h
  \brief Abstract parser interface
******************************************************************************/

#ifndef VALIB_PARSER_H
#define VALIB_PARSER_H

#include "buffer.h"
#include "syncscan.h"
#include "spk.h"


struct SyncInfo;
struct FrameInfo;

class FrameParser;
class BasicFrameParser;

class StreamBuffer;


/**************************************************************************//**
  \struct SyncInfo
  \brief Synchronization information.

  Basic idea of synchronization is to search a certain pattern at the given
  range. Syncword of a next audio frame must appear at the range between
  minimum and maximum frame sizes. This structure holds this information
  required for synchronization. This information is also independent of
  the algorithm used for synchronization. You may use it to scan directly with
  help of SyncTrie::is_sync(), use SyncScan that uses some tricks to scan
  faster, or use StreamBuffer to use an advanced 3-point synchronization and
  frame loading.

  It must be noted that we may need data that exceeds the range because of the
  size of the syncword. Here is an illustration of the frame of the max size:

  \verbatim
  first syncword          next syncword
  V                       V
  +-----------------------+----+
   <--- max_frame_size --> <-->
                           syncword size
  \endverbatim

  The syncword size may be obtained with sync_trie.sync_size().

  Correct SyncInfo must be fully defined, i.e. contain non-empty trie, min and
  max frame sizes. (In fact, trie must not degenerate with SyncTrie::optimize(), 
  but this cannot be checked fast). Correctness may be checked with
  is_good() function. Default constructor creates an incorrect
  SyncInfo. clear() makes the structure incorrect in a correct way :).

  Minimum frame size must be >= 1 because scanning starts from old_syncpoint +
  min_frame_size. Endless cycle is possible when min_frame_size is zero.

  When min_frame_size == max_frame_size, scanning is not nessesary because
  frame size is constant and known explicitly. This important special case may
  be tested with const_frame_size() function.

  Two SyncInfo's may be combined together with combime() function or
  with 'or' operator (|).

  \code
    SyncInfo s, s1, s2, s3, s4;

    ...
    s1.combine(s2);
    s1 |= s3;
    s = s1 | s4;

    if (s.is_sync(data)) // match any pattern s1-s4
    {
      ...
    }
  \endcode

  \var SyncTrie SyncInfo::sync_trie;
    Synchronization pattern.

  \var size_t SyncInfo::min_frame_size;
    Minimum frame size.

  \var size_t SyncInfo::max_frame_size;
    Maximum frame size.

  \fn SyncInfo::SyncInfo()
    Constructs an invalid SyncInfo.

  \fn SyncInfo::SyncInfo(const SyncTrie &sync_trie, size_t min_frame_size, size_t max_frame_size):
    \param sync_trie      Synchronization pattern
    \param min_frame_size Minimum frame size
    \param max_frame_size Maximum frame size

    Construct a fully-defined SyncInfo. Validity depends on the data given.

  \fn void SyncInfo::clear()
    Make the structure incorrect.

  \fn bool SyncInfo::is_good() const
    Returns true when SyncInfo is fully defined and correct.

  \fn bool SyncInfo::const_frame_size() const
    Returns true when frame size is constant.

  \fn void SyncInfo::combine(const SyncInfo &other)
    Combine this structure with another. The resulting SyncInfo may be used to
    search for both types of syncpoints.

******************************************************************************/

struct SyncInfo :
  boost::orable<SyncInfo>
{
  SyncTrie sync_trie;
  size_t min_frame_size;
  size_t max_frame_size;

  SyncInfo(): min_frame_size(0), max_frame_size(0)
  {}

  SyncInfo(const SyncTrie &sync_trie_, size_t min_frame_size_, size_t max_frame_size_):
  sync_trie(sync_trie_), min_frame_size(min_frame_size_), max_frame_size(max_frame_size_)
  {}

  void clear()
  {
    sync_trie.clear();
    min_frame_size = 0;
    max_frame_size = 0;
  }

  bool is_good() const
  {
    return !sync_trie.is_empty() &&
           min_frame_size > 0 &&
           max_frame_size >= min_frame_size;
  }

  bool const_frame_size() const
  {
    return min_frame_size > 0 && min_frame_size == max_frame_size;
  }

  void combine(const SyncInfo &other)
  {
    if (!is_good())
      *this = other;
    else if (other.is_good())
    {
      sync_trie |= other.sync_trie;
      if (other.min_frame_size < min_frame_size)
        min_frame_size = other.min_frame_size;
      if (other.max_frame_size > max_frame_size)
        max_frame_size = other.max_frame_size;
    }
  }
 
  SyncInfo &operator |=(const SyncInfo &other)
  { combine(other); return *this; }
};



/**************************************************************************//**
  \struct FrameInfo
  \brief Audio frame information.

  This structure holds the most important information about an audio frame.

  The information may be partially filled, therefore you have to check it
  for correctness before use.

  \var Speakers FrameInfo::spk;
    Format of the stream. FORMAT_UNKNOWN if not filled.

  \var size_t FrameInfo::frame_size;
    Frame size. Zero if not filled.

  \var size_t FrameInfo::nsamples;
    Number of samples at the frame. Zero if not filled.

  \var int FrameInfo::bs_type;
    Bitstream type. BITSTREAM_NONE if not filled.

  \var uint16_t FrameInfo::spdif_type;
    If given format is spdifable it defines spdif packet type (Pc burst-info).
    Zero otherwise. This field may be used to determine spdifable format.

  \fn void FrameInfo::clear();
    Clear all information.

  \fn size_t FrameInfo::bitrate() const;
    Find the bitrate of the stream. Zero if information required is not filled.

******************************************************************************/

struct FrameInfo
{
  Speakers spk;
  size_t   frame_size;
  size_t   nsamples;
  int      bs_type;
  uint16_t spdif_type;

  FrameInfo(): 
    spk(spk_unknown),
    frame_size(0),
    nsamples(0),
    bs_type(BITSTREAM_NONE),
    spdif_type(0)
  {}

  void clear()
  {
    spk = Speakers();
    frame_size = 0;
    nsamples = 0;
    bs_type = 0;
    spdif_type = 0;
  }

  size_t bitrate() const
  {
    return nsamples? frame_size * 8 * spk.sample_rate / nsamples: 0;
  }
};



/**************************************************************************//**
  \class FrameParser
  \brief Abstract interface for frame operations.

  This class allows to separate synchronization algorithms from the
  format-depedent code. It consists of a set of data and atomic operations
  required for synchronization. Interface is designed to be lightweight and
  contain only elementary and fast operations. Implementation should not
  contain large buffers, big tables and other heavy decoder/encoder stuff.

  We use the following terminology:
  - \b Syncpoint (synchronization point) is a point at the stream that matches
    certain pattern (defined by SyncTrie). It's a beginning of the frame and
    <b>frame header</b>. Syncpoint that does not start a frame is a
    <b>false syncpoint</b>.

  - <b>Frame header</b> is a set of parameters placed at the beginning of the
    frame that allows to determine frame characteristics. Header validation
    also allows to decrease false-sync probability. Includes synchronization
    word. In some cases header does not allow to determine frame
    characteristics.

  The task of synchronization is to divide a byte stream into a sequence of
  frames of the same format.

  To find a frame we may search for 2 consequtive syncpoints:

  \verbatim
        p1                     p2      p3       p4
        V                      V       V        V
  ------+------------------------------+-------------
        | hdr1 |      frame 1          | hdr2 |  frame 2
  ------+------------------------------+-------------
  \endverbatim

  But second syncpoint (p2) may appear at the middle of the frame (false sync).
  Header validation is the first of the methods to exclude p2 and avoid false
  sync.
  
  If first header (hdr1) contains the frame size we can check the placement of
  the second syncpoint directly:

  \verbatim
        p1                     p2      p3       p4
        V                      V       V        V
  ------+------------------------------+-------------
        | hdr1 |      frame 1          | hdr2 |  frame 2
  ------+------------------------------+-------------
        |                              ^
        +- - - - - - - - - - - - - - - +
                frame_size (hdr1)
  \endverbatim

  This allows to exclude p2 and p4 and ensure that p1 is a correct syncpoint.
  An example of false sync at p1 and good sync at p2:

  \verbatim
        p1                 p2                    p3
        V                  V                     V
  ------+------------------+---------------------+------------------
        | false hdr |      | hdr1 |  frame 1     | hdr2 |  frame 2
  ------+------------------+---------------------+------------------
        |                              ^         ^
        +- - - - - - - - - - - - - - - +         |
              frame_size (false hdr)             |
                                                 |
                           |                     |
                           +- - - - - - - - - - -+
                              frame_size (hdr1)
  \endverbatim

  At last, we must ensure that consecutive frames belong to the same stream
  and have the same format. For example, the following does not form a correct
  audio stream:

  \verbatim
        p1                             p2
        V                              V
  ------+------------------------------+------------------
        | hdr1 (mono) |                | hdr2 (stereo) |
  ------+------------------------------+------------------
        |                              ^
        +- - - - - - - - - - - - - - - +
                frame_size (hdr1)
        p1                             p2
        V                              V
  ------+------------------------------+------------------
        | hdr1 (ac3) |                 | hdr2 (dts) |
  ------+------------------------------+------------------
        |                              ^
        +- - - - - - - - - - - - - - - +
                frame_size (hdr1)
  \endverbatim

  So, it should be a way to compare headers.

  As the result, we have 3 basic operations:
  - Syncpoint matching. Done with help of SyncInfo provided by sync_info()
    function.
  - Header validation and parsing. Done by parse_header() function.
  - Header comparison. Done by compare_headers() function.

  All methods may be divided into several groups:
  - Synchronization info.
  - Stateless frame header functions. These functions operate only on frame
    header. Stateless, but may not provide accurate info in some cases.
  - Stateful frame operations. Validate whole frames and track the sequence of
    frames.

  Frame parsers may be divided into:
  - Frame information is fully known from the header. In this case all
    information about a frame (including the frame size) is known from the
    frame header. In this case we don't have to scan for the next frame.
  - Frame size unknown from the header. In this case frame size is not known
    from the header.
    - Constant frame size.
    - Variable frame size.

  \name Synchronization info

  \fn bool FrameParser::can_parse(int format) const = 0;
    \param format Format to test

    Determine that we can parse the format given:

    \code
    if (parser.can_parse(FORMAT_AC3))
    {
      // Parser accepts AC3
    }
    \endcode

  \fn SyncInfo FrameParser::sync_info() const = 0;
    Returns synchronization info.

  \name Frame header operations

  \fn size_t FrameParser::header_size() const = 0;
    Minimum amount of data in bytes required to parse a frame header.

  \fn bool FrameParser::parse_header(const uint8_t *hdr, FrameInfo *finfo = 0) const = 0;
    \param hdr Pointer to the start of the header
    \param finfo Optional pointer to FrameInfo structure that receives information
      about the header given
    \return True when header is successfully parsed and false otherwise.

    Parse header and (optionally) write header information.

    Size of header buffer given must be >= header_size() (it is not verified
    and may lead to memory fault).

    Note, that this method does not provide reliable synchronization, only the
    fast check.

  \fn bool FrameParser::compare_headers(const uint8_t *hdr1, const uint8_t *hdr2) const = 0;
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
    parser.parse_header(hdr1, &finfo1);
    parser.parse_header(hdr2, &finfo2);
    if (compare_headers(hdr1, hdr2))
    {
      assert(finfo1.spk == finfo2.spk);
      assert(finfo1.bs_type == finfo2.bs_type);
      assert(finfo1.spdif_type == finfo2.spdif_type);
      // frame size may differ for variable bitrate
      // nsamples may differ
    }
    \endcode

    Size of header buffers given must be >= header_size() (it is not verified
    and may lead to memory fault).

  \name Frame operations

  \fn bool FrameParser::first_frame(const uint8_t *frame, size_t size) = 0;
  \fn bool FrameParser::next_frame(const uint8_t *frame, size_t size) = 0;
  \fn void FrameParser::reset() = 0;

  \fn bool FrameParser::in_sync() const = 0;
  \fn SyncInfo FrameParser::sync_info2() const = 0;
  \fn FrameInfo FrameParser::frame_info() const = 0;
  \fn string FrameParser::stream_info() const = 0;

******************************************************************************/

class FrameParser
{
public:
  FrameParser() {}
  virtual ~FrameParser() {}

  // Synchronization info
  virtual bool      can_parse(int format) const = 0;
  virtual SyncInfo  sync_info() const = 0;

  // Frame header operations
  virtual size_t    header_size() const = 0;
  virtual bool      parse_header(const uint8_t *hdr, FrameInfo *finfo = 0) const = 0;
  virtual bool      compare_headers(const uint8_t *hdr1, const uint8_t *hdr2) const = 0;

  // Frame operations
  virtual bool      first_frame(const uint8_t *frame, size_t size) = 0;
  virtual bool      next_frame(const uint8_t *frame, size_t size) = 0;
  virtual void      reset() = 0;

  virtual bool      in_sync() const = 0;
  virtual SyncInfo  sync_info2() const = 0;
  virtual FrameInfo frame_info() const = 0;
  virtual string    stream_info() const = 0;
};

class BasicFrameParser : public FrameParser
{
public:
  BasicFrameParser() {}

  virtual bool      first_frame(const uint8_t *frame, size_t size);
  virtual bool      next_frame(const uint8_t *frame, size_t size);
  virtual void      reset();

  virtual bool      in_sync() const { return !finfo.spk.is_unknown(); }
  virtual SyncInfo  sync_info2() const { return in_sync()? sinfo: sync_info(); }
  virtual FrameInfo frame_info() const { return finfo; }
  virtual string    stream_info() const;

protected:
  virtual SyncInfo build_syncinfo(const uint8_t *frame, size_t size, const FrameInfo &finfo) const
  { return sync_info(); }

  Rawdata   header;
  SyncInfo  sinfo;
  FrameInfo finfo;
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

    Note, that you may have to call this function several times to flush all
    the data buffered. When StreamBuffer have no more data to flush it goes
    out of sync (i.e. is_in_sync() returns false). Or you may call flush()
    until it returns false.

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
  // Parser info
  // (1) set at parser change
  // (2) set after successful synchronization
  // (3) changes each frame

  FrameParser *parser;           //!< frame parser
  size_t    header_size;         //!< header size (1)
  SyncInfo  sinfo;               //!< synchronization info (1)
  SyncScan  scan;                //!< syncpoint scanner (1)

  size_t    const_frame_size;    //!< frame size if constant, zero otherwise (2)
  FrameInfo finfo;               //!< last frame info (3)

  // Buffers
  // We need a header of a previous frame to load next one, but frame data of
  // the frame loaded may be changed by in-place frame processing. Therefore
  // we have to keep a copy of the header. So we need 2 buffers: header buffer
  // and sync buffer. Header buffer size is always header_size.

  Rawdata  buf;
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

  // Flags

  bool in_sync;                  //!< we're in sync with the stream
  bool new_stream;               //!< frame loaded belongs to a new stream
  int  frames;                   //!< number of frames loaded

  inline bool load_buffer(uint8_t **data, uint8_t *end, size_t required_size);
  inline void drop_buffer(size_t size);

  void resync();
  bool sync(uint8_t **data, uint8_t *data_end);

public:
  StreamBuffer();
  StreamBuffer(FrameParser *parser);
  virtual ~StreamBuffer();

  /////////////////////////////////////////////////////////
  // Init

  void set_parser(FrameParser *parser);
  const FrameParser *get_parser() const { return parser; }
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
  bool need_flushing()          const { return sync_data > 0;   }

  /////////////////////////////////////////////////////////
  // Data access

  const uint8_t *get_buffer()   const { return sync_buf;       }
  size_t   get_buffer_size()    const { return sync_data;      }

  uint8_t *get_debris()         const { return debris;         }
  size_t   get_debris_size()    const { return debris_size;    }

  Speakers get_spk()            const { return finfo.spk;      }
  uint8_t *get_frame()          const { return frame;          }
  size_t   get_frame_size()     const { return frame_size;     }

  FrameInfo frame_info() const { return finfo; }
  int get_frames() const { return frames; }
  string stream_info() const;
};

#endif
