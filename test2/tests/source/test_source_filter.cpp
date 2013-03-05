/*
  SourceFilter class test
*/

#include <boost/test/unit_test.hpp>
#include "source/generator.h"
#include "source/list_source.h"
#include "source/source_filter.h"
#include "filters/gain.h"
#include "filters/log_filter.h"
#include "../../suite.h"

static const int seed = 834753495;
static const size_t noise_size = 1024*1024; // for noise passthrough test
static uint8_t rawdata[] = { 0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7 };
static const ListSource::fchunk_list_t empty_list;

BOOST_AUTO_TEST_SUITE(source_filter)

BOOST_AUTO_TEST_CASE(default_constructor)
{
  Chunk chunk;
  SourceFilter src;
  BOOST_CHECK(src.get_source() == 0);
  BOOST_CHECK(src.get_filter() == 0);

  BOOST_CHECK_EQUAL(src.new_stream(), false);
  BOOST_CHECK_EQUAL(src.get_output().is_unknown(), true);
  BOOST_CHECK_EQUAL(src.get_chunk(chunk), false);
  BOOST_CHECK_NO_THROW(src.reset());
}

BOOST_AUTO_TEST_CASE(init_constructor)
{
  ListSource source;
  LogFilter filter;
  SourceFilter src(&source, &filter);

  BOOST_CHECK_EQUAL(src.get_source(), &source);
  BOOST_CHECK_EQUAL(src.get_filter(), &filter);

  BOOST_CHECK_EQUAL(src.new_stream(), false);
  BOOST_CHECK_EQUAL(src.get_output().is_unknown(), true);
}

BOOST_AUTO_TEST_CASE(set_release)
{
  const Speakers spk1(FORMAT_RAWDATA, 0, 0);
  const Speakers spk2(FORMAT_RAWDATA, MODE_STEREO, 48000);

  Chunk chunk;
  LogFilter filter;
  ListSource source;
  SourceFilter src;

  // Uninitialized source only (FORMAT_UNKNOWN)
  src.set(&source, 0);
  BOOST_CHECK(src.get_source() == &source);
  BOOST_CHECK(src.get_filter() == 0);
  BOOST_CHECK(!src.new_stream());
  BOOST_CHECK(src.get_output().is_unknown());

  // Uninitialized source (FORMAT_UNKNOWN) and closed filter
  src.set(&source, &filter);
  BOOST_CHECK(src.get_source() == &source);
  BOOST_CHECK(src.get_filter() == &filter);
  BOOST_CHECK(!src.new_stream());
  BOOST_CHECK(src.get_output().is_unknown());

  BOOST_CHECK(!filter.is_open());
  BOOST_CHECK_EQUAL(filter.print(), "");

  // Initialized source and closed filter
  source.set(spk1, empty_list);
  src.set(&source, &filter);
  BOOST_CHECK(src.get_source() == &source);
  BOOST_CHECK(src.get_filter() == &filter);
  BOOST_CHECK(!src.new_stream());
  BOOST_CHECK_EQUAL(src.get_output(), source.get_output());

  BOOST_CHECK(filter.is_open());
  BOOST_CHECK_EQUAL(filter.get_input(), source.get_output());
  BOOST_CHECK_EQUAL(filter.print(), "open(Raw data - 0)");
  filter.log.clear();

  // Empty source and open filter
  source.set(spk1, empty_list);
  src.set(&source, &filter);
  BOOST_CHECK(src.get_source() == &source);
  BOOST_CHECK(src.get_filter() == &filter);
  BOOST_CHECK(!src.new_stream());
  BOOST_CHECK_EQUAL(src.get_output(), source.get_output());

  BOOST_CHECK(filter.is_open());
  BOOST_CHECK_EQUAL(filter.get_input(), source.get_output());
  BOOST_CHECK_EQUAL(filter.print(), "");
  filter.log.clear();

  // Empty source and open filter with a different format
  filter.open(spk2);
  source.set(spk1, empty_list);
  src.set(&source, &filter);
  BOOST_CHECK(src.get_source() == &source);
  BOOST_CHECK(src.get_filter() == &filter);
  BOOST_CHECK(!src.new_stream());
  BOOST_CHECK_EQUAL(src.get_output(), source.get_output());

  BOOST_CHECK(filter.is_open());
  BOOST_CHECK_EQUAL(filter.get_input(), source.get_output());
  BOOST_CHECK_EQUAL(filter.print(), "open(Raw data Stereo 48000)\nopen(Raw data - 0)");
  filter.log.clear();

  // Release using set(0, 0)
  src.set(0, 0);
  BOOST_CHECK(src.get_source() == 0);
  BOOST_CHECK(src.get_filter() == 0);

  BOOST_CHECK_EQUAL(src.new_stream(), false);
  BOOST_CHECK_EQUAL(src.get_output().is_unknown(), true);
  BOOST_CHECK_EQUAL(src.get_chunk(chunk), false);
  BOOST_CHECK_NO_THROW(src.reset());
  BOOST_CHECK_EQUAL(filter.print(), "");

  // Release using release()
  src.set(&source, &filter);
  src.release();
  BOOST_CHECK(src.get_source() == 0);
  BOOST_CHECK(src.get_filter() == 0);

  BOOST_CHECK_EQUAL(src.new_stream(), false);
  BOOST_CHECK_EQUAL(src.get_output().is_unknown(), true);
  BOOST_CHECK_EQUAL(src.get_chunk(chunk), false);
  BOOST_CHECK_NO_THROW(src.reset());
  BOOST_CHECK_EQUAL(filter.print(), "");
}

