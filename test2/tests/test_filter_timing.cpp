/*
  Test correctness of timestamp handling
*/

#include <boost/test/unit_test.hpp>
#include "fir/param_fir.h"
#include "source/file_parser.h"
#include "source/generator.h"
#include "source/raw_source.h"
#include "source/source_filter.h"
#include "../all_filters.h"

static const int seed = 9873457;
static const size_t noise_size = 1024*1024;
static const vtime_t time1 = 123;
static const vtime_t time2 = 321;

#define FIRST_TIMESTAMP1 1
#define FIRST_TIMESTAMP2 2
#define FIRST_TIMESTAMP3 4
#define BUFFERING        8

#define ALL_TESTS        0xf
#define PARSER_TESTS     3


static const vtime_t size2time(Speakers spk, size_t size)
{
  size_t k = 0;
  switch (spk.format)
  {
    case FORMAT_LINEAR:
      return vtime_t(size) / spk.sample_rate;

    case FORMAT_PCM16:
    case FORMAT_PCM16_BE:
      return vtime_t(size) / (2 * spk.nch() * spk.sample_rate);

    case FORMAT_PCM24:
    case FORMAT_PCM24_BE:
      return vtime_t(size) / (3 * spk.nch() * spk.sample_rate);

    case FORMAT_PCM32:
    case FORMAT_PCM32_BE:
      return vtime_t(size) / (4 * spk.nch() * spk.sample_rate);

    case FORMAT_PCMFLOAT:
      return vtime_t(size) / (sizeof(float) * spk.nch() * spk.sample_rate);

    case FORMAT_PCMDOUBLE:
      return vtime_t(size) / (sizeof(double) * spk.nch() * spk.sample_rate);

    case FORMAT_SPDIF:
      return vtime_t(size) / (4 * spk.sample_rate);

    default:
      return 0;
  }
}

static inline bool compare_time(vtime_t t1, vtime_t t2)
{
  return fabs(t1 - t2) < 1e-6;
}

static void test_first_timestamp1(Source *src, Filter *f)
{
  // Scenario:
  // 1) reset()
  // 2) Timestamp the first input chunk
  // 3) Ensure, that first output chunk has the timestamp
  // 4) Get no timestamp for the next output chunk

  if (!f->is_open())
    f->open(src->get_output());
  BOOST_REQUIRE(f->is_open());

  f->reset();
  Chunk in, out;
  src->get_chunk(in);
  in.set_sync(true, time1);
  while (!f->process(in, out))
    if (!src->get_chunk(in))
      BOOST_FAIL("Cannot fill the filter");

  BOOST_CHECK(out.sync);
  BOOST_CHECK_EQUAL(out.time, time1);

  while (!f->process(in, out))
    if (!src->get_chunk(in))
      BOOST_FAIL("Cannot fill the filter");

  BOOST_CHECK(!out.sync);
  while (f->process(in, out))
    /*do nothing*/;
}

static void test_first_timestamp2(Source *src, Filter *f, bool frame_sync)
{
  // Scenario:
  // 1) reset();
  // 2) Fill the filter and get first output chunk(s)
  // 3) Timestamp next input chunk
  // 4) Wait for output chunk with timestamp and check it
  // 5) Get no timestamp for the next chunk

  // Note, that output timestamp may be shifted because of buffering:
  //                   t1 (at pos1)
  //                   V
  // +---------------------------------------------
  // |        |        |        | input chunks
  // +---------------------------------------------
  // |           |           |           | output chunks
  // +---------------------------------------------
  //                         V
  //                         t1' = t1 + dt (at pos2)
  // To find dt we have to track the position of input and output timestamps
  // and convert the difference into time. It is done with size2time().

  Chunk in, out;
  size_t in_pos = 0;
  size_t out_pos = 0;

  if (!f->is_open())
    f->open(src->get_output());
  BOOST_REQUIRE(f->is_open());

  f->reset();
  src->get_chunk(in);
  in_pos += in.size;
  while (!f->process(in, out))
  {
    if (!src->get_chunk(in))
      BOOST_FAIL("Cannot fill the filter");
    in_pos += in.size;
  }
  out_pos += out.size;

  BOOST_CHECK(!out.sync);
  while (f->process(in, out))
  {
    out_pos += out.size;
    BOOST_CHECK(!out.sync);
  }

  src->get_chunk(in);
  in.set_sync(true, time1);
  while (!out.sync)
    if (f->process(in, out))
      out_pos += out.size;
    else if (!src->get_chunk(in))
      BOOST_FAIL("Cannot fill the filter");

  if (frame_sync)
    BOOST_CHECK_EQUAL(out.time, time1);
  else
  {
    vtime_t in_pos_time = size2time(f->get_input(), in_pos);
    vtime_t out_pos_time = size2time(f->get_output(), out_pos - out.size);
    BOOST_CHECK(compare_time(out.time - time1, out_pos_time - in_pos_time));
  }
}

