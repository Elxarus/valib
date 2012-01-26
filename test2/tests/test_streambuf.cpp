/*
  Streambuf class test

  StreamBuffer supports 3 modes of operation:
  * Const frame size: FrameParser::sync_info2() returns equal min_frame_size
    and max_frame_size. The fastest frame loading algorithm.
  * Header frame size: FrameParser::parse_header() returns non-zero frame size.
    Not too much slower than const frame size algorithm.
  * Unknown frame size: uses scannikng to find next syncpoint. Much slower
    than other algorithms.

  To test this modes 3 mock frame parsers are used: ConstFrameSize,
  HeaderFrameSize, UnknownFrameSize. All are based on MPAFrameParser and must
  be fed with CBR mp2 files.

  Passthrough test
  ================

  This test ensures that we load frames correctly and stream constructed back
  of frames loaded and debris is eqal to the original stream.

  Also this test checks correct frame loading after inplace frame processing.
  To simulate such processing we zap the frame buffer after comparing.

  We also count and check number of streams and frames in a file.

  Notes
  -----
  * we load the whole file into memory
  * DTS parser can work with both raw DTS and SPDIF/DTS, but cannot load the
    last frame of SPDIF/DTS stream (see notes for StreamBuffer class)
*/

#include "auto_file.h"
#include "../noise_buf.h"

#include "parsers/uni/uni_frame_parser.h"
#include <boost/test/unit_test.hpp>

static const int seed = 958745023;
static const int noise_size = 1*1024*1024;

///////////////////////////////////////////////////////////////////////////////
// Mock parsers

class ConstFrameSize : public MPAFrameParser
{
public:
  ConstFrameSize(): frame_size(0)
  {}

  virtual void reset()
  {
    MPAFrameParser::reset();
    frame_size = 0;
  }

  virtual bool first_frame(const uint8_t *frame, size_t size)
  {
    if (!MPAFrameParser::first_frame(frame, size))
      return false;

    frame_size = size;
    sinfo.min_frame_size = size;
    sinfo.max_frame_size = size;
    return true;
  }

  virtual FrameInfo frame_info() const
  {
    assert(finfo.frame_size == frame_size);
    return finfo;
  }

protected:
  size_t frame_size;
};

class HeaderFrameSize : public MPAFrameParser
{
public:
  HeaderFrameSize()
  {}

  virtual bool parse_header(const uint8_t *hdr, FrameInfo *finfo = 0) const
  {
    FrameInfo temp_finfo;
    bool result = MPAFrameParser::parse_header(hdr, &temp_finfo);
    if (!result) return false;
    if (temp_finfo.frame_size == 0) return false;
    if (finfo) *finfo = temp_finfo;
    return true;
  }
};

class UnknownFrameSize : public MPAFrameParser
{
public:
  UnknownFrameSize()
  {}

  virtual bool parse_header(const uint8_t *hdr, FrameInfo *finfo = 0) const
  {
    bool result = MPAFrameParser::parse_header(hdr, finfo);
    if (result && finfo)
      finfo->frame_size = 0;
    return result;
  }

  virtual bool first_frame(const uint8_t *frame, size_t size)
  {
    FrameInfo temp_finfo;
    bool result = MPAFrameParser::parse_header(frame, &temp_finfo);
    if (!temp_finfo.frame_size || temp_finfo.frame_size != size)
      return false;
    return MPAFrameParser::first_frame(frame, size);
  }

  virtual bool next_frame(const uint8_t *frame, size_t size)
  {
    FrameInfo temp_finfo;
    bool result = MPAFrameParser::parse_header(frame, &temp_finfo);
    if (!temp_finfo.frame_size || temp_finfo.frame_size != size)
      return false;
    return MPAFrameParser::next_frame(frame, size);
  }
};

// Frame parser that returns frame size > max frame size.
// Endless loop was possible in this case.
class BadFrameParser : public MPAFrameParser
{
public:
  BadFrameParser()
  {}

  virtual SyncInfo sync_info() const { return SyncInfo(sync_trie, 32, 32); }
  virtual SyncInfo sync_info2() const { return sync_info(); }
};

///////////////////////////////////////////////////////////////////////////////
// Sync test
// Sync on a stream starting at some distance from the start of the buffer.
// The size of the junk before the stream equals to junk_size. Feed the parser
// with chunks of size chunk_size.

