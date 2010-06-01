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

void passthrough_test(const HeaderParser *hparser, uint8_t *buf, size_t buf_size, int file_streams, int file_frames, size_t chunk_size)
{
  // setup pointers
  uint8_t *ptr = buf;
  uint8_t *end = buf + buf_size;
  uint8_t *ref_ptr = buf;

  uint8_t *frame;
  size_t frame_size;
  uint8_t *debris;
  size_t debris_size;

  // setup cycle
  int frames = 0;
  int streams = 0;

  StreamBuffer streambuf(hparser);
  while (ptr < buf + buf_size)
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

      // count frames and streams
      if (streambuf.is_new_stream())   streams++;
      if (streambuf.is_frame_loaded()) frames++;

      // get data
      debris      = streambuf.get_debris();
      debris_size = streambuf.get_debris_size();
      frame       = streambuf.get_frame();
      frame_size  = streambuf.get_frame_size();

      if (ref_ptr + debris_size + frame_size > end)
        BOOST_FAIL("Frame ends after the end of the reference file");

      // Check debris
      if (memcmp(debris, ref_ptr, debris_size))
        BOOST_FAIL("Debris check failed at pos = " << ref_ptr - buf);
      ref_ptr += debris_size;

      // Check frame
      if (memcmp(frame, ref_ptr, frame_size))
        BOOST_FAIL("Frame check failed at pos = " << ref_ptr - buf);
      ref_ptr += frame_size;

      // zap the frame buffer to simulate in-place processing
      memset(frame, 0, frame_size);
    }
  }

  // Check stream and frame counters
  if (file_streams) BOOST_CHECK_EQUAL(streams, file_streams);
  if (file_frames)  BOOST_CHECK_EQUAL(frames, file_frames);
}

void passthrough_test(const char *filename, const HeaderParser *hparser, int file_streams, int file_frames)
{
  BOOST_MESSAGE("Passthrough test " << filename);

  MemFile f(filename);
  BOOST_REQUIRE(f);

  const size_t chunk_size[] = {
    0,        // The whole file at once
    1,        // Per-byte processing (very slow)
    2, 3,     // Very small chunk size
    127, 128, // Small chunk size

    // border frame sizes
    hparser->max_frame_size(),
    hparser->max_frame_size() + 1,
    hparser->max_frame_size() - 1
  };

  for (int i = 0; i < array_size(chunk_size); i++)
    passthrough_test(hparser, f, f.size(), file_streams, file_frames, chunk_size[i]);
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

BOOST_AUTO_TEST_CASE(passthrough)
{
  RawNoise noise(noise_size, seed);
  const HeaderParser *headers[] = { &spdif_header, &ac3_header, &mpa_header, &dts_header };
  MultiHeader multi_header(headers, array_size(headers));

  BOOST_MESSAGE("MPAHeader");
  passthrough_test("a.mp2.002.mp2",   &mpa_header, 1, 500);
  passthrough_test("a.mp2.005.mp2",   &mpa_header, 1, 500);
  passthrough_test("a.mp2.mix.mp2",   &mpa_header, 3, 1500);
  passthrough_test(&mpa_header, noise, noise.size(), 0, 0, 0);
                                 
  BOOST_MESSAGE("AC3Header");
  passthrough_test("a.ac3.005.ac3",   &ac3_header, 1, 375);
  passthrough_test("a.ac3.03f.ac3",   &ac3_header, 1, 375);
  passthrough_test("a.ac3.mix.ac3",   &ac3_header, 3, 1500);
  passthrough_test(&ac3_header, noise, noise.size(), 0, 0, 0);
                                 
  // We cannot load the last frame of SPDIF/DTS stream.
  // See note at StreamBuffer class comments.
  BOOST_MESSAGE("DTSHeader");
  passthrough_test("a.dts.03f.dts",   &dts_header, 1, 1125);
  passthrough_test("a.dts.03f.spdif", &dts_header, 1, 1124);
  passthrough_test(&dts_header, noise, noise.size(), 0, 0, 0);
                                 
  // SPDIFHeader must work with SPDIF/DTS stream correctly
  BOOST_MESSAGE("SPDIFHeader");
  passthrough_test("a.mp2.005.spdif", &spdif_header, 1, 500);
  passthrough_test("a.ac3.03f.spdif", &spdif_header, 1, 375);
  passthrough_test("a.dts.03f.spdif", &spdif_header, 1, 1125);
  passthrough_test("a.mad.mix.spdif", &spdif_header, 7, 4375);
  passthrough_test(&spdif_header, noise, noise.size(), 0, 0, 0);
                                 
  BOOST_MESSAGE("MultiHeader");
  passthrough_test("a.mad.mix.mad",   &multi_header, 7, 4375);
  passthrough_test("a.mad.mix.spdif", &multi_header, 7, 4375);
  passthrough_test(&multi_header, noise, noise.size(), 0, 0, 0);
}

BOOST_AUTO_TEST_SUITE_END()