static void test_first_timestamp3(Source *src, Filter *f)
{
  // Scenario:
  // 1) reset()
  // 2) Timestamp the first *empty* input chunk
  // 3) Ensure, that first output chunk has the timestamp
  // 4) Get no timestamp for the next output chunk

  if (!f->is_open())
    f->open(src->get_output());
  BOOST_REQUIRE(f->is_open());

  f->reset();
  Chunk in, out;
  in.set_sync(true, time1);
  while (!f->process(in, out))
    if (!src->get_chunk(in))
      BOOST_FAIL("Cannot fill the filter");

  BOOST_CHECK(out.sync);
  BOOST_CHECK_EQUAL(out.time, time1);

  while (!f->process(in, out))
    if (!src->get_chunk(in))
      BOOST_FAIL("Cannot fill the filter");

  BOOST_CHECK(!out.sync);
  while (f->process(in, out))
    /*do nothing*/;
}

static void test_buffering(Source *src, Filter *f, bool frame_sync)
{
  // Test buffering filter.
  // Scenario:
  // 1) reset();
  // 2) Timestamp the first chunk (t1) and make it very small
  // 3) Process it.
  // 4) If filter returns data, it is not buffering filter. Exit.
  // 5) Timestamp the next chunk with differnt time (t2)
  // 6) Wait for output chunk and check that timestamp equals to time1
  // 7) Next chunk must be stamped with t2+dt.
  // 8) Get no timestamp for the next chunk.

  // t1  t2
  // V   V
  // +---------------------------------------------
  // |   |              |           | input chunks
  // +---------------------------------------------
  // |           |           |           | output chunks
  // +---------------------------------------------
  // V           V
  // t1          t2' = t2 + dt

  static const size_t min_block_size = 1;

  Chunk in, out1, out2;
  if (!f->is_open())
    f->open(src->get_output());
  BOOST_REQUIRE(f->is_open());
  f->reset();

  // Split the first chunk into 2 parts:
  // very small (size=1) and the rest
  Chunk chunk;
  src->get_chunk(chunk);

  in = chunk;
  in.size = min_block_size;
  in.set_sync(true, time1);
  if (f->process(in, Chunk()))
    // not a buffering filter
    return;

  in = chunk;
  if (in.rawdata)
    in.drop_rawdata(min_block_size);
  else
    in.drop_samples(min_block_size);
  in.set_sync(true, time2);

  // Get the first output chunk
  while (!f->process(in, out1))
    if (!src->get_chunk(in))
      BOOST_FAIL("Cannot fill the filter");

  BOOST_CHECK_EQUAL(out1.time, time1);

  // Get the second output chunk
  while (!f->process(in, out2))
    if (!src->get_chunk(in))
      BOOST_FAIL("Cannot fill the filter");

  BOOST_CHECK(out2.sync);
  if (frame_sync)
    BOOST_CHECK_EQUAL(out2.time, time2);
  else
  {
    vtime_t in_pos_time = size2time(f->get_input(), min_block_size);
    vtime_t out_pos_time = size2time(f->get_output(), out1.size);
    BOOST_CHECK(compare_time(out2.time - time2, out_pos_time - in_pos_time));
  }
}

