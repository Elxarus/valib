/*
  PcmWavSink class test
*/

#include <boost/test/unit_test.hpp>
#include "sink/sink_pcmwav.h"
#include "../../suite.h"
#include "../../temp_filename.h"
#include "boost/filesystem.hpp"

static Speakers spk(FORMAT_LINEAR, MODE_STEREO, 48000, 32767.5);
static Speakers spk_bad(FORMAT_LINEAR, 0, 0, 32767.5);
static const int format = FORMAT_PCM16;

BOOST_AUTO_TEST_SUITE(pcmwav_sink)

BOOST_AUTO_TEST_CASE(constructor)
{
  PcmWavSink sink;

  BOOST_CHECK(!sink.is_open());
  BOOST_CHECK(!sink.is_file_open());
  BOOST_CHECK(!sink.can_open(spk));
}

BOOST_AUTO_TEST_CASE(init_constructor)
{
  TempFilename temp;
  PcmWavSink sink(temp.c_str(), format);

  BOOST_CHECK(!sink.is_open());
  BOOST_CHECK(sink.is_file_open());
  BOOST_CHECK(sink.can_open(spk));

  BOOST_CHECK(boost::filesystem::exists(temp.str()));
}

BOOST_AUTO_TEST_CASE(file_open_close)
{
  TempFilename temp;
  PcmWavSink sink;

  BOOST_CHECK(sink.open_file(temp.c_str(), format));
  BOOST_CHECK(!sink.is_open());
  BOOST_CHECK(sink.is_file_open());
  BOOST_CHECK(sink.can_open(spk));
  BOOST_CHECK(boost::filesystem::exists(temp.str()));

  sink.close_file();
  BOOST_CHECK(!sink.is_open());
  BOOST_CHECK(!sink.is_file_open());
  BOOST_CHECK(!sink.can_open(spk));
}

BOOST_AUTO_TEST_CASE(open_sink)
{
  TempFilename temp;
  PcmWavSink sink;

  // Try to open sink with no file open
  BOOST_CHECK(!sink.open(spk));

  // Open file, then open sink
  BOOST_CHECK(sink.open_file(temp.c_str(), format));
  BOOST_CHECK(sink.open(spk));
  BOOST_CHECK(sink.is_open());
  BOOST_CHECK_EQUAL(sink.get_input(), spk);
}

BOOST_AUTO_TEST_CASE(open_close)
{
  TempFilename temp;
  PcmWavSink sink;

  // Open file, open sink, close file
  BOOST_CHECK(sink.open_file(temp.c_str(), format));
  BOOST_CHECK(sink.open(spk));
  sink.close_file();
  BOOST_CHECK(!sink.is_file_open());
  BOOST_CHECK(!sink.is_open());
  BOOST_CHECK(!sink.can_open(spk));

  // Open file, open sink, close sink, close file
  BOOST_CHECK(sink.open_file(temp.c_str(), format));
  BOOST_CHECK(sink.open(spk));
  sink.close();
  BOOST_CHECK(!sink.is_open());
  BOOST_CHECK(sink.can_open(spk));
  sink.close_file();
  BOOST_CHECK(!sink.is_file_open());
  BOOST_CHECK(!sink.can_open(spk));
}

// Test correct channel order
BOOST_AUTO_TEST_CASE(channel_order)
{
  int format = FORMAT_PCM16;
  Speakers spk_linear(FORMAT_LINEAR, MODE_7_1, 48000);
  Speakers spk_pcm(format, MODE_7_1, 48000);
  TempFilename temp_wav, temp_pcmwav;
  Chunk chunk;

  // Write using PcmWavSink
  sample_t s[NCHANNELS] = { 1, 3, 2, 7, 8, 4, 5, 6 };
  chunk.size = 1;
  for (int i = 0; i < NCHANNELS; i++)
    chunk.samples[i] = s + i;

  PcmWavSink pcmwav(temp_pcmwav.c_str(), format);
  pcmwav.open(spk_linear);
  BOOST_REQUIRE(pcmwav.is_open());
  pcmwav.process(chunk);
  pcmwav.flush();
  pcmwav.close_file();

  // Write using WavSink
  const size_t data_size = NCHANNELS * sample_size(format);
  uint8_t data[16] =
  { 0x01, 0x00, 0x02, 0x00, 0x03, 0x00, 0x04, 0x00, 0x05, 0x00, 0x06, 0x00, 0x07, 0x00, 0x08, 0x00 };
  chunk.set_rawdata(data, data_size);
  BOOST_REQUIRE_EQUAL(array_size(data), data_size);

  WAVSink wav(temp_wav.c_str());
  wav.open(spk_pcm);
  BOOST_REQUIRE(wav.is_open());
  wav.process(chunk);
  wav.flush();
  wav.close_file();

  // Compare
  MemFile file_pcmwav(temp_pcmwav.c_str());
  MemFile file_wav(temp_wav.c_str());
  BOOST_REQUIRE_EQUAL(file_pcmwav.size(), file_wav.size());
  BOOST_CHECK(memcmp(file_pcmwav, file_wav, file_pcmwav.size()) == 0);
}

BOOST_AUTO_TEST_SUITE_END()
