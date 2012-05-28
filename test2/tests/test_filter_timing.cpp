/*
  Test correctness of timestamp handling
*/

#include <boost/test/unit_test.hpp>
#include "fir/param_fir.h"
#include "parsers/ac3/ac3_header.h"
#include "parsers/eac3/eac3_header.h"
#include "source/file_parser.h"
#include "source/generator.h"
#include "source/raw_source.h"
#include "source/source_filter.h"
#include "../all_filters.h"
#include "../noise_buf.h"

static const int seed = 9873457;
static const size_t noise_size = 1024*1024;
static const vtime_t time1 = 123;
static const vtime_t time2 = 321;

#define FIRST_TIMESTAMP1 1
#define FIRST_TIMESTAMP2 2
#define FIRST_TIMESTAMP3 4
#define BUFFERING        8

#define ALL_TESTS        (FIRST_TIMESTAMP1 | FIRST_TIMESTAMP2 | FIRST_TIMESTAMP3 | BUFFERING)
#define PARSER_TESTS     (FIRST_TIMESTAMP1 | FIRST_TIMESTAMP2)


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
  BOOST_MESSAGE("First timestamp test 1");

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
  BOOST_MESSAGE("First timestamp test 2");

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
  BOOST_MESSAGE("First timestamp test 3");

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
  BOOST_MESSAGE("Buffering test");

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

  Chunk in, out;
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
  while (!f->process(in, out))
    if (!src->get_chunk(in))
      BOOST_FAIL("Cannot fill the filter");

  BOOST_CHECK(out.sync);
  BOOST_CHECK_EQUAL(out.time, time1);

  // Get the second sync
  size_t delta_size = out.size;
  out.clear();
  while (!out.sync)
  {
    while (!f->process(in, out))
      if (!src->get_chunk(in))
        BOOST_FAIL("Cannot fill the filter");
    delta_size += out.size;
  }
  delta_size -= out.size;

  if (frame_sync)
    BOOST_CHECK_EQUAL(out.time, time2);
  else
  {
    vtime_t in_pos_time = size2time(f->get_input(), min_block_size);
    vtime_t out_pos_time = size2time(f->get_output(), delta_size);
    BOOST_CHECK(compare_time(out.time - time2, out_pos_time - in_pos_time));
  }
}

static void test_timing(Source *src, Filter *f, bool frame_sync = false, int tests = ALL_TESTS)
{
  if (!f->is_open())
    f->open(src->get_output());
  BOOST_REQUIRE(f->is_open());

  // Buffering test must be firest. Parser filters require this test to begin
  // with a frame boundary.
  if (tests & BUFFERING)        test_buffering(src, f, frame_sync);
  if (tests & FIRST_TIMESTAMP1) test_first_timestamp1(src, f);
  if (tests & FIRST_TIMESTAMP2) test_first_timestamp2(src, f, frame_sync);
  if (tests & FIRST_TIMESTAMP3) test_first_timestamp3(src, f);
}

static void test_timing(Speakers spk, Filter *f, const char *filename = 0, bool frame_sync = false, int tests = ALL_TESTS)
{
  BOOST_REQUIRE(f);
  BOOST_REQUIRE(f->open(spk));
  if (filename)
  {
    BOOST_MESSAGE("Testing on file: " << filename);
    RAWSource raw(spk, filename);
    BOOST_REQUIRE(raw.is_open());
    test_timing(&raw, f, frame_sync, tests);
  }
  else
  {
    BOOST_MESSAGE("Testing on noise");
    NoiseGen noise(spk, seed, noise_size);
    test_timing(&noise, f, frame_sync, tests);
  }
}