static void test_timing(Source *src, Filter *f, bool frame_sync = false, int tests = ALL_TESTS)
{
  if (!f->is_open())
    f->open(src->get_output());
  BOOST_REQUIRE(f->is_open());

  if (tests & FIRST_TIMESTAMP1) test_first_timestamp1(src, f);
  if (tests & FIRST_TIMESTAMP2) test_first_timestamp2(src, f, frame_sync);
  if (tests & FIRST_TIMESTAMP3) test_first_timestamp3(src, f);
  if (tests & BUFFERING)        test_buffering(src, f, frame_sync);
}

static void test_timing(Speakers spk, Filter *f, const char *filename = 0, bool frame_sync = false, int tests = ALL_TESTS)
{
  BOOST_REQUIRE(f);
  BOOST_REQUIRE(f->open(spk));
  if (filename)
  {
    RAWSource raw(spk, filename);
    BOOST_REQUIRE(raw.is_open());
    test_timing(&raw, f, frame_sync, tests);
  }
  else
  {
    NoiseGen noise(spk, seed, noise_size);
    test_timing(&noise, f, frame_sync, tests);
  }
}

BOOST_AUTO_TEST_SUITE(filter_timing)

BOOST_AUTO_TEST_CASE(agc)
{
  AGC filter(1024);
  test_timing(Speakers(FORMAT_LINEAR, MODE_STEREO, 48000), &filter);
}

BOOST_AUTO_TEST_CASE(bass_redir)
{
  BassRedir filter;
  test_timing(Speakers(FORMAT_LINEAR, MODE_STEREO, 48000), &filter);
}

BOOST_AUTO_TEST_CASE(cache)
{
  CacheFilter filter;
  test_timing(Speakers(FORMAT_LINEAR, MODE_STEREO, 48000), &filter);
}

BOOST_AUTO_TEST_CASE(convert_linear2pcm)
{
  Converter filter(1024);
  filter.set_format(FORMAT_PCM16);
  test_timing(Speakers(FORMAT_LINEAR, MODE_STEREO, 48000), &filter);
}

BOOST_AUTO_TEST_CASE(convert_pcm2linear)
{
  Converter filter(1024);
  filter.set_format(FORMAT_LINEAR);
  test_timing(Speakers(FORMAT_PCM16, MODE_STEREO, 48000), &filter);
}

BOOST_AUTO_TEST_CASE(convolver)
{
  ParamFIR low_pass(ParamFIR::low_pass, 0.5, 0.0, 0.1, 100, true);
  Convolver filter(&low_pass);
  test_timing(Speakers(FORMAT_LINEAR, MODE_STEREO, 48000), &filter);
}

BOOST_AUTO_TEST_CASE(convolver_mch)
{
  ParamFIR low_pass(ParamFIR::low_pass, 0.5, 0.0, 0.1, 100, true);
  ConvolverMch filter;
  // Set only one channel filter (more complex case)
  filter.set_fir(CH_L, &low_pass);
  test_timing(Speakers(FORMAT_LINEAR, MODE_STEREO, 48000), &filter);
}

BOOST_AUTO_TEST_CASE(audio_decoder_ac3)
{
  AudioDecoder filter;
  test_timing(Speakers(FORMAT_RAWDATA, 0, 0), &filter, "a.ac3.03f.ac3", true);
}

BOOST_AUTO_TEST_CASE(decoder_graph_spdif)
{
  DecoderGraph filter;
  // DecoderGraph Does not pass BUFFERING test because it does a complex time
  // shift. Despdifer applies frame time shift and AudioProcessor does buffering.
  test_timing(Speakers(FORMAT_RAWDATA, 0, 0), &filter, "a.dts.03f.spdif", true,
    ALL_TESTS & ~BUFFERING);
}

