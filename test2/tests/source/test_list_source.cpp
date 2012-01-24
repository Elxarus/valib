/*
  ListSource class test
*/

#include <boost/test/unit_test.hpp>
#include "source/list_source.h"
#include "../../suite.h"

static const Speakers init_spk(FORMAT_RAWDATA, 0, 0);

static uint8_t rawdata[] = { 0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7 };

static const Chunk chunks[] = {
  Chunk(true, 100),
  Chunk(),
  Chunk(rawdata, array_size(rawdata)),
};

static const ListSource::chunk_list_t list(chunks, chunks + array_size(chunks));

static const FormatChangeChunk fchunks[] = {
  FormatChangeChunk(Chunk(true, 100)),
  FormatChangeChunk(),
  FormatChangeChunk(Chunk(rawdata, array_size(rawdata))),
  FormatChangeChunk(Chunk(), true, Speakers(FORMAT_RAWDATA, MODE_STEREO, 48000)),
};

static const ListSource::fchunk_list_t flist(fchunks, fchunks + array_size(fchunks));



BOOST_AUTO_TEST_SUITE(list_source)

BOOST_AUTO_TEST_CASE(default_constructor)
{
  ListSource src;
  BOOST_CHECK_EQUAL(src.get_output(), Speakers());
  BOOST_CHECK_EQUAL(src.new_stream(), false);
  BOOST_CHECK_EQUAL(src.get_list().size(), 0);
  BOOST_CHECK_EQUAL(src.get_pos(), 0);
}

BOOST_AUTO_TEST_CASE(init_constructor1)
{
  ListSource src(init_spk, chunks, array_size(chunks));
  BOOST_CHECK_EQUAL(src.get_output(), init_spk);
  BOOST_CHECK_EQUAL(src.new_stream(), false);
  BOOST_CHECK_EQUAL(src.get_list().size(), array_size(chunks));
  BOOST_CHECK_EQUAL(src.get_pos(), 0);
}

BOOST_AUTO_TEST_CASE(init_constructor2)
{
  ListSource src(init_spk, list);
  BOOST_CHECK_EQUAL(src.get_output(), init_spk);
  BOOST_CHECK_EQUAL(src.new_stream(), false);
  BOOST_CHECK_EQUAL(src.get_list().size(), list.size());
  BOOST_CHECK_EQUAL(src.get_pos(), 0);
}

BOOST_AUTO_TEST_CASE(init_constructor3)
{
  ListSource src(init_spk, fchunks, array_size(fchunks));
  BOOST_CHECK_EQUAL(src.get_output(), init_spk);
  BOOST_CHECK_EQUAL(src.new_stream(), false);
  BOOST_CHECK_EQUAL(src.get_list().size(), array_size(fchunks));
  BOOST_CHECK_EQUAL(src.get_pos(), 0);
}

BOOST_AUTO_TEST_CASE(init_constructor4)
{
  ListSource src(init_spk, flist);
  BOOST_CHECK_EQUAL(src.get_output(), init_spk);
  BOOST_CHECK_EQUAL(src.new_stream(), false);
  BOOST_CHECK_EQUAL(src.get_list().size(), flist.size());
  BOOST_CHECK_EQUAL(src.get_pos(), 0);
}

BOOST_AUTO_TEST_CASE(process_chunks)
{
  Chunk chunk;
  ListSource src(init_spk, list);
  for (size_t i = 0; i < list.size(); i++)
  {
    BOOST_CHECK_EQUAL(src.get_pos(), i);
    BOOST_CHECK_EQUAL(src.get_chunk(chunk), true);
    BOOST_CHECK_EQUAL(chunk, list[i]);
    BOOST_CHECK_EQUAL(src.new_stream(), false);
    BOOST_CHECK_EQUAL(src.get_output(), init_spk);
  }
}

BOOST_AUTO_TEST_CASE(process_fchunks)
{
  Chunk chunk;
  ListSource src(init_spk, flist);

  Speakers spk = init_spk;
  for (size_t i = 0; i < flist.size(); i++)
  {
    BOOST_CHECK_EQUAL(src.get_pos(), i);
    BOOST_CHECK_EQUAL(src.get_chunk(chunk), true);
    BOOST_CHECK_EQUAL(chunk, flist[i].chunk);
    BOOST_CHECK_EQUAL(src.new_stream(), flist[i].new_stream);
    if (src.new_stream())
    {
      BOOST_CHECK_EQUAL(src.get_output(), fchunks[i].spk);
      spk = src.get_output();
    }
    BOOST_CHECK_EQUAL(src.get_output(), spk);
  }
}

BOOST_AUTO_TEST_SUITE_END()