///////////////////////////////////////////////////////////////////////////////
// Parser timing test
// This test may be applied to FrameSplitter, ParserFilter and Detector.
//
// t1                                        t2        t3        t4        t5                      t6        t7        t8        none
// v                                         v         v         v         v                       v         v         v         V
// +-----------------------------------------+---------+---------+---------+-----------------------+---------+---------+---------+---------+
// |                  Chunk 1                | Chunk 2 | Chunk 3 | Chunk 4 |       Chunk 5         | Chunk 6 | Chunk 7 | Chunk 8 | Chunk 9 |
// +-----+---------+---------+---------+-----+---------+-----+---+---------+-+---------+---------+-+---------+---------+---------+---------+
// | *** | Frame 1 | Frame 2 | Frame 3 | *** | ******* | *** |    Frame 4    | Frame 5 | Frame 6 |       Frame 7       | Frame 8 | Frame 9 |
// +-----+---------+---------+---------+-----+---------+-----+---------------+---------+---------+---------------------+---------+---------+
//       ^         ^         ^                               ^               ^         ^         ^                     ^         ^
//       t1        none      none                            t3              t5        none      none                  t8        none
//
// Chunk N - input chunks
// Frame N - output chunks
// *** - Noise data at input, no output

static void parser_timing(Speakers spk, Filter *filter, const char *filename, FrameParser *frame_parser)
{
  BOOST_MESSAGE("Parser timing test on file: " << filename);

  const size_t noise_size = 1000;
  const int frames_required = 10;

  MemFile f(filename);
  BOOST_REQUIRE(f);

  // Find frame positions
  StreamBuffer stream;
  stream.set_parser(frame_parser);

  size_t frame_pos[frames_required];
  size_t pos = 0;
  size_t frames = 0;

  uint8_t *ptr = f;
  uint8_t *end = ptr + f.size();
  while (frames < frames_required)
  {
    BOOST_REQUIRE(stream.load(&ptr, end));

    if (stream.has_debris())
      pos += stream.get_debris_size();

    if (stream.has_frame())
    {
      frame_pos[frames++] = pos;
      pos += stream.get_frame_size();
    }
  }

  // Prepare test stream

  RawNoise noise(noise_size, seed);
  Rawdata test_buf(frame_pos[9] + 2 * noise_size);

  memcpy(test_buf, noise, noise_size);
  pos = noise_size;

  memcpy(test_buf + pos, f + frame_pos[0], frame_pos[3] - frame_pos[0]);
  pos += frame_pos[3] - frame_pos[0];

  memcpy(test_buf + pos, noise, noise_size);
  pos += noise_size;

  memcpy(test_buf + pos, f + frame_pos[3], frame_pos[9] - frame_pos[3]);

  // Prepare input chunks

  Chunk chunks[9];
  size_t chunk_size;

  chunk_size = noise_size + frame_pos[3] - frame_pos[0] + noise_size / 3;
  chunks[0].set_rawdata(test_buf, chunk_size, true, 0);
  pos = chunk_size;

  chunk_size = noise_size / 3;
  chunks[1].set_rawdata(test_buf + pos, chunk_size, true, 1000);
  pos += chunk_size;

  chunk_size = noise_size / 3 + (frame_pos[4] - frame_pos[3]) / 3;
  chunks[2].set_rawdata(test_buf + pos, chunk_size, true, 2000);
  pos += chunk_size;

  chunk_size = (frame_pos[4] - frame_pos[3]) / 3;
  chunks[3].set_rawdata(test_buf + pos, chunk_size, true, 3000);
  pos += chunk_size;

  chunk_size = (frame_pos[4] - frame_pos[3]) / 3 + (frame_pos[6] - frame_pos[4]) + (frame_pos[7] - frame_pos[6]) / 3;
  chunks[4].set_rawdata(test_buf + pos, chunk_size, true, 4000);
  pos += chunk_size;

  chunk_size = (frame_pos[7] - frame_pos[6]) / 3;
  chunks[5].set_rawdata(test_buf + pos, chunk_size, true, 5000);
  pos += chunk_size;

  chunk_size = frame_pos[7] + 2 * noise_size - pos;
  chunks[6].set_rawdata(test_buf + pos, chunk_size, true, 6000);
  pos += chunk_size;

  chunk_size = frame_pos[8] - frame_pos[7];
  chunks[7].set_rawdata(test_buf + pos, chunk_size, true, 7000);
  pos += chunk_size;

  chunk_size = frame_pos[9] - frame_pos[8];
  chunks[8].set_rawdata(test_buf + pos, chunk_size);

  // List of output chunks

  struct OutputChunk {
    bool new_stream;
    bool sync;
    vtime_t time;
  };
  
  OutputChunk parser_chunks[9] =
  {
    { true,  true,  0    },
    { false, false, 0    },
    { false, false, 0    },
    { true,  true,  2000 },
    { false, true,  4000 },
    { false, false, 0    },
    { false, false, 0    },
    { false, true,  7000 },
    { false, false, 0    }
  };

  OutputChunk detector_chunks[] =
  {
    { true,  false, 0    }, // Pre-frame junk
    { false, true,  0    }, // First frame
    { false, false, 0    }, // Frame 2
    { false, false, 0    }, // Frame 3
    { true,  false, 0    }, // Pre-frame junk of a new stream
    { false, true,  2000 }, // Frame 4
    { false, true,  4000 },
    { false, false, 0    },
    { false, false, 0    },
    { false, true,  7000 },
    { false, false, 0    }
  };

  // Process
  Chunk in, out;
  frames = 0;
  OutputChunk *out_chunks = parser_chunks;
  size_t n_out_chunks = array_size(parser_chunks);

  BOOST_REQUIRE(filter->open(spk));
  for (int i = 0; i < array_size(chunks); i++)
  {
    in = chunks[i];
    while (filter->process(in, out))
    {
      if (frames == 0 && !out.sync)
      {
        // It's Detector
        out_chunks = detector_chunks;
        n_out_chunks = array_size(detector_chunks);
      }

      BOOST_REQUIRE(frames < n_out_chunks);
      BOOST_CHECK_EQUAL(filter->new_stream(), out_chunks[frames].new_stream);
      BOOST_CHECK_EQUAL(out.sync, out_chunks[frames].sync);
      if (out.sync && out_chunks[frames].sync)
        BOOST_CHECK_EQUAL(out.time, out_chunks[frames].time);
      frames++;
    }
  }
  BOOST_CHECK_EQUAL(frames, n_out_chunks);
}