BOOST_AUTO_TEST_CASE(delay)
{
  const float delays[CH_NAMES] = { 0, 1, 2, 3, 4, 5, 6, 7 };
  Delay filter;
  filter.set_units(DELAY_MS);
  filter.set_delays(delays);
  test_timing(Speakers(FORMAT_LINEAR, MODE_STEREO, 48000), &filter);
}

BOOST_AUTO_TEST_CASE(demux)
{
  Demux filter;
  test_timing(Speakers(FORMAT_PES, 0, 0), &filter, "a.ac3.03f.pes", true);
}

BOOST_AUTO_TEST_CASE(detector_pcm)
{
  Detector filter;
  test_timing(Speakers(FORMAT_PCM16, MODE_STEREO, 48000), &filter);
}

BOOST_AUTO_TEST_CASE(detector_spdif)
{
  Detector filter;
  test_timing(Speakers(FORMAT_RAWDATA, 0, 0), &filter, "a.dts.03f.spdif", true);
}

BOOST_AUTO_TEST_CASE(dvd_graph_pcm)
{
  DVDGraph filter;
  test_timing(Speakers(FORMAT_PCM16, MODE_STEREO, 48000), &filter);
}

BOOST_AUTO_TEST_CASE(frame_splitter_ac3)
{
  FrameSplitter filter(&ac3_header);
  test_timing(Speakers(FORMAT_RAWDATA, 0, 0), &filter, "a.ac3.03f.ac3", true);
}

BOOST_AUTO_TEST_CASE(gain)
{
  Gain filter;
  test_timing(Speakers(FORMAT_LINEAR, MODE_STEREO, 48000), &filter);
}

BOOST_AUTO_TEST_CASE(levels)
{
  Levels filter;
  test_timing(Speakers(FORMAT_LINEAR, MODE_STEREO, 48000), &filter);
}


BOOST_AUTO_TEST_CASE(mixer_inplace)
{
  Mixer filter(1024);
  filter.set_output(Speakers(FORMAT_LINEAR, MODE_STEREO, 48000));
  test_timing(Speakers(FORMAT_LINEAR, MODE_STEREO, 48000), &filter);
}

BOOST_AUTO_TEST_CASE(mixer_buffered)
{
  Mixer filter(1024);
  filter.set_output(Speakers(FORMAT_LINEAR, MODE_5_1, 48000));
  test_timing(Speakers(FORMAT_LINEAR, MODE_STEREO, 48000), &filter);
}

BOOST_AUTO_TEST_CASE(parser_filter_ac3)
{
  ParserFilter filter;
  AC3Parser ac3_parser;
  filter.add(&ac3_header, &ac3_parser);
  test_timing(Speakers(FORMAT_RAWDATA, 0, 0), &filter, "a.ac3.03f.ac3", true);
}

BOOST_AUTO_TEST_CASE(audio_processor)
{
  AudioProcessor filter(1024);
  filter.set_user(Speakers(FORMAT_PCM16, 0, 0));
  test_timing(Speakers(FORMAT_PCM16, MODE_STEREO, 48000), &filter, 0);
}

BOOST_AUTO_TEST_CASE(resample_up)
{
  Resample filter;
  filter.set_sample_rate(48000);
  // Resample does not pass FIRST_TIMESTAMP2 and BUFFERING tests
  // because it introduces time jitter in range of one sample
  test_timing(Speakers(FORMAT_LINEAR, MODE_STEREO, 44100), &filter, 0, false,
    ALL_TESTS & ~FIRST_TIMESTAMP2 & ~BUFFERING);
}

BOOST_AUTO_TEST_CASE(resample_down)
{
  Resample filter;
  filter.set_sample_rate(44100);
  // Resample does not pass FIRST_TIMESTAMP2 and BUFFERING test
  // because it introduces time jitter in range of one sample
  test_timing(Speakers(FORMAT_LINEAR, MODE_STEREO, 48000), &filter, 0, false,
    ALL_TESTS & ~FIRST_TIMESTAMP2 & ~BUFFERING);
}

BOOST_AUTO_TEST_CASE(spdifer_ac3)
{
  Spdifer filter;
  test_timing(Speakers(FORMAT_AC3, 0, 0), &filter, "a.ac3.03f.ac3", true);
}

