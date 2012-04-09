/*
  PSParser and MPEGDemux test
*/

#include "auto_file.h"
#include "mpeg_demux.h"
#include "../noise_buf.h"
#include <boost/test/unit_test.hpp>

static void sync_test(uint8_t *buf, size_t buf_size, size_t junk_size, size_t chunk_size)
{
  uint8_t *ptr = buf;
  uint8_t *end = buf + buf_size;

  PSParser parser;
  while (ptr < buf + buf_size && !parser.payload_size)
  {
    if (chunk_size)
    {
      end = ptr + chunk_size;
      if (end > buf + buf_size)
        end = buf + buf_size;
    }

    // process data
    while (ptr < end)
      if (parser.parse(&ptr, end))
      {
        size_t actual_junk_size = ptr - buf - parser.header_size;
        if (actual_junk_size != junk_size)
          BOOST_CHECK_EQUAL(actual_junk_size, junk_size);
        break;
      }
  }
}

///////////////////////////////////////////////////////////////////////////////
// PSParser

BOOST_AUTO_TEST_SUITE(ps_parser)

BOOST_AUTO_TEST_CASE(constructor)
{
  PSParser parser;

  // Has no data loaded
  BOOST_CHECK_EQUAL(parser.header_size, 0);
  BOOST_CHECK_EQUAL(parser.payload_size, 0);

  // No stream info
  BOOST_CHECK_EQUAL(parser.stream, 0);
  BOOST_CHECK_EQUAL(parser.substream, 0);
  BOOST_CHECK_EQUAL(parser.packets, 0);
  BOOST_CHECK_EQUAL(parser.errors, 0);
}

///////////////////////////////////////////////////////////////////////////////
// Test synchrinization
// Some data may appear before the first synchpoint. Some combinations of
// pre-synch junk and chunk size may lead to sync problems. To test this,
// try different sizes of the junk data before the start of the stream.

BOOST_AUTO_TEST_CASE(sync)
{
  const char *files[] = {
    "a.ac3.03f.pes",
    "a.dts.03f.pes",
    "a.mp2.005.pes",
    "a.pcm.005.pes",
  };

  const size_t chunk_size[] = {
    0,                     // the whole buffer at once
    1, 2, 3, 5,            // very small chunks
    512, 1024, 4096, 8192, // large chunks
  };

  const size_t junk_size[] = {
    1, 2, 3, 4, 5, 16
  };

  RawNoise data;
  for (size_t ifile = 0; ifile < array_size(files); ifile++)
  {
    MemFile file(files[ifile]);
    BOOST_MESSAGE("Sync test " << files[ifile]);
    BOOST_REQUIRE(file);

    for (size_t ijunk_size = 0; ijunk_size < array_size(junk_size); ijunk_size++)
      for (size_t ichunk = 0; ichunk < array_size(chunk_size); ichunk++)
      {
        data.allocate(junk_size[ijunk_size] + file.size());
        data.rng.fill_raw(data, junk_size[ijunk_size]);
        memcpy(data + junk_size[ijunk_size], file, file.size());
        sync_test(
          data, junk_size[ijunk_size] + file.size(),
          junk_size[ijunk_size], chunk_size[ichunk]);
      }
  }
}

BOOST_AUTO_TEST_CASE(demux)
{
  const struct { const char *pes; const char *raw; } files[] = {
    { "a.ac3.005.pes", "a.ac3.005.ac3" },
    { "a.ac3.03f.pes", "a.ac3.03f.ac3" },
    { "a.dts.03f.pes", "a.dts.03f.dts" },
    { "a.mp2.002.pes", "a.mp2.002.mp2" },
    { "a.mp2.005.pes", "a.mp2.005.mp2" },
    { "a.pcm.005.pes", "a.pcm.005.lpcm" },

    { "a.ac3.mix.pes", "a.ac3.mix.ac3" },
    { "a.mp2.mix.pes", "a.mp2.mix.mp2" },

    { "a.mad.mix.pes", "a.mad.mix.mad" },
    { "a.madp.mix.pes","a.madp.mix.madp" }
  };

  for (size_t ifile = 0; ifile < array_size(files); ifile++)
  {
    MemFile pes(files[ifile].pes);
    MemFile raw(files[ifile].raw);

    uint8_t *ptr = pes;
    uint8_t *end = pes + pes.size();
    uint8_t *ref = raw;

    PSParser parser;
    while (ptr < end)
      if (parser.parse(&ptr, end))
      {
        if (memcmp(ptr, ref, parser.payload_size) != 0)
          BOOST_FAIL("Fail at frame = " << parser.packets << " pos = " << ptr - pes);
        ref += parser.payload_size;
        ptr += parser.payload_size;
      }

    if (ptr > end)
      BOOST_FAIL("Stream ends after the end of the file");

    if (ref < raw + raw.size())
      BOOST_FAIL("Reference file is longer than parser's output");
  }
}

BOOST_AUTO_TEST_SUITE_END()

///////////////////////////////////////////////////////////////////////////////
// PSDemux

BOOST_AUTO_TEST_SUITE(ps_demux)

BOOST_AUTO_TEST_CASE(constructor)
{
  PSDemux demux;
  BOOST_CHECK_EQUAL(demux.stream, 0);
  BOOST_CHECK_EQUAL(demux.substream, 0);
}

BOOST_AUTO_TEST_CASE(init_constructor)
{
  const int stream = 0xbd;
  const int substream = 0x82;

  PSDemux demux(stream, substream);
  BOOST_CHECK_EQUAL(demux.stream, stream);
  BOOST_CHECK_EQUAL(demux.substream, substream);
}

BOOST_AUTO_TEST_CASE(demux)
{
  const struct
  {
    const char *pes;
    const char *raw;
    int stream, substream;
  } files[] = {
    // Sync on the certain stream
    { "a.ac3.03f.pes", "a.ac3.03f.ac3",  0xbd, 0x82 },
    { "a.dts.03f.pes", "a.dts.03f.dts",  0xbd, 0x8d },
    { "a.mp2.005.pes", "a.mp2.005.mp2",  0xc0, 0x00 },
    { "a.pcm.005.pes", "a.pcm.005.lpcm", 0xbd, 0xa6 },
    // Extract the certain stream
    { "a.ac3.mix.pes", "a.ac3.005.ac3",  0xbd, 0x84 },
    { "a.mp2.mix.pes", "a.mp2.002.mp2",  0xc1, 0x00 },
    // Sync on the first stream, allow resync
    { "a.ac3.mix.pes", "a.ac3.mix.ac3",  0, 0 },
    { "a.mp2.mix.pes", "a.mp2.mix.mp2",  0, 0 },
    { "a.mad.mix.pes", "a.mad.mix.mad",  0, 0 },
    { "a.madp.mix.pes","a.madp.mix.madp",0, 0 }
  };

  for (int ifile = 0; ifile < array_size(files); ifile++)
  {
    BOOST_MESSAGE(files[ifile].pes);

    MemFile pes(files[ifile].pes);
    MemFile raw(files[ifile].raw);

    PSDemux demux(files[ifile].stream, files[ifile].substream);
    size_t demux_size = demux.demux(pes, pes.size());

    BOOST_CHECK_EQUAL(demux_size, raw.size());
    BOOST_CHECK(memcmp(pes, raw, demux_size) == 0);
  }
}

BOOST_AUTO_TEST_SUITE_END()
