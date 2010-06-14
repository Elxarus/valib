/*
  Streambuf class test

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
  * SPDIF parser uses scanning
  * DTS parser has unknown frame size
  * DTS parser can work with both raw DTS and SPDIF/DTS, but cannot load the
    last frame of SPDIF/DTS stream (see notes for StreamBuffer class)
*/

#include "auto_file.h"
#include "../noise_buf.h"

#include "parsers\ac3\ac3_header.h"
#include "parsers\dts\dts_header.h"
#include "parsers\mpa\mpa_header.h"
#include "parsers\spdif\spdif_header.h"
#include "parsers\multi_header.h"

#include <boost/test/unit_test.hpp>

static const int seed = 958745023;
static const int noise_size = 1*1024*1024;

///////////////////////////////////////////////////////////////////////////////
// Sync test
// Sync on a stream starting at some distance from the start of the buffer.
// The size of the junk before the stream equals to junk_size. Feed the parser
// with chunks of size chunk_size.

static void sync_test(const HeaderParser *hparser, uint8_t *buf, size_t buf_size, size_t junk_size, size_t chunk_size)
{
  // setup pointers
  uint8_t *ptr = buf;
  uint8_t *end = buf + buf_size;
  uint8_t *ref_ptr = buf;

  uint8_t *frame, *debris;
  size_t frame_size, debris_size;

  StreamBuffer streambuf(hparser);
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
      if (streambuf.is_debris_exists())
      {
        debris      = streambuf.get_debris();
        debris_size = streambuf.get_debris_size();
        if (memcmp(debris, ref_ptr, debris_size))
          BOOST_FAIL("Debris check failed at pos = " << ref_ptr - buf);
        ref_ptr += debris_size;
      }

      // check frame & junk size
      if (streambuf.is_frame_loaded())
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

static void passthrough_test(const HeaderParser *hparser, uint8_t *buf, size_t buf_size, int file_streams, int file_frames)
{
  // setup pointers
  uint8_t *ptr = buf;
  uint8_t *end = buf + buf_size;
  uint8_t *ref_ptr = buf;

  uint8_t *frame, *debris;
  size_t frame_size, debris_size;

  int frames = 0;
  int streams = 0;
  StreamBuffer streambuf(hparser);

  while (ptr < end)
  {
    // process data
    streambuf.load(&ptr, end);

    // count streams & frames
    if (streambuf.is_new_stream())  streams++;
    if (streambuf.is_frame_loaded()) frames++;

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

  // Check stream and frame counters
  if (file_streams) BOOST_CHECK_EQUAL(streams, file_streams);
  if (file_frames)  BOOST_CHECK_EQUAL(frames, file_frames);
}

BOOST_AUTO_TEST_SUITE(streambuf)

BOOST_AUTO_TEST_CASE(constructor)
{
  StreamBuffer buf;

  BOOST_CHECK(buf.get_parser() == 0);

  BOOST_CHECK(!buf.is_in_sync());
  BOOST_CHECK(!buf.is_new_stream());
  BOOST_CHECK(!buf.is_frame_loaded());
  BOOST_CHECK(!buf.is_debris_exists());
}

BOOST_AUTO_TEST_CASE(init_constructor)
{
  StreamBuffer buf(&ac3_header);

  BOOST_CHECK(buf.get_parser() == &ac3_header);

  BOOST_CHECK(!buf.is_in_sync());
  BOOST_CHECK(!buf.is_new_stream());
  BOOST_CHECK(!buf.is_frame_loaded());
  BOOST_CHECK(!buf.is_debris_exists());
}

BOOST_AUTO_TEST_CASE(set_parser)
{
  StreamBuffer buf;

  buf.set_parser(&ac3_header);
  BOOST_CHECK(buf.get_parser() == &ac3_header);

  buf.set_parser(&dts_header);
  BOOST_CHECK(buf.get_parser() == &dts_header);

  buf.release_parser();
  BOOST_CHECK(buf.get_parser() == 0);
}

///////////////////////////////////////////////////////////////////////////////
// Test synchrinization
// Some data may appear before the first synchpoint. Some combinations of
// pre-synch junk and chunk size may lead to sync problems. To test this,
// try different sizes of the junk data before the start of the stream.

BOOST_AUTO_TEST_CASE(sync)
{
  const HeaderParser *headers[] = { &spdif_header, &ac3_header, &mpa_header, &dts_header };
  const MultiHeader multi_header(headers, array_size(headers));

  const struct {
    const char *filename;
    const char *parser_name;
    const HeaderParser *parser;
  } parsers[] = {
    { "a.mp2.002.mp2",   "MPAHeader",   &mpa_header   },
    { "a.ac3.005.ac3",   "AC3Header",   &ac3_header   },
    { "a.dts.03f.dts",   "DTSHeader",   &dts_header   },
    { "a.ac3.03f.spdif", "SPDIFHeader", &spdif_header },
    { "a.mp2.002.mp2",   "MultiHeader", &multi_header },
    { "a.ac3.005.ac3",   "MultiHeader", &multi_header },
    { "a.dts.03f.dts",   "MultiHeader", &multi_header }
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
      1, 2, 3, 4, 5,
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
  const HeaderParser *headers[] = { &spdif_header, &ac3_header, &mpa_header, &dts_header };
  const MultiHeader multi_header(headers, array_size(headers));

  struct {
    const char *filename;
    const char *parser_name;
    const HeaderParser *hparser;
    int streams;
    int frames;
  } parsers[] = {
    { "a.mp2.002.mp2",   "MPAHeader", &mpa_header, 1, 500 },
    { "a.mp2.005.mp2",   "MPAHeader", &mpa_header, 1, 500 },
    { "a.mp2.mix.mp2",   "MPAHeader", &mpa_header, 3, 1500 },
    { "a.ac3.005.ac3",   "AC3Header", &ac3_header, 1, 375 },
    { "a.ac3.03f.ac3",   "AC3Header", &ac3_header, 1, 375 },
    { "a.ac3.mix.ac3",   "AC3Header", &ac3_header, 3, 1500 },
    { "a.dts.03f.dts",   "DTSHeader", &dts_header, 1, 1125 },
    // We cannot load the last frame of SPDIF/DTS stream.
    // See note at StreamBuffer class comments.
    { "a.dts.03f.spdif", "DTSHeader", &dts_header, 1, 1124 },
    { "a.mp2.005.spdif", "SPDIFHeader", &spdif_header, 1, 500 },
    { "a.ac3.03f.spdif", "SPDIFHeader", &spdif_header, 1, 375 },
    { "a.dts.03f.spdif", "SPDIFHeader", &spdif_header, 1, 1125 },
    { "a.mad.mix.spdif", "SPDIFHeader", &spdif_header, 7, 4375 },
    { "a.mad.mix.mad",   "SPDIFHeader", &multi_header, 7, 4375 },
    { "a.mad.mix.spdif", "SPDIFHeader", &multi_header, 7, 4375 },
  };

  for (size_t iparser = 0; iparser < array_size(parsers); iparser++)
  {
    BOOST_MESSAGE("Passthrough test " << parsers[iparser].parser_name << " " << parsers[iparser].filename);

    MemFile f(parsers[iparser].filename);
    BOOST_REQUIRE(f);
    passthrough_test(parsers[iparser].hparser, f, f.size(), parsers[iparser].streams, parsers[iparser].frames);
  }
}

BOOST_AUTO_TEST_CASE(noise_passthrough)
{
  const HeaderParser *headers[] = { &spdif_header, &ac3_header, &mpa_header, &dts_header };
  const MultiHeader multi_header(headers, array_size(headers));

  RawNoise noise(noise_size, seed);
  passthrough_test(&mpa_header, noise, noise.size(), 0, 0);
  passthrough_test(&ac3_header, noise, noise.size(), 0, 0);
  passthrough_test(&dts_header, noise, noise.size(), 0, 0);
  passthrough_test(&spdif_header, noise, noise.size(), 0, 0);
  passthrough_test(&multi_header, noise, noise.size(), 0, 0);
}

BOOST_AUTO_TEST_SUITE_END()