static void sync_test(FrameParser *parser, uint8_t *buf, size_t buf_size, size_t junk_size, size_t chunk_size)
{
  // setup pointers
  uint8_t *ptr = buf;
  uint8_t *end = buf + buf_size;
  uint8_t *ref_ptr = buf;

  uint8_t *frame, *debris;
  size_t frame_size, debris_size;

  StreamBuffer streambuf(parser);
  while (ptr < buf + buf_size && !streambuf.is_in_sync())
  {
    if (chunk_size)
    {
      end = ptr + chunk_size;
      if (end > buf + buf_size)
        end = buf + buf_size;
    }

    while (ptr < end)
    {
      // process data
      streambuf.load(&ptr, end);

      // check debris
      if (streambuf.has_debris())
      {
        debris      = streambuf.get_debris();
        debris_size = streambuf.get_debris_size();
        if (memcmp(debris, ref_ptr, debris_size))
          BOOST_FAIL("Debris check failed at pos = " << ref_ptr - buf);
        ref_ptr += debris_size;
      }

      // check frame & junk size
      if (streambuf.has_frame())
      {
        frame       = streambuf.get_frame();
        frame_size  = streambuf.get_frame_size();
        if (memcmp(frame, ref_ptr, frame_size))
          BOOST_FAIL("Frame check failed at pos = " << ref_ptr - buf);

        if (ref_ptr - buf != junk_size)
          BOOST_CHECK_EQUAL(ref_ptr - buf, junk_size);

        ref_ptr += frame_size;
        break;
      }
    }
  }

  if (ref_ptr > end)
    BOOST_FAIL("Frame ends after the end of the reference file");
}

///////////////////////////////////////////////////////////////////////////////
// Passthrough test

static void passthrough_test(FrameParser *parser, uint8_t *buf, size_t buf_size, int file_streams, int file_frames)
{
  // setup pointers
  uint8_t *ptr = buf;
  uint8_t *end = buf + buf_size;
  uint8_t *ref_ptr = buf;

  uint8_t *frame, *debris;
  size_t frame_size, debris_size;

  int frames = 0;
  int streams = 0;
  StreamBuffer streambuf(parser);

  while (ptr < end || streambuf.need_flushing())
  {
    // process data
    if (ptr < end)
      streambuf.load(&ptr, end);
    else
      streambuf.flush();

    // count streams & frames
    if (streambuf.is_new_stream())  streams++;
    if (streambuf.has_frame()) frames++;

    debris      = streambuf.get_debris();
    debris_size = streambuf.get_debris_size();
    frame       = streambuf.get_frame();
    frame_size  = streambuf.get_frame_size();

    // Check debris
    if (memcmp(debris, ref_ptr, debris_size))
      BOOST_FAIL("Debris check failed at pos = " << ref_ptr - buf << " frame = " << frames);
    ref_ptr += debris_size;

    // Check frame
    if (memcmp(frame, ref_ptr, frame_size))
      BOOST_FAIL("Frame check failed at pos = " << ref_ptr - buf << " frame = " << frames);
    ref_ptr += frame_size;

    // zap the frame buffer to simulate in-place processing
    memset(frame, 0, frame_size);

    if (ref_ptr > end)
      BOOST_FAIL("Frame ends after the end of the reference file");
  }

  BOOST_CHECK_MESSAGE(ref_ptr == end, "Reference is longer than output");

  // Check stream and frame counters
  if (file_streams) BOOST_CHECK_EQUAL(streams, file_streams);
  if (file_frames)  BOOST_CHECK_EQUAL(frames, file_frames);
}

///////////////////////////////////////////////////////////////////////////////

BOOST_AUTO_TEST_SUITE(stream_buffer)

BOOST_AUTO_TEST_CASE(constructor)
{
  StreamBuffer buf;

  BOOST_CHECK(buf.get_parser() == 0);

  BOOST_CHECK(!buf.is_in_sync());
  BOOST_CHECK(!buf.is_new_stream());
  BOOST_CHECK(!buf.has_frame());
  BOOST_CHECK(!buf.has_debris());
}

BOOST_AUTO_TEST_CASE(init_constructor)
{
  AC3FrameParser frame_parser;
  StreamBuffer buf(&frame_parser);

  BOOST_CHECK(buf.get_parser() == &frame_parser);

  BOOST_CHECK(!buf.is_in_sync());
  BOOST_CHECK(!buf.is_new_stream());
  BOOST_CHECK(!buf.has_frame());
  BOOST_CHECK(!buf.has_debris());
}

BOOST_AUTO_TEST_CASE(set_parser)
{
  AC3FrameParser frame_parser1;
  DTSFrameParser frame_parser2;

  MultiFrameParser bad_parser; // MultiHeader without parsers set
  StreamBuffer buf;

  buf.set_parser(&bad_parser);
  BOOST_CHECK(buf.get_parser() == 0);

  buf.set_parser(&frame_parser1);
  BOOST_CHECK_EQUAL(buf.get_parser(), &frame_parser1);

  buf.set_parser(&frame_parser2);
  BOOST_CHECK_EQUAL(buf.get_parser(), &frame_parser2);

  buf.release_parser();
  BOOST_CHECK(buf.get_parser() == 0);
}

