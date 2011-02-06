/*
  FileParser class test
*/

#include <boost/test/unit_test.hpp>
#include "parsers/ac3/ac3_header.h"
#include "parsers/dts/dts_header.h"
#include "source/file_parser.h"
#include "source/raw_source.h"
#include "../../suite.h"

BOOST_AUTO_TEST_SUITE(file_parser)

BOOST_AUTO_TEST_CASE(constructor)
{
  FileParser f;
  BOOST_CHECK(!f.is_open());
}

BOOST_AUTO_TEST_CASE(open)
{
  bool result;
  const char *absent_file = "no-such-file";
  const char *filename = "a.ac3.005.ac3";
  const HeaderParser *parser = &ac3_header;
  FileParser f;

  // Do not open absent file
  result = f.open(absent_file, parser);
  BOOST_CHECK(!result);
  BOOST_CHECK(!f.is_open());

  // Do not open file without a parser
  result = f.open(filename, 0);
  BOOST_CHECK(!result);
  BOOST_CHECK(!f.is_open());

  // Open file correctly
  result = f.open(filename, parser);
  BOOST_CHECK(result);
  BOOST_CHECK(f.is_open());
  BOOST_CHECK(!f.eof());
  BOOST_CHECK_EQUAL(f.get_pos(), 0);
  BOOST_CHECK_EQUAL(f.get_filename(), string(filename));
  BOOST_CHECK_EQUAL(f.get_parser(), &ac3_header);
  BOOST_CHECK(f.get_output().is_unknown());

  // Close file
  f.close();
  BOOST_CHECK(!f.is_open());

  // Open good file, then wrong file without closing
  // File must be closed.
  result = f.open(filename, parser);
  BOOST_CHECK(result);
  result = f.open(absent_file, 0);
  BOOST_CHECK(!result);
  BOOST_CHECK(!f.is_open());
}

BOOST_AUTO_TEST_CASE(probe)
{
  Speakers spk(FORMAT_AC3, MODE_STEREO, 48000);
  const char *filename = "a.ac3.005.ac3";
  const HeaderParser *parser = &ac3_header;
  const HeaderParser *wrong_parser = &dts_header;
  bool result;

  FileParser f;

  // Test with correct parser
  result = f.open(filename, parser);
  BOOST_REQUIRE(result);
  result = f.probe();
  BOOST_CHECK(result);
  BOOST_CHECK(f.get_output() == spk);

  // Test with wrong parser
  result = f.open(filename, wrong_parser);
  BOOST_REQUIRE(result);
  result = f.probe();
  BOOST_CHECK(!result);
}

BOOST_AUTO_TEST_CASE(stats)
{
  bool result;
  const double precision = 0.5;
  struct {
    const char *filename;
    const HeaderParser *parser;
    int frames;
    vtime_t duration;
  } files[] = {
    { "a.ac3.03f.ac3", &ac3_header, 375, 12.0 },  // CBR
    { "a.ac3.mix.ac3", &ac3_header, 1500, 48.0 }, // VBR
  };

  FileParser f;
  for (int i = 0; i < array_size(files); i++)
  {
    // Statistical values must be zero after open
    result = f.open(files[i].filename, files[i].parser);
    BOOST_REQUIRE(result);
    BOOST_CHECK_EQUAL(f.get_avg_frame_interval(), 0.0);
    BOOST_CHECK_EQUAL(f.get_avg_bitrate(), 0.0);

    // Statistical values must be set after stat()
    result = f.stats(precision, 10, 1000);
    BOOST_REQUIRE(result);
    BOOST_CHECK_GT(f.get_avg_frame_interval(), 0.0);
    BOOST_CHECK_GT(f.get_avg_bitrate(), 0.0);

    // Check file size in frames and secs.
    double frames_error = precision * f.get_avg_bitrate() / f.get_avg_frame_interval();
    BOOST_CHECK_LE(fabs(f.get_size(FileParser::frames) - files[i].frames), frames_error);
    BOOST_CHECK_LE(fabs(f.get_size(FileParser::time) - files[i].duration), precision);
  }
}

BOOST_AUTO_TEST_CASE(positioning)
{
  bool result;
  const string filename = "a.ac3.03f.ac3";
  const HeaderParser *parser = &ac3_header;
  FileParser::fsize_t pos = 1000;

  FileParser f;
  result = f.open(filename, parser) && f.stats();
  BOOST_REQUIRE(result);

  // After open position must be zero
  BOOST_CHECK_EQUAL(f.get_pos(), 0);

  // get_pos() with all possible units
  f.seek(pos);
  BOOST_CHECK_EQUAL(f.get_pos(), pos);
  BOOST_CHECK_EQUAL(f.get_pos(FileParser::bytes), pos);
  BOOST_CHECK_CLOSE(f.get_pos(FileParser::relative), double(pos)/f.get_size(), 1e-5);
  BOOST_CHECK_CLOSE(f.get_pos(FileParser::frames), double(pos)/f.get_avg_frame_interval(), 1e-5);
  BOOST_CHECK_CLOSE(f.get_pos(FileParser::time), double(pos)*8/f.get_avg_bitrate(), 1e-5);

  // Seek using bytes
  f.seek(double(pos), FileParser::bytes);
  BOOST_CHECK_EQUAL(f.get_pos(), pos);

  // Seek relative
  f.seek(double(pos)/f.get_size(), FileParser::relative);
  BOOST_CHECK_EQUAL(f.get_pos(), pos);

  // Seek using frames
  f.seek(double(pos)/f.get_avg_frame_interval(), FileParser::frames);
  BOOST_CHECK_EQUAL(f.get_pos(), pos);

  // Seek using time
  f.seek(double(pos)*8/f.get_avg_bitrate(), FileParser::time);
  BOOST_CHECK_EQUAL(f.get_pos(), pos);
}

