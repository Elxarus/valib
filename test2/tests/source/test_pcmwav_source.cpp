/*
  PcmWavSource test
*/

#include <boost/test/unit_test.hpp>
#include "filters/gain.h"
#include "source/pcmwav_source.h"
#include "source/generator.h"
#include "sink/sink_pcmwav.h"
#include "../../suite.h"
#include "../../temp_filename.h"

static const char *filename_bad = "nonexistent_file";
static const char *filename = "test.pcm16.03f.wav";
static const Speakers spk = Speakers(FORMAT_LINEAR, MODE_5_1, 48000, 32767.5);

static int seed = 467398456;
static size_t noise_samples = 2048;

static void read_write_test(int format)
{
  BOOST_MESSAGE("Read/write " << format_text(format) << " wave file");
  Speakers file_spk(format, spk.mask, spk.sample_rate);
  Speakers noise_spk(FORMAT_LINEAR, spk.mask, spk.sample_rate, file_spk.level);

  TempFilename temp;
  NoiseGen noise(noise_spk, seed, noise_samples);
  Gain gain(file_spk.level);
  SourceFilter data_source(&noise, &gain);

  // Write wav file
  Chunk chunk;
  PcmWavSink sink(temp.c_str(), format);
  sink.open(spk);
  BOOST_REQUIRE(sink.is_open());
  while (data_source.get_chunk(chunk))
    sink.process(chunk);
  sink.flush();
  sink.close_file();

  // Read wav file
  data_source.reset();
  PcmWavSource source(temp.c_str(), noise_samples);
  sample_t diff = calc_diff(&data_source, &source);
  BOOST_CHECK_LT(diff, 0.5);
}

BOOST_AUTO_TEST_SUITE(pcmwav_source)

BOOST_AUTO_TEST_CASE(constructor)
{
  PcmWavSource source;

  BOOST_CHECK_EQUAL(source.get_output(), spk_unknown);
  BOOST_CHECK(!source.is_file_open());
  BOOST_CHECK(!source.new_stream());
  BOOST_CHECK(!source.get_chunk(Chunk()));
  source.reset();
}

BOOST_AUTO_TEST_CASE(init_constructor)
{
  PcmWavSource source(filename);

  BOOST_CHECK_EQUAL(source.get_output(), spk);
  BOOST_CHECK(source.is_file_open());
  BOOST_CHECK(!source.new_stream());
}

BOOST_AUTO_TEST_CASE(open_close)
{
  PcmWavSource source;

  // Nonexistent file
  BOOST_CHECK(!source.open_file(filename_bad));
  BOOST_CHECK_EQUAL(source.get_output(), spk_unknown);
  BOOST_CHECK(!source.is_file_open());
  BOOST_CHECK(!source.new_stream());

  // Open good file
  BOOST_CHECK(source.open_file(filename));
  BOOST_CHECK_EQUAL(source.get_output(), spk);
  BOOST_CHECK(source.is_file_open());
  BOOST_CHECK(!source.new_stream());

  // Close
  source.close_file();
  BOOST_CHECK_EQUAL(source.get_output(), spk_unknown);
  BOOST_CHECK(!source.is_file_open());
  BOOST_CHECK(!source.new_stream());
}

// Test correct channel order
BOOST_AUTO_TEST_CASE(channel_order)
{
  int format = FORMAT_PCM16;
  Speakers spk_linear(FORMAT_LINEAR, MODE_7_1, 48000);
  Speakers spk_pcm(format, MODE_7_1, 48000);
  TempFilename temp;

  // Write using WavSink
  Chunk chunk;
  const size_t data_size = NCHANNELS * sample_size(format);
  uint8_t data[16] =
  { 0x01, 0x00, 0x02, 0x00, 0x03, 0x00, 0x04, 0x00, 0x05, 0x00, 0x06, 0x00, 0x07, 0x00, 0x08, 0x00 };
  chunk.set_rawdata(data, data_size);
  BOOST_REQUIRE_EQUAL(array_size(data), data_size);

  WAVSink wav(temp.c_str());
  wav.open(spk_pcm);
  BOOST_REQUIRE(wav.is_open());
  wav.process(chunk);
  wav.flush();
  wav.close_file();

  // Read using PcmWavSource
  PcmWavSource pcmwav(temp.c_str(), format);
  BOOST_REQUIRE(pcmwav.is_file_open());
  pcmwav.get_chunk(chunk);

  sample_t s[NCHANNELS] = { 1.5, 3.5, 2.5, 7.5, 8.5, 4.5, 5.5, 6.5 };
  for (int i = 0; i < NCHANNELS; i++)
    BOOST_CHECK(EQUAL_SAMPLES(chunk.samples[i][0], s[i]));
}

BOOST_AUTO_TEST_CASE(get_data)
{
  read_write_test(FORMAT_PCM16);
  read_write_test(FORMAT_PCM24);
  read_write_test(FORMAT_PCM32);
  read_write_test(FORMAT_PCMFLOAT);
  read_write_test(FORMAT_PCMDOUBLE);
}

BOOST_AUTO_TEST_SUITE_END()