// Test operation without parser set
BOOST_AUTO_TEST_CASE(null_parser)
{
  RawNoise noise(noise_size, seed);
  uint8_t *begin = 0, *end = noise.begin() + noise.size();
  bool result;

  StreamBuffer buf;

  buf.reset();

  begin = noise.begin();
  result = buf.load(&begin, end);
  BOOST_CHECK(!result);
  BOOST_CHECK_EQUAL(begin, end);

  begin = noise.begin();
  result = buf.load_frame(&begin, end);
  BOOST_CHECK(!result);
  BOOST_CHECK_EQUAL(begin, end);

  result = buf.flush();
  BOOST_CHECK(!result);
}

///////////////////////////////////////////////////////////////////////////////
// Test synchrinization
// Some data may appear before the first synchpoint. Some combinations of
// pre-synch junk and chunk size may lead to sync problems. To test this,
// try different sizes of the junk data before the start of the stream.

BOOST_AUTO_TEST_CASE(sync)
{
  ConstFrameSize   const_frame_size;
  HeaderFrameSize  header_frame_size;
  UnknownFrameSize unknown_frame_size;
  UniFrameParser   uni;

  const struct {
    const char *filename;
    const char *parser_name;
    FrameParser *parser;
  } parsers[] = {
    // Mock parsers
    { "a.mp2.002.mp2",   "ConstFrameSize",   &const_frame_size   },
    { "a.mp2.002.mp2",   "HeaderFrameSize",  &header_frame_size  },
    { "a.mp2.002.mp2",   "UnknownFrameSize", &unknown_frame_size },
    // Real parsers
    { "a.aac.03f.adts",     "ADTSFrameParser",  &uni.adts  },
    { "a.ac3.005.ac3",      "AC3FrameParser",   &uni.ac3   },
    { "test.eac3.03f.eac3", "EAC3FrameParser",  &uni.eac3  },
    { "a.dts.03f.dts",      "DTSFrameParser",   &uni.dts   },
    { "a.mp2.002.mp2",      "MPAFrameParser",   &uni.mpa   },
    { "a.ac3.03f.spdif",    "SPDIFFrameParser", &uni.spdif },
    // MultiFrameParser
    { "a.aac.03f.adts",     "UniFrameParser",   &uni },
    { "a.ac3.005.ac3",      "UniFrameParser",   &uni },
    { "test.eac3.03f.eac3", "UniFrameParser",   &uni },
    { "a.dts.03f.dts",      "UniFrameParser",   &uni },
    { "a.mp2.002.mp2",      "UniFrameParser",   &uni },
  };

  for (size_t iparser = 0; iparser < array_size(parsers); iparser++)
  {
    const size_t header_size = parsers[iparser].parser->header_size();

    const size_t chunk_size[] = {
      0,                     // the whole buffer at once
      1, 2, 3, 5,            // very small chunks
      512, 1024, 4096, 8192, // large chunks
      // Chunk sizes around the header size
      header_size, header_size + 1, header_size - 1
    };

    const size_t junk_size[] = {
      0, 1, 2, 3, 4, 5,
      // Junk sizes around the header size
      header_size / 2, header_size * 2,
      header_size, header_size + 1, header_size - 1
    };

    RawNoise data;
    MemFile file(parsers[iparser].filename);
    BOOST_MESSAGE("Sync test " << parsers[iparser].parser_name << " " << parsers[iparser].filename);
    BOOST_REQUIRE(file);

    for (size_t ijunk_size = 0; ijunk_size < array_size(junk_size); ijunk_size++)
      for (size_t ichunk = 0; ichunk < array_size(chunk_size); ichunk++)
      {
        data.allocate(junk_size[ijunk_size] + file.size());
        data.rng.fill_raw(data, junk_size[ijunk_size]);
        memcpy(data + junk_size[ijunk_size], file, file.size());
        sync_test(parsers[iparser].parser, 
          data, junk_size[ijunk_size] + file.size(),
          junk_size[ijunk_size], chunk_size[ichunk]);
      }
  }
}