// Test max_scan option effect.
BOOST_AUTO_TEST_CASE(max_scan)
{
  bool result;
  const string filename = "a.mad.mix.mad";
  const HeaderParser *parser = &dts_header;
  size_t max_scan_fail = 100000;
  size_t max_scan_pass = 1000000;

  FileParser f;
  f.open(filename, parser, max_scan_fail);
  result = f.probe();
  BOOST_CHECK(!result);

  f.open(filename, parser, max_scan_pass);
  result = f.probe();
  BOOST_CHECK(result);

  f.open(filename, parser, 0);
  result = f.probe();
  BOOST_CHECK(result);
}

// Check new_stream() behavior after probe() and seek()
BOOST_AUTO_TEST_CASE(new_stream)
{
  bool result;
  Chunk chunk;
  const string filename = "a.ac3.03f.ac3";
  const HeaderParser *parser = &ac3_header;
  const Speakers spk(FORMAT_AC3, MODE_5_1, 48000);

  FileParser f;

  // new_stream() returns true after open()
  result = f.open(filename, parser);
  BOOST_REQUIRE(result);
  BOOST_CHECK(f.get_output().is_unknown());

  result = f.get_chunk(chunk);
  BOOST_CHECK(result);
  BOOST_CHECK(f.new_stream());
  BOOST_CHECK(f.get_output() == spk);

  // new_stream() returns false after probe()
  result = f.open_probe(filename, parser);
  BOOST_REQUIRE(result);
  BOOST_CHECK(f.get_output() == spk);

  result = f.get_chunk(chunk);
  BOOST_CHECK(result);
  BOOST_CHECK(!f.new_stream());
  BOOST_CHECK(f.get_output() == spk);

  // new_stream() returns true after seek()
  f.seek(1000);
  BOOST_CHECK(f.get_output().is_unknown());

  result = f.get_chunk(chunk);
  BOOST_CHECK(result);
  BOOST_CHECK(f.new_stream());
  BOOST_CHECK(f.get_output() == spk);
}

// Read frames and compare with raw file
BOOST_AUTO_TEST_CASE(passthrough)
{
  bool result;
  const string filename = "a.ac3.03f.ac3";
  const HeaderParser *parser = &ac3_header;

  FileParser f;
  RAWSource raw;

  result = f.open(filename, parser);
  BOOST_REQUIRE(result);

  result = raw.open(Speakers(FORMAT_RAWDATA, 0, 0), filename.c_str());
  BOOST_REQUIRE(result);

  compare(&f, &raw);
}

// Read frames and compare with raw file
BOOST_AUTO_TEST_CASE(open_probe_passthrough)
{
  bool result;
  const string filename = "a.ac3.03f.ac3";
  const HeaderParser *parser = &ac3_header;

  FileParser f;
  RAWSource raw;

  result = f.open_probe(filename, parser);
  BOOST_REQUIRE(result);

  result = raw.open(Speakers(FORMAT_RAWDATA, 0, 0), filename.c_str());
  BOOST_REQUIRE(result);

  compare(&f, &raw);
}

BOOST_AUTO_TEST_CASE(format_change)
{
  bool result;
  Chunk chunk;
  const string filename = "a.ac3.mix.ac3";
  const HeaderParser *parser = &ac3_header;
  struct {
    Speakers spk;
    size_t frames;
  } streams[] = {
    { Speakers(FORMAT_AC3, MODE_5_1, 48000), 750 },
    { Speakers(FORMAT_AC3, MODE_STEREO, 48000), 375 },
    { Speakers(FORMAT_AC3, MODE_5_1, 48000), 375 },
  };

  FileParser f;
  result = f.open(filename, parser);
  BOOST_REQUIRE(result);

  size_t frame_count = 0;
  size_t stream_count = 0;
  while (f.get_chunk(chunk))
  {
    if (f.new_stream())
    {
      BOOST_CHECK(f.get_output() == streams[stream_count].spk);
      if (stream_count > 0)
        BOOST_CHECK_EQUAL(frame_count, streams[stream_count-1].frames);
      stream_count++;
      frame_count = 0;
    }
    frame_count++;
  }

  BOOST_CHECK_EQUAL(frame_count, streams[stream_count-1].frames);
  BOOST_CHECK_EQUAL(stream_count, array_size(streams));
}

BOOST_AUTO_TEST_SUITE_END()
