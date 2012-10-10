/*
  WAVSink test
*/

#include <boost/test/unit_test.hpp>
#include "sink/sink_wav.h"
#include "../../suite.h"
#include "../../temp_filename.h"

static const Speakers spk1(FORMAT_PCM16, MODE_STEREO, 48000);
static const Speakers spk_bad(FORMAT_LINEAR, MODE_5_1, 48000);

static uint8_t data[] = { 1, 0, 2, 0, 3, 0, 4, 0 }; // 2 PCM16 samples
static const uint8_t file_data[] = 
{
  0x52,0x49,0x46,0x46, // 'RIFF'
  0x52,0x00,0x00,0x00, // RIFF size
  0x57,0x41,0x56,0x45, // 'WAVE'
  0x4a,0x55,0x4e,0x4b, // 'JUNK'
  0x1c,0x00,0x00,0x00, // Junk chunk size
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,
  0x66,0x6d,0x74,0x20, // 'fmt '
  0x12,0x00,0x00,0x00, // Format chunk size
  0x01,0x00,0x02,0x00,0x80,0xbb,0x00,0x00,
  0x00,0xee,0x02,0x00,0x04,0x00,0x10,0x00,
  0x00,0x00,
  0x64,0x61,0x74,0x61, // 'data'
  0x08,0x00,0x00,0x00, // Data chunk size
  0x01,0x00,0x02,0x00, // sample 1
  0x03,0x00,0x04,0x00, // sample 2
};

BOOST_AUTO_TEST_SUITE(wav_sink)

BOOST_AUTO_TEST_CASE(constructor)
{
  WAVSink sink;
  BOOST_CHECK(!sink.is_file_open());
  BOOST_CHECK(!sink.is_open());

  // Sink with no file open cannot be open at all
  BOOST_CHECK(!sink.can_open(spk1));
}

BOOST_AUTO_TEST_CASE(init_constructor)
{
  TempFilename tmp;
  WAVSink sink(tmp.c_str());
  BOOST_CHECK(sink.is_file_open());
  BOOST_CHECK_EQUAL(sink.filename(), tmp.str());
  BOOST_CHECK(!sink.is_open());
  BOOST_CHECK(sink.can_open(spk1));
}

BOOST_AUTO_TEST_CASE(file_open_close)
{
  TempFilename tmp;
  WAVSink sink;

  bool result = sink.open_file(tmp.c_str());
  BOOST_CHECK(result);
  BOOST_CHECK(sink.is_file_open());
  BOOST_CHECK_EQUAL(sink.filename(), tmp.str());
  BOOST_CHECK(!sink.is_open());
  BOOST_CHECK(sink.can_open(spk1));

  sink.close_file();
  BOOST_CHECK(!sink.is_file_open());
  BOOST_CHECK(!sink.is_open());
  BOOST_CHECK(!sink.can_open(spk1));
}

BOOST_AUTO_TEST_CASE(open_sink)
{
  TempFilename tmp;
  WAVSink sink;

  // Try to open sink with no file open
  bool result = sink.open(spk1);
  BOOST_CHECK(!result);

  // Open file, then open sink
  result = sink.open_file(tmp.c_str());
  BOOST_CHECK(result);
  result = sink.open(spk1);
  BOOST_CHECK(result);
  BOOST_CHECK(sink.is_open());
  BOOST_CHECK_EQUAL(sink.get_input(), spk1);

  // Should destruct well now
}

BOOST_AUTO_TEST_CASE(open_close)
{
  TempFilename tmp;
  WAVSink sink;

  // Open file, open sink, close file
  bool result = sink.open_file(tmp.c_str());
  BOOST_CHECK(result);
  result = sink.open(spk1);
  BOOST_CHECK(result);
  sink.close_file();
  BOOST_CHECK(!sink.is_file_open());
  BOOST_CHECK(!sink.is_open());
  BOOST_CHECK(!sink.can_open(spk1));

  // Open file, open sink, close sink, close file
  result = sink.open_file(tmp.c_str());
  BOOST_CHECK(result);
  result = sink.open(spk1);
  BOOST_CHECK(result);
  sink.close();
  BOOST_CHECK(!sink.is_open());
  BOOST_CHECK(sink.can_open(spk1));
  sink.close_file();
  BOOST_CHECK(!sink.is_file_open());
  BOOST_CHECK(!sink.can_open(spk1));
}

BOOST_AUTO_TEST_CASE(process)
{
  TempFilename tmp;
  WAVSink sink(tmp.c_str());
  sink.open(spk1);
  sink.process(Chunk(data, array_size(data)));
  sink.close_file();

  MemFile f(tmp.c_str());
  BOOST_REQUIRE_EQUAL(f.size(), array_size(file_data));
  BOOST_CHECK(memcmp(f, file_data, f.size()) == 0);
}

BOOST_AUTO_TEST_SUITE_END();