BOOST_AUTO_TEST_CASE(file_passthrough)
{
  ConstFrameSize   const_frame_size;
  HeaderFrameSize  header_frame_size;
  UnknownFrameSize unknown_frame_size;
  UniFrameParser   uni;

  struct {
    const char *filename;
    const char *parser_name;
    FrameParser *parser;
    int streams;
    int frames;
  } parsers[] = {
    // Mock parsers
    { "a.mp2.mix.mp2", "ConstFrameSize",   &const_frame_size,   3, 1500 },
    { "a.mp2.mix.mp2", "HeaderFrameSize",  &header_frame_size,  3, 1500 },
    { "a.mp2.mix.mp2", "UnknownFrameSize", &unknown_frame_size, 3, 1500 },

    // Real parsers
    { "a.aac.03f.adts",     "ADTSFrameParser",  &uni.adts,1, 564  },
    { "a.ac3.005.ac3",      "AC3FrameParser",   &uni.ac3, 1, 375  },
    { "a.ac3.03f.ac3",      "AC3FrameParser",   &uni.ac3, 1, 375  },
    { "a.ac3.mix.ac3",      "AC3FrameParser",   &uni.ac3, 3, 1500 },
    { "test.eac3.03f.eac3", "EAC3FrameParser",  &uni.eac3,1, 819  },
    { "a.dts.03f.dts",      "DTSFrameParser",   &uni.dts, 1, 1125 },
    // We cannot load the last frame of SPDIF/DTS stream.
    // See note at StreamBuffer class comments.
    { "a.dts.03f.spdif",    "DTSFrameParser",   &uni.dts, 1, 1124 },
    { "a.mp2.002.mp2",      "MPAFrameParser",   &uni.mpa, 1, 500  },
    { "a.mp2.005.mp2",      "MPAFrameParser",   &uni.mpa, 1, 500  },
    { "a.mp2.mix.mp2",      "MPAFrameParser",   &uni.mpa, 3, 1500 },
    { "a.mp2.005.spdif",    "SPDIFFrameParser", &uni.spdif, 1, 500  },
    { "a.ac3.03f.spdif",    "SPDIFFrameParser", &uni.spdif, 1, 375  },
    { "a.dts.03f.spdif",    "SPDIFFrameParser", &uni.spdif, 1, 1125 },

    // Mixed streams
    { "a.mad.mix.spdif",    "SPDIFFrameParser", &uni.spdif, 7, 4375 },
    { "a.mad.mix.mad",      "UniFrameParser",   &uni,       7, 4375 },
    { "a.mad.mix.spdif",    "UniFrameParser",   &uni,       7, 4375 },
  };

  for (size_t iparser = 0; iparser < array_size(parsers); iparser++)
  {
    BOOST_MESSAGE("Passthrough test " << parsers[iparser].parser_name << " " << parsers[iparser].filename);

    MemFile f(parsers[iparser].filename);
    BOOST_REQUIRE(f);
    passthrough_test(parsers[iparser].parser, f, f.size(), parsers[iparser].streams, parsers[iparser].frames);
  }
}

BOOST_AUTO_TEST_CASE(noise_passthrough)
{
  ConstFrameSize   const_frame_size;
  HeaderFrameSize  header_frame_size;
  UnknownFrameSize unknown_frame_size;
  UniFrameParser   uni;

  RawNoise noise(noise_size, seed);
  // Mock parsers
  passthrough_test(&const_frame_size,   noise, noise.size(), 0, 0);
  passthrough_test(&header_frame_size,  noise, noise.size(), 0, 0);
  passthrough_test(&unknown_frame_size, noise, noise.size(), 0, 0);
  // Real parsers
  passthrough_test(&uni.adts,  noise, noise.size(), 0, 0);
  passthrough_test(&uni.ac3,   noise, noise.size(), 0, 0);
  passthrough_test(&uni.eac3,  noise, noise.size(), 0, 0);
  passthrough_test(&uni.dts,   noise, noise.size(), 0, 0);
  passthrough_test(&uni.mpa,   noise, noise.size(), 0, 0);
  passthrough_test(&uni.spdif, noise, noise.size(), 0, 0);
  passthrough_test(&uni,       noise, noise.size(), 0, 0);
}

BOOST_AUTO_TEST_CASE(bad_parser)
{
  // Endless loop was possible when frame parser return frame size > max frame size.
  // i.e. FrameInfo::frame_size > FrameParser::sync_info().max_frame_size()
  BadFrameParser parser;
  StreamBuffer streambuf(&parser);

  MemFile data("a.mp2.005.mp2");
  BOOST_REQUIRE(data.size() > 0);

  uint8_t *pos = data;
  uint8_t *end = pos + data.size();
  while (pos < end)
  {
    uint8_t *old_pos = pos;
    streambuf.load(&pos, end);
    if (old_pos == pos && !streambuf.has_frame() && !streambuf.has_debris())
      BOOST_FAIL("Endless loop");
  }
}

BOOST_AUTO_TEST_SUITE_END()