BOOST_AUTO_TEST_CASE(reset)
{
  const Speakers spk1(FORMAT_RAWDATA, 0, 0);
  const Speakers spk2(FORMAT_RAWDATA, MODE_STEREO, 48000);

  ListSource source;
  LogFilter filter;
  SourceFilter src;

  // Do not reset if both filter and source are not open
  src.set(&source, &filter);
  src.reset();
  BOOST_CHECK_EQUAL(filter.print(), "");

  // Do not reset if filter is open and source is not
  // (filter will be open with the source / on format change)
  filter.open(spk1);
  filter.log.clear();
  src.reset();
  BOOST_CHECK_EQUAL(filter.print(), "");

  // Open a closed filter if source was open externally
  filter.close();
  filter.log.clear();
  source.set(spk1, empty_list);
  src.reset();
  BOOST_CHECK_EQUAL(filter.print(), "open(Raw data - 0)");

  // Reopen filter if source was open externally with a different format
  filter.log.clear();
  source.set(spk2, empty_list);
  src.reset();
  BOOST_CHECK_EQUAL(filter.print(), "open(Raw data Stereo 48000)");

  // Just reset if source and filter have equal formats
  filter.log.clear();
  src.reset();
  BOOST_CHECK_EQUAL(filter.print(), "reset()");
}

BOOST_AUTO_TEST_CASE(passthrough)
{
  Speakers spk(FORMAT_LINEAR, MODE_STEREO, 48000);
  
  Passthrough pass_filter;
  Gain        zero_filter(0.0);

  NoiseGen src_noise(spk, seed, noise_size);;
  NoiseGen ref_noise(spk, seed, noise_size);;
  ZeroGen  ref_zero(spk, noise_size);

  SourceFilter src;

  // Noise source == Noise source (no filter)
  src.set(&src_noise, 0);
  src.reset();
  ref_noise.reset();
  compare(&src, &ref_noise);

  // Noise source + Passthrough == Noise source
  src.set(&src_noise, &pass_filter);
  src.reset();
  ref_noise.reset();
  compare(&src, &ref_noise);

  // Noise source + ZeroFilter == Zero source
  src.set(&src_noise, &zero_filter);
  src.reset();
  ref_zero.reset();
  compare(&src, &ref_zero);
}

