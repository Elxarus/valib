/*
  SyncTrie and SyncScan test.
*/

#include "syncscan.h"
#include <boost/test/unit_test.hpp>

static const int seed = 958745023;
static const int noise_size = 1*1024*1024;
using std::string;

BOOST_AUTO_TEST_SUITE(sync_trie)

BOOST_AUTO_TEST_CASE(constructor)
{
  SyncTrie t;
  BOOST_CHECK(t.is_empty());
  BOOST_CHECK(t.sync_size() == 0);
  BOOST_CHECK(t.serialize() == string());
  BOOST_CHECK(t.get_depth() == 0);
  BOOST_CHECK(t.get_size() == 0);
}

BOOST_AUTO_TEST_CASE(init_constructor1)
{
  uint8_t good_sync[1] = { 0x55 };
  uint8_t bad_sync[1] = { 0x54 };
  SyncTrie t(0x55, 8);

  BOOST_CHECK(!t.is_empty());
  BOOST_CHECK(t.sync_size() == 1);
  BOOST_CHECK(t.is_sync(good_sync));
  BOOST_CHECK(!t.is_sync(bad_sync));
  BOOST_CHECK(t.serialize() == string("oioioioI"));
  BOOST_CHECK(t.get_depth() == 8);
  BOOST_CHECK(t.get_size() == 8);
}

BOOST_AUTO_TEST_CASE(init_constructor2)
{
  uint8_t good_sync[1] = { 0x55 };
  uint8_t bad_sync[1] = { 0x54 };
  SyncTrie t("oioioioI");

  BOOST_CHECK(!t.is_empty());
  BOOST_CHECK(t.sync_size() == 1);
  BOOST_CHECK(t.is_sync(good_sync));
  BOOST_CHECK(!t.is_sync(bad_sync));
  BOOST_CHECK(t.serialize() == string("oioioioI"));
  BOOST_CHECK(t.get_depth() == 8);
  BOOST_CHECK(t.get_size() == 8);
}

BOOST_AUTO_TEST_CASE(init_constructor3)
{
  uint8_t good_sync1[1] = { 0x55 };
  uint8_t good_sync2[1] = { 0xaa };
  SyncTrie t(8);

  BOOST_CHECK(!t.is_empty());
  BOOST_CHECK(t.sync_size() == 1);
  BOOST_CHECK(t.is_sync(good_sync1));
  BOOST_CHECK(t.is_sync(good_sync2));
  BOOST_CHECK(t.serialize() == string("xxxxxxxA"));
  BOOST_CHECK(t.get_depth() == 8);
  BOOST_CHECK(t.get_size() == 8);
}

BOOST_AUTO_TEST_CASE(clear)
{
  SyncTrie t(0x55, 8);
  t.clear();

  BOOST_CHECK(t.is_empty());
  BOOST_CHECK(t.sync_size() == 0);
  BOOST_CHECK(t.serialize() == string());
  BOOST_CHECK(t.get_depth() == 0);
  BOOST_CHECK(t.get_size() == 0);
}

BOOST_AUTO_TEST_CASE(invert)
{
  static const string trie("oix*R*OIL*AD");
  static bool trie_test[256] = 
  {
    // 00xxxxxx - false [0-64)
    false, false, false, false, false, false, false, false, 
    false, false, false, false, false, false, false, false, 
    false, false, false, false, false, false, false, false, 
    false, false, false, false, false, false, false, false, 
    false, false, false, false, false, false, false, false, 
    false, false, false, false, false, false, false, false, 
    false, false, false, false, false, false, false, false, 
    false, false, false, false, false, false, false, false, 
    // 01000xxx - true
    true,  true,  true,  true,  true,  true,  true,  true,
    // 0100100x - true
    true,  true,
    // 0100101x - false
    false, false, 
    // 0100110x - false
    false, false, 
    // 0100111x - true
    true,  true,
    // 010100xx - true
    true,  true,  true,  true,
    // 010101xx - false
    false, false, false, false,
    // 01011xxx - true
    true,  true,  true,  true,  true,  true,  true,  true,
    // 01100xxx - true
    true,  true,  true,  true,  true,  true,  true,  true,
    // 0110100x - true
    true,  true,
    // 0110101x - false
    false, false, 
    // 0110110x - false
    false, false, 
    // 0110111x - true
    true,  true,
    // 011100xx - true
    true,  true,  true,  true,
    // 011101xx - false
    false, false, false, false,
    // 01111xxx - true
    true,  true,  true,  true,  true,  true,  true,  true,
    // 1xxxxxxx - false [127-256)
    false, false, false, false, false, false, false, false, 
    false, false, false, false, false, false, false, false, 
    false, false, false, false, false, false, false, false, 
    false, false, false, false, false, false, false, false, 
    false, false, false, false, false, false, false, false, 
    false, false, false, false, false, false, false, false, 
    false, false, false, false, false, false, false, false, 
    false, false, false, false, false, false, false, false, 
    false, false, false, false, false, false, false, false, 
    false, false, false, false, false, false, false, false, 
    false, false, false, false, false, false, false, false, 
    false, false, false, false, false, false, false, false, 
    false, false, false, false, false, false, false, false, 
    false, false, false, false, false, false, false, false, 
    false, false, false, false, false, false, false, false, 
    false, false, false, false, false, false, false, false, 
  };

  SyncTrie t(trie);
  t.invert();
  for (int i = 0; i < array_size(trie_test); i++)
  {
    uint8_t sync[1];
    sync[0] = (uint8_t) i;
    if (t.is_sync(sync) == trie_test[i])
      BOOST_FAIL("value: " << i << " result must be: " << !trie_test[i]);
  }
}