BOOST_AUTO_TEST_CASE(despdifer)
{
  Despdifer filter;
  test_timing(Speakers(FORMAT_SPDIF, 0, 0), &filter, "a.ac3.03f.spdif", true);
}

BOOST_AUTO_TEST_CASE(spectrum)
{
  Spectrum filter;
  filter.set_length(1024);
  test_timing(Speakers(FORMAT_LINEAR, MODE_STEREO, 48000), &filter);
}

///////////////////////////////////////////////////////////
// Codecs
// Do not run test_first_timestamp3() because parsers must
// be feed with whole frames.

BOOST_AUTO_TEST_CASE(aac_adts_parser)
{
  ADTSParser filter;
  FileParser source;
  source.open_probe("a.aac.03f.adts", &adts_header);
  BOOST_REQUIRE(source.is_open());
  test_timing(&source, &filter, true, PARSER_TESTS);
}

BOOST_AUTO_TEST_CASE(aac_parser)
{
  AACParser filter;

  FileParser f;
  f.open("a.aac.03f.adts", &adts_header);
  BOOST_REQUIRE(f.is_open());

  ADTSParser adts;
  SourceFilter source(&f, &adts);

  Chunk chunk;
  BOOST_REQUIRE(source.get_chunk(chunk));
  BOOST_REQUIRE(filter.open(source.get_output()));

  // AACParser does not pass ttest_first_timestamp1() test because it swallows
  // the first frame.
  test_timing(&source, &filter, true, PARSER_TESTS & ~FIRST_TIMESTAMP1);
}

BOOST_AUTO_TEST_CASE(ac3_parser)
{
  AC3Parser filter;
  FileParser source;
  source.open_probe("a.ac3.03f.ac3", &ac3_header);
  BOOST_REQUIRE(source.is_open());
  test_timing(&source, &filter, true, PARSER_TESTS);
}

BOOST_AUTO_TEST_CASE(ac3_enc)
{
  AC3Enc filter;
  // AC3Enc does not pass FIRST_TIMESTAMP2 and BUFFERING test because it applies
  // time shift and it is hard to determine the correctness of this shift.
  test_timing(Speakers(FORMAT_LINEAR, MODE_STEREO, 48000), &filter, 0, false,
    ALL_TESTS & ~FIRST_TIMESTAMP2 & ~BUFFERING);
}

BOOST_AUTO_TEST_CASE(dts_parser)
{
  DTSParser filter;
  FileParser source;
  source.open_probe("a.dts.03f.dts", &dts_header);
  BOOST_REQUIRE(source.is_open());
  test_timing(&source, &filter, true, PARSER_TESTS);
}

BOOST_AUTO_TEST_CASE(mpa_parser)
{
  MPAParser filter;
  FileParser source;
  source.open_probe("a.mp2.005.mp2", &mpa_header);
  BOOST_REQUIRE(source.is_open());
  test_timing(&source, &filter, true, PARSER_TESTS);
}

BOOST_AUTO_TEST_CASE(mpg123_parser)
{
  MPG123Parser filter;
  FileParser source;
  source.open_probe("a.mp2.005.mp2", &mpa_header);
  BOOST_REQUIRE(source.is_open());
  test_timing(&source, &filter, true, PARSER_TESTS);
}

BOOST_AUTO_TEST_CASE(spdif_parser)
{
  SPDIFParser filter;
  FileParser source;
  source.open_probe("a.ac3.03f.spdif", &spdif_header);
  BOOST_REQUIRE(source.is_open());
  test_timing(&source, &filter, true, PARSER_TESTS);
}

BOOST_AUTO_TEST_CASE(spdif_wrapper)
{
  SPDIFWrapper filter;
  FileParser source;
  source.open_probe("a.ac3.03f.ac3", &ac3_header);
  BOOST_REQUIRE(source.is_open());
  test_timing(&source, &filter, true, PARSER_TESTS);
}

BOOST_AUTO_TEST_SUITE_END()