BOOST_AUTO_TEST_CASE(flushing)
{
  const Speakers spk1(FORMAT_RAWDATA, 0, 0);
  const Speakers spk2(FORMAT_RAWDATA, MODE_STEREO, 48000);

  Chunk chunk;
  LogFilter filter;
  ListSource source;
  SourceFilter src;

  // Uninitialized source and closed filter
  // Do nothing with the filter.
  src.set(&source, &filter);
  BOOST_CHECK(!src.get_chunk(chunk));
  BOOST_CHECK_EQUAL(filter.print(), "");
  filter.log.clear();

  // Uninitialized source and open filter
  // Flush the filter
  filter.open(spk1);
  filter.log.clear();
  src.set(&source, &filter);
  BOOST_CHECK(!src.get_chunk(chunk));
  BOOST_CHECK_EQUAL(filter.print(), "flush()");
  filter.log.clear();

  // Empty source and closed filter
  // Open the filter, do not flush
  source.set(spk1, empty_list);
  filter.close();
  filter.log.clear();
  src.set(&source, &filter);
  BOOST_CHECK(!src.get_chunk(chunk));
  BOOST_CHECK_EQUAL(filter.print(), "open(Raw data - 0)");
  filter.log.clear();

  // Empty source and open filter
  // Flush the filter
  source.set(spk1, empty_list);
  filter.open(spk1);
  filter.log.clear();
  src.set(&source, &filter);
  BOOST_CHECK(!src.get_chunk(chunk));
  BOOST_CHECK_EQUAL(filter.print(), "flush()");
  filter.log.clear();

  // Empty source and open filter with a different format
  // Open the filter, do not flush
  source.set(spk1, empty_list);
  filter.open(spk2);
  filter.log.clear();
  src.set(&source, &filter);
  BOOST_CHECK(!src.get_chunk(chunk));
  BOOST_CHECK_EQUAL(filter.print(), "open(Raw data - 0)");
  filter.log.clear();
}

BOOST_AUTO_TEST_CASE(process_source_only)
{
  Speakers spk;
  Chunk chunk;

  const FormatChangeChunk chunks[] =
  {
    FormatChangeChunk(Chunk(), true, Speakers(FORMAT_RAWDATA, 0, 0)),
    FormatChangeChunk(Chunk(rawdata, array_size(rawdata))),
    FormatChangeChunk(Chunk(rawdata, array_size(rawdata)), true, Speakers(FORMAT_RAWDATA, 0, 0)),
    FormatChangeChunk(),
    FormatChangeChunk(Chunk(rawdata, array_size(rawdata))),
  };

  ListSource source(Speakers(), chunks, array_size(chunks));
  SourceFilter src(&source, 0);

  for (int i = 0; i < array_size(chunks); i++)
  {
    BOOST_CHECK_EQUAL(src.get_chunk(chunk), true);
    BOOST_CHECK_EQUAL(chunk, chunks[i].chunk);
    BOOST_CHECK_EQUAL(src.new_stream(), chunks[i].new_stream);
    if (chunks[i].new_stream)
    {
      BOOST_CHECK_EQUAL(src.get_output(), chunks[i].spk);
      spk = src.get_output();
    }
    BOOST_CHECK_EQUAL(src.get_output(), spk);
  }
  BOOST_CHECK_EQUAL(src.get_chunk(chunk), false);
}

BOOST_AUTO_TEST_CASE(process_source_filter)
{
  Speakers spk;
  Chunk chunk;

  const FormatChangeChunk chunks[] =
  {
    // No empty chunks because filters do not pass them.
    FormatChangeChunk(Chunk(rawdata, array_size(rawdata)), true, Speakers(FORMAT_RAWDATA, 0, 0)),
    FormatChangeChunk(Chunk(rawdata, array_size(rawdata))),
    FormatChangeChunk(Chunk(rawdata, array_size(rawdata)), true, Speakers(FORMAT_RAWDATA, 0, 0)),
    FormatChangeChunk(Chunk(rawdata, array_size(rawdata))),
  };

  ListSource source(Speakers(), chunks, array_size(chunks));
  LogFilter filter;
  SourceFilter src(&source, &filter);

  for (int i = 0; i < array_size(chunks); i++)
  {
    BOOST_CHECK_EQUAL(src.get_chunk(chunk), true);
    BOOST_CHECK_EQUAL(chunk, chunks[i].chunk);
    BOOST_CHECK_EQUAL(src.new_stream(), chunks[i].new_stream);
    if (chunks[i].new_stream)
    {
      BOOST_CHECK_EQUAL(src.get_output(), chunks[i].spk);
      spk = src.get_output();
    }
    BOOST_CHECK_EQUAL(src.get_output(), spk);
  }
  BOOST_CHECK_EQUAL(src.get_chunk(chunk), false);

  string log = 
"open(Raw data - 0)\n"
"process(size = 8, rawdata)\n"
"process(size = 0)\n"
"process(size = 8, rawdata)\n"
"process(size = 0)\n"
"flush()\n"
"open(Raw data - 0)\n"
"process(size = 8, rawdata)\n"
"process(size = 0)\n"
"process(size = 8, rawdata)\n"
"process(size = 0)\n"
"flush()";

  BOOST_CHECK_EQUAL(filter.print(), log);
}

BOOST_AUTO_TEST_SUITE_END()