BOOST_AUTO_TEST_CASE(merge)
{
  static struct {
    const char *left_trie;
    const char *right_trie;
    const char *result_trie;
    size_t result_size;
    size_t result_depth;
  } tests[] =
  {
    { "O", "O",  "O",  1, 1 },
    { "O", "I",  "A",  1, 1 },
    { "O", "A",  "A",  1, 1 },
    { "O", "D",  "O",  1, 1 },
    { "O", "oI", "O",  1, 1 },
    { "O", "iI", "RI", 2, 2 },
    { "O", "xI", "RI", 2, 2 },
    { "O", "RI", "RI", 2, 2 },
    { "O", "LI", "A",  1, 1 },
    { "O", "*II", "RI", 2, 2 },
    { "I", "I",  "I",  1, 1 },
    { "I", "A",  "A",  1, 1 },
    { "I", "D",  "I",  1, 1 },
    { "I", "oI", "LI", 2, 2 },
    { "I", "iI", "I",  1, 1 },
    { "I", "xI", "LI", 2, 2 },
    { "I", "RI", "A",  1, 1 },
    { "I", "LI", "LI", 2, 2 },
    { "I", "*II", "LI", 2, 2 },
    { "A", "A",  "A",  1, 1 },
    { "A", "D",  "A",  1, 1 },
    { "D", "D",  "D",  1, 1 },
  };

  SyncTrie t;

  // Merge an empty trie with another empty one
  t.merge(SyncTrie());
  BOOST_CHECK(t.serialize() == string());
  BOOST_CHECK(t.is_empty());
  BOOST_CHECK(t.sync_size() == 0);
  BOOST_CHECK(t.get_depth() == 0);
  BOOST_CHECK(t.get_size() == 0);

  // Merge an empty trie with non-empty one
  t.merge(SyncTrie("I"));
  BOOST_CHECK_EQUAL(t.serialize(), string("I"));
  BOOST_CHECK(!t.is_empty());
  BOOST_CHECK(t.sync_size() == 1);
  BOOST_CHECK(t.get_depth() == 1);
  BOOST_CHECK(t.get_size() == 1);

  // Merge non-empty trie with an empty one
  t.merge(SyncTrie());
  BOOST_CHECK_EQUAL(t.serialize(), string("I"));
  BOOST_CHECK(!t.is_empty());
  BOOST_CHECK(t.sync_size() == 1);
  BOOST_CHECK(t.get_depth() == 1);
  BOOST_CHECK(t.get_size() == 1);

  // Depth of the merge must be the max depth of both
  t = SyncTrie(0x55, 8);
  t.merge(SyncTrie(0xaaaa, 16));
  BOOST_CHECK_EQUAL(t.get_depth(), 16);

  t = SyncTrie(0xaaaa, 16);
  t.merge(SyncTrie(0x55, 8));
  BOOST_CHECK_EQUAL(t.get_depth(), 16);

  // Run test cases
  for (size_t i = 0; i < array_size(tests); i++)
  {
    t = SyncTrie(tests[i].left_trie);
    t.merge(SyncTrie(tests[i].right_trie));
    BOOST_MESSAGE(tests[i].left_trie << " or " << tests[i].right_trie << " = " << tests[i].result_trie);
    BOOST_CHECK_EQUAL(t.serialize(), tests[i].result_trie);
    BOOST_CHECK_EQUAL(t.get_depth(), tests[i].result_depth);
    BOOST_CHECK_EQUAL(t.get_size(), tests[i].result_size);
  }
}

BOOST_AUTO_TEST_SUITE_END()