///////////////////////////////////////////////////////////////////////////////

BOOST_AUTO_TEST_SUITE(filter_timing)

BOOST_AUTO_TEST_CASE(agc)
{
  AGC filter;
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

BOOST_AUTO_TEST_CASE(decoder_graph_spdif)
{
  DecoderGraph filter;
  // DecoderGraph Does not pass BUFFERING test because it does a complex time
  // shift. Despdifer applies frame time shift and AudioProcessor does buffering.
  test_timing(Speakers(FORMAT_SPDIF, 0, 0), &filter, "a.dts.03f.spdif", true,
    ALL_TESTS & ~BUFFERING & ~FIRST_TIMESTAMP2);
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

BOOST_AUTO_TEST_CASE(drc)
{
  DRC filter;
  test_timing(Speakers(FORMAT_LINEAR, MODE_STEREO, 48000), &filter);
}

BOOST_AUTO_TEST_CASE(dvd_graph_pcm)
{
  DVDGraph filter;
  test_timing(Speakers(FORMAT_PCM16, MODE_STEREO, 48000), &filter);
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
  ADTSFrameParser frame_parser;
  ADTSParser filter;
  FileParser source;
  source.open_probe("a.aac.03f.adts", &frame_parser);
  BOOST_REQUIRE(source.is_open());
  test_timing(&source, &filter, true, PARSER_TESTS);
}

BOOST_AUTO_TEST_CASE(aac_parser)
{
  ADTSFrameParser frame_parser;
  AACParser filter;
  FileParser f;
  f.open("a.aac.03f.adts", &frame_parser);
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
  AC3FrameParser frame_parser;
  AC3Parser filter;
  FileParser source;
  source.open_probe("a.ac3.03f.ac3", &frame_parser);
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
  DTSFrameParser frame_parser;
  DTSParser filter;
  FileParser source;
  source.open_probe("a.dts.03f.dts", &frame_parser);
  BOOST_REQUIRE(source.is_open());
  test_timing(&source, &filter, true, PARSER_TESTS);
}

BOOST_AUTO_TEST_CASE(eac3_parser)
{
  EAC3FrameParser frame_parser;
  EAC3Parser filter;
  FileParser source;
  source.open_probe("test.eac3.03f.eac3", &frame_parser);
  BOOST_REQUIRE(source.is_open());
  test_timing(&source, &filter, true, PARSER_TESTS);
}

BOOST_AUTO_TEST_CASE(mpa_parser)
{
  MPAFrameParser frame_parser;
  MPAParser filter;
  FileParser source;
  source.open_probe("a.mp2.005.mp2", &frame_parser);
  BOOST_REQUIRE(source.is_open());
  test_timing(&source, &filter, true, PARSER_TESTS);
}

BOOST_AUTO_TEST_CASE(mpg123_parser)
{
  MPAFrameParser frame_parser;
  MPG123Parser filter;
  FileParser source;
  source.open_probe("a.mp2.005.mp2", &frame_parser);
  BOOST_REQUIRE(source.is_open());
  test_timing(&source, &filter, true, PARSER_TESTS);
}

BOOST_AUTO_TEST_CASE(spdif_parser)
{
  SPDIFFrameParser frame_parser;
  SPDIFParser filter;
  FileParser source;
  source.open_probe("a.ac3.03f.spdif", &frame_parser);
  BOOST_REQUIRE(source.is_open());
  test_timing(&source, &filter, true, PARSER_TESTS);
}

BOOST_AUTO_TEST_CASE(spdif_wrapper)
{
  AC3FrameParser frame_parser;
  SPDIFWrapper filter;
  FileParser source;
  source.open_probe("a.ac3.03f.ac3", &frame_parser);
  BOOST_REQUIRE(source.is_open());
  test_timing(&source, &filter, true, PARSER_TESTS);
}

///////////////////////////////////////////////////////////
// Parsers

BOOST_AUTO_TEST_CASE(frame_splitter)
{
  UniFrameParser uni;
  FrameSplitter filter(&uni);

  test_timing(Speakers(FORMAT_RAWDATA, 0, 0), &filter, "a.ac3.03f.ac3", true);
  test_timing(Speakers(FORMAT_RAWDATA, 0, 0), &filter, "a.aac.03f.adts", true);
  test_timing(Speakers(FORMAT_RAWDATA, 0, 0), &filter, "a.dts.03f.dts", true);
  test_timing(Speakers(FORMAT_RAWDATA, 0, 0), &filter, "test.eac3.03f.eac3", true);
  test_timing(Speakers(FORMAT_RAWDATA, 0, 0), &filter, "a.mp2.005.mp2", true);
  test_timing(Speakers(FORMAT_RAWDATA, 0, 0), &filter, "a.ac3.03f.spdif", true);
  test_timing(Speakers(FORMAT_RAWDATA, 0, 0), &filter, "a.dts.03f.spdif", true);
  test_timing(Speakers(FORMAT_RAWDATA, 0, 0), &filter, "a.mp2.005.spdif", true);

  parser_timing(Speakers(FORMAT_RAWDATA, 0, 0), &filter, "a.ac3.03f.ac3",   &AC3FrameParser());
  parser_timing(Speakers(FORMAT_RAWDATA, 0, 0), &filter, "a.aac.03f.adts",  &ADTSFrameParser());
  parser_timing(Speakers(FORMAT_RAWDATA, 0, 0), &filter, "a.dts.03f.dts",   &DTSFrameParser());
  parser_timing(Speakers(FORMAT_RAWDATA, 0, 0), &filter, "test.eac3.03f.eac3", &EAC3FrameParser());
  parser_timing(Speakers(FORMAT_RAWDATA, 0, 0), &filter, "a.mp2.005.mp2",   &MPAFrameParser());
  parser_timing(Speakers(FORMAT_RAWDATA, 0, 0), &filter, "a.ac3.03f.spdif", &SPDIFFrameParser());
  parser_timing(Speakers(FORMAT_RAWDATA, 0, 0), &filter, "a.dts.03f.spdif", &SPDIFFrameParser());
  parser_timing(Speakers(FORMAT_RAWDATA, 0, 0), &filter, "a.mp2.005.spdif", &SPDIFFrameParser());
}

BOOST_AUTO_TEST_CASE(parser_filter)
{
  UniFrameParser uni;

  ADTSParser  adts;
  AC3Parser   ac3;
  DTSParser   dts;
  EAC3Parser  eac3;
  MPAParser   mpa;
  SPDIFParser spdif;

  ParserFilter filter;
  filter.add(&uni.ac3,   &ac3);
  filter.add(&uni.adts,  &adts);
  filter.add(&uni.dts,   &dts);
  filter.add(&uni.eac3,  &eac3);
  filter.add(&uni.mpa,   &mpa);
  filter.add(&uni.spdif, &spdif);

  test_timing(Speakers(FORMAT_AC3,   0, 0), &filter, "a.ac3.03f.ac3", true);
  test_timing(Speakers(FORMAT_AAC_ADTS, 0, 0), &filter, "a.aac.03f.adts", true);
  test_timing(Speakers(FORMAT_DTS,   0, 0), &filter, "a.dts.03f.dts", true);
  test_timing(Speakers(FORMAT_EAC3,  0, 0), &filter, "test.eac3.03f.eac3", true);
  test_timing(Speakers(FORMAT_MPA,   0, 0), &filter, "a.mp2.005.mp2", true);
  test_timing(Speakers(FORMAT_SPDIF, 0, 0), &filter, "a.ac3.03f.spdif", true);
  test_timing(Speakers(FORMAT_SPDIF, 0, 0), &filter, "a.dts.03f.spdif", true);
  test_timing(Speakers(FORMAT_SPDIF, 0, 0), &filter, "a.mp2.005.spdif", true);

  parser_timing(Speakers(FORMAT_AC3,   0, 0), &filter, "a.ac3.03f.ac3",   &AC3FrameParser());
  parser_timing(Speakers(FORMAT_AAC_ADTS, 0, 0), &filter, "a.aac.03f.adts",  &ADTSFrameParser());
  parser_timing(Speakers(FORMAT_DTS,   0, 0), &filter, "a.dts.03f.dts",   &DTSFrameParser());
  parser_timing(Speakers(FORMAT_EAC3,  0, 0), &filter, "test.eac3.03f.eac3", &EAC3FrameParser());
  parser_timing(Speakers(FORMAT_MPA,   0, 0), &filter, "a.mp2.005.mp2",   &MPAFrameParser());
  parser_timing(Speakers(FORMAT_SPDIF, 0, 0), &filter, "a.ac3.03f.spdif", &SPDIFFrameParser());
  parser_timing(Speakers(FORMAT_SPDIF, 0, 0), &filter, "a.dts.03f.spdif", &SPDIFFrameParser());
  parser_timing(Speakers(FORMAT_SPDIF, 0, 0), &filter, "a.mp2.005.spdif", &SPDIFFrameParser());
}

BOOST_AUTO_TEST_CASE(audio_decoder)
{
  AudioDecoder filter;

  test_timing(Speakers(FORMAT_AC3,  0, 0), &filter, "a.ac3.03f.ac3", true);
  // AAC/ADTS is not supported by AudioDecoder because
  // it requires an additional demuxer (ADTSParser)
  //test_timing(Speakers(FORMAT_RAWDATA, 0, 0), &filter, "a.aac.03f.adts", true);
  test_timing(Speakers(FORMAT_DTS,  0, 0), &filter, "a.dts.03f.dts", true);
  test_timing(Speakers(FORMAT_EAC3, 0, 0), &filter, "test.eac3.03f.eac3", true);
  test_timing(Speakers(FORMAT_MPA,  0, 0), &filter, "a.mp2.005.mp2", true);

  parser_timing(Speakers(FORMAT_AC3,  0, 0), &filter, "a.ac3.03f.ac3",  &AC3FrameParser());
  //parser_timing(Speakers(FORMAT_RAWDATA, 0, 0), &filter, "a.aac.03f.adts", &ADTSFrameParser());
  parser_timing(Speakers(FORMAT_DTS,  0, 0), &filter, "a.dts.03f.dts",  &DTSFrameParser());
  parser_timing(Speakers(FORMAT_EAC3, 0, 0), &filter, "test.eac3.03f.eac3", &EAC3FrameParser());
  parser_timing(Speakers(FORMAT_MPA,  0, 0), &filter, "a.mp2.005.mp2",  &MPAFrameParser());
}

BOOST_AUTO_TEST_CASE(detector)
{
  // Detector does not pass test_first_timestamp1() and test_first_timestamp3()
  // because it does not stamp first chunk filled with pre-frame junk.
  const int tests = ALL_TESTS & ~FIRST_TIMESTAMP1 & ~FIRST_TIMESTAMP3;
  Detector filter;

  test_timing(Speakers(FORMAT_PCM16, MODE_STEREO, 48000), &filter);
  test_timing(Speakers(FORMAT_RAWDATA, 0, 0), &filter, "a.ac3.03f.ac3",   true, tests);
  test_timing(Speakers(FORMAT_RAWDATA, 0, 0), &filter, "a.aac.03f.adts",  true, tests);
  test_timing(Speakers(FORMAT_RAWDATA, 0, 0), &filter, "a.dts.03f.dts",   true, tests);
  test_timing(Speakers(FORMAT_RAWDATA, 0, 0), &filter, "test.eac3.03f.eac3", true, tests);
  test_timing(Speakers(FORMAT_RAWDATA, 0, 0), &filter, "a.mp2.005.mp2",   true, tests);
  test_timing(Speakers(FORMAT_RAWDATA, 0, 0), &filter, "a.ac3.03f.spdif", true, tests);
  test_timing(Speakers(FORMAT_RAWDATA, 0, 0), &filter, "a.dts.03f.spdif", true, tests);
  test_timing(Speakers(FORMAT_RAWDATA, 0, 0), &filter, "a.mp2.005.spdif", true, tests);

  parser_timing(Speakers(FORMAT_RAWDATA, 0, 0), &filter, "a.ac3.03f.ac3",   &AC3FrameParser());
  parser_timing(Speakers(FORMAT_RAWDATA, 0, 0), &filter, "a.aac.03f.adts",  &ADTSFrameParser());
  parser_timing(Speakers(FORMAT_RAWDATA, 0, 0), &filter, "a.dts.03f.dts",   &DTSFrameParser());
  parser_timing(Speakers(FORMAT_RAWDATA, 0, 0), &filter, "test.eac3.03f.eac3", &EAC3FrameParser());
  parser_timing(Speakers(FORMAT_RAWDATA, 0, 0), &filter, "a.mp2.005.mp2",   &MPAFrameParser());
  parser_timing(Speakers(FORMAT_RAWDATA, 0, 0), &filter, "a.ac3.03f.spdif", &SPDIFFrameParser());
  parser_timing(Speakers(FORMAT_RAWDATA, 0, 0), &filter, "a.dts.03f.spdif", &SPDIFFrameParser());
  parser_timing(Speakers(FORMAT_RAWDATA, 0, 0), &filter, "a.mp2.005.spdif", &SPDIFFrameParser());
}

BOOST_AUTO_TEST_SUITE_END()
