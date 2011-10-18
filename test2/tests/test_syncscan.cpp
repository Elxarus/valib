/*
  SyncTrie and SyncScan test.
*/

#include <boost/test/unit_test.hpp>
#include "../noise_buf.h"
#include "syncscan.h"

using std::string;

static const int seed = 98374592;
static const size_t noise_size = 1024; // for sync scan test

// Test trie that contains all node types
static const string test_trie("oix*R*OIL*AD");

// Sync result for one byte using the test trie
static bool test_sync[256] = 
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

BOOST_AUTO_TEST_CASE(is_sync)
{
  SyncTrie t(test_trie);
  for (int i = 0; i < array_size(test_sync); i++)
  {
    uint8_t sync[1];
    sync[0] = (uint8_t) i;
    if (t.is_sync(sync) != test_sync[i])
      BOOST_FAIL("value: " << i << " result must be: " << !test_sync[i]);
  }
}

BOOST_AUTO_TEST_CASE(invert)
{
  SyncTrie t(test_trie);
  t.invert();
  for (int i = 0; i < array_size(test_sync); i++)
  {
    uint8_t sync[1];
    sync[0] = (uint8_t) i;
    if (t.is_sync(sync) == test_sync[i])
      BOOST_FAIL("value: " << i << " result must be: " << !test_sync[i]);
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
    { "O", "*II","RI", 2, 2 },
    { "I", "I",  "I",  1, 1 },
    { "I", "A",  "A",  1, 1 },
    { "I", "D",  "I",  1, 1 },
    { "I", "oI", "LI", 2, 2 },
    { "I", "iI", "I",  1, 1 },
    { "I", "xI", "LI", 2, 2 },
    { "I", "RI", "A",  1, 1 },
    { "I", "LI", "LI", 2, 2 },
    { "I", "*II","LI", 2, 2 },
    { "A", "A",  "A",  1, 1 },
    { "A", "D",  "A",  1, 1 },
    { "A", "oI", "A",  1, 1 },
    { "A", "iI", "A",  1, 1 },
    { "A", "xI", "A",  1, 1 },
    { "A", "RI", "A",  1, 1 },
    { "A", "LI", "A",  1, 1 },
    { "A", "*II","A",  1, 1 },
    { "D", "D",  "D",  1, 1 },
    { "D", "oI", "oI", 2, 2 },
    { "D", "iI", "iI", 2, 2 },
    { "D", "xI", "xI", 2, 2 },
    { "D", "RI", "RI", 2, 2 },
    { "D", "LI", "LI", 2, 2 },
    { "D", "*II","*II",3, 2 },
    { "oI", "oI", "oI", 2, 2 },
    { "oI", "iI", "*II",3, 2 },
    { "oI", "xI", "*II",3, 2 },
    { "oI", "RI", "RI", 2, 2 },
    { "oI", "LI", "LI", 2, 2 },
    { "oI", "*II","*II",3, 2 },
    { "iI", "iI", "iI", 2, 2 },
    { "iI", "xI", "*II",3, 2 },
    { "iI", "RI", "RI", 2, 2 },
    { "iI", "LI", "LI", 2, 2 },
    { "iI", "*II","*II",3, 2 },
    { "xI", "xI", "xI", 2, 2 },
    { "xI", "RI", "RI", 2, 2 },
    { "xI", "LI", "LI", 2, 2 },
    { "xI", "*II","*II",3, 2 },
    { "RI", "RI", "RI", 2, 2 },
    { "RI", "LI", "A",  1, 1 },
    { "RI", "*II","RI", 2, 2 },
    { "LI", "LI", "LI", 2, 2 },
    { "LI", "*II","LI", 2, 2 },
    { "*II","*II","*II",3, 2 },
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

  // Typical usage test
  uint8_t sync[1];
  t = SyncTrie(0x55, 8);
  t.merge(SyncTrie(0xaa, 8));

  sync[0] = 0;
  BOOST_CHECK(!t.is_sync(sync));
  sync[0] = 0x55;
  BOOST_CHECK(t.is_sync(sync));
  sync[0] = 0xaa;
  BOOST_CHECK(t.is_sync(sync));

  // Run test cases
  for (size_t i = 0; i < array_size(tests); i++)
  {
    BOOST_MESSAGE(tests[i].left_trie << " or " << tests[i].right_trie << " = " << tests[i].result_trie);

    t = SyncTrie(tests[i].left_trie);
    t.merge(SyncTrie(tests[i].right_trie));
    BOOST_CHECK_EQUAL(t.serialize(), tests[i].result_trie);
    BOOST_CHECK_EQUAL(t.get_depth(), tests[i].result_depth);
    BOOST_CHECK_EQUAL(t.get_size(), tests[i].result_size);

    BOOST_MESSAGE(tests[i].right_trie << " or " << tests[i].left_trie << " = " << tests[i].result_trie);

    t = SyncTrie(tests[i].right_trie);
    t.merge(SyncTrie(tests[i].left_trie));
    BOOST_CHECK_EQUAL(t.serialize(), tests[i].result_trie);
    BOOST_CHECK_EQUAL(t.get_depth(), tests[i].result_depth);
    BOOST_CHECK_EQUAL(t.get_size(), tests[i].result_size);
  }
}

BOOST_AUTO_TEST_CASE(append)
{
  SyncTrie t;

  // Append an empty trie to another empty trie
  t.append(SyncTrie());
  BOOST_CHECK(t.serialize() == string());
  BOOST_CHECK(t.is_empty());
  BOOST_CHECK(t.sync_size() == 0);
  BOOST_CHECK(t.get_depth() == 0);
  BOOST_CHECK(t.get_size() == 0);

  // Append non-empty trie to an empty one
  t.append(SyncTrie("I"));
  BOOST_CHECK_EQUAL(t.serialize(), string("I"));
  BOOST_CHECK(!t.is_empty());
  BOOST_CHECK(t.sync_size() == 1);
  BOOST_CHECK(t.get_depth() == 1);
  BOOST_CHECK(t.get_size() == 1);

  // Append an empty trie to non-empty one
  t.append(SyncTrie());
  BOOST_CHECK_EQUAL(t.serialize(), string("I"));
  BOOST_CHECK(!t.is_empty());
  BOOST_CHECK(t.sync_size() == 1);
  BOOST_CHECK(t.get_depth() == 1);
  BOOST_CHECK(t.get_size() == 1);

  // Append an non-empty trie to non-empty one
  t.append(SyncTrie("I"));
  BOOST_CHECK_EQUAL(t.serialize(), string("iI"));
  BOOST_CHECK(!t.is_empty());
  BOOST_CHECK(t.sync_size() == 1);
  BOOST_CHECK(t.get_depth() == 2);
  BOOST_CHECK(t.get_size() == 2);

  // Append to different types of terminal nodes.
  static const string trie("oix*R*OIL*AD");
  static const string append("I");
  static const string result("oix**I*oIiI**xIDI");

  t = SyncTrie(trie);
  t.append(SyncTrie(append));
  BOOST_CHECK_EQUAL(t.serialize(), result);

  // Typical usage test
  uint8_t sync[1];
  t = SyncTrie(0x5, 4);
  t.append(SyncTrie(0x5, 4));

  sync[0] = 0x55;
  BOOST_CHECK(t.is_sync(sync));

  sync[0] = 0x5;
  BOOST_CHECK(!t.is_sync(sync));
  sync[0] = 0x5a;
  BOOST_CHECK(!t.is_sync(sync));
}

BOOST_AUTO_TEST_CASE(optimize)
{
  static struct {
    const char *trie;
    const char *result_trie;
    size_t result_size;
    size_t result_depth;
  } tests[] =
  {
    // Depth 1
    { "O",   "O", 1, 1 },
    { "I",   "I", 1, 1 },
    { "A",   "A", 1, 1 },
    { "D",   "",  0, 0 },
    // Depth 2
    { "oO",  "oO", 2, 2},
    { "oI",  "oI", 2, 2},
    { "oA",  "O",  1, 1},
    { "oD",  "",   0, 0},
    { "iO",  "iO", 2, 2},
    { "iI",  "iI", 2, 2},
    { "iA",  "I",  1, 1},
    { "iD",  "",   0, 0},
    { "xO",  "xO", 2, 2},
    { "xI",  "xI", 2, 2},
    { "xA",  "A",  1, 1},
    { "xD",  "",   0, 0},
    { "RO",  "RO", 2, 2},
    { "RI",  "RI", 2, 2},
    { "RA",  "A",  1, 1},
    { "RD",  "O",  1, 1},
    { "LO",  "LO", 2, 2},
    { "LI",  "LI", 2, 2},
    { "LA",  "A",  1, 1},
    { "LD",  "I",  1, 1},
    { "*OO", "*OO", 3, 2},
    { "*OI", "*OI", 3, 2},
    { "*OA", "LO",  2, 2},
    { "*OD", "oO",  2, 2},
    { "*IO", "*IO", 3, 2},
    { "*II", "*II", 3, 2},
    { "*IA", "LI",  2, 2},
    { "*ID", "oI",  2, 2},
    { "*AO", "RO",  2, 2},
    { "*AI", "RI",  2, 2},
    { "*AA", "A",   1, 1},
    { "*AD", "O",   1, 1},
    { "*DO", "iO",  2, 2},
    { "*DI", "iI",  2, 2},
    { "*DA", "I",   1, 1},
    { "*DD", "",    0, 0},
  };

  // Optimize an empty trie
  SyncTrie t;
  t.optimize();
  BOOST_CHECK(t.serialize() == string());
  BOOST_CHECK(t.is_empty());
  BOOST_CHECK(t.sync_size() == 0);
  BOOST_CHECK(t.get_depth() == 0);
  BOOST_CHECK(t.get_size() == 0);

  // Run tests
  for (int i = 0; i < array_size(tests); i++)
  {
    BOOST_MESSAGE("Optimize " << tests[i].trie << " -> " << tests[i].result_trie);

    t = SyncTrie(tests[i].trie);
    t.optimize();
    BOOST_CHECK_EQUAL(t.serialize(), tests[i].result_trie);
    BOOST_CHECK_EQUAL(t.get_depth(), tests[i].result_depth);
    BOOST_CHECK_EQUAL(t.get_size(), tests[i].result_size);
  }
}

BOOST_AUTO_TEST_CASE(deserialize_bad)
{
  SyncTrie t;

  // Unexpected end of data
  BOOST_CHECK(!t.deserialize("o"));
  BOOST_CHECK(!t.deserialize("i"));
  BOOST_CHECK(!t.deserialize("x"));
  BOOST_CHECK(!t.deserialize("L"));
  BOOST_CHECK(!t.deserialize("R"));
  BOOST_CHECK(!t.deserialize("*"));
  BOOST_CHECK(!t.deserialize("*I"));

  // Bad symbol
  BOOST_CHECK(!t.deserialize("w"));
}

BOOST_AUTO_TEST_SUITE_END()

///////////////////////////////////////////////////////////////////////////////
// SyncScan test
///////////////////////////////////////////////////////////////////////////////

BOOST_AUTO_TEST_SUITE(sync_scan)

BOOST_AUTO_TEST_CASE(constructor)
{
  SyncScan s;
  BOOST_CHECK(s.get_trie().is_empty());
}

BOOST_AUTO_TEST_CASE(init_constructor)
{
  SyncTrie t(test_trie);
  SyncScan s(t);

  t.optimize();
  BOOST_CHECK_EQUAL(s.get_trie().serialize(), t.serialize());
}

BOOST_AUTO_TEST_CASE(set_trie)
{
  SyncTrie t(test_trie);
  SyncScan s;
  s.set_trie(t);

  t.optimize();
  BOOST_CHECK_EQUAL(s.get_trie().serialize(), t.serialize());
}

BOOST_AUTO_TEST_CASE(scan_pos)
{
  RawNoise buf(noise_size, seed);
  SyncTrie t(test_trie);
  SyncScan s;
  size_t pos;

  // Scan using an empty trie
  pos = 0;
  bool result = s.scan_pos(buf.begin(), buf.size(), pos);

  BOOST_CHECK(!result);
  BOOST_CHECK_EQUAL(pos, noise_size);

  // Empty trie and wrong pos
  pos = buf.size() + 1;
  s.scan_pos(buf.begin(), buf.size(), pos);
  BOOST_CHECK_EQUAL(pos, buf.size());

  // Good trie and wrong pos
  s.set_trie(t);
  pos = buf.size() + 1;
  s.scan_pos(buf.begin(), buf.size(), pos);
  BOOST_CHECK_EQUAL(pos, buf.size());

  // Find a syncpoint
  for (size_t buf_size = 1; buf_size < 16; buf_size++)
    for (size_t sync_len = 1; sync_len <= buf_size; sync_len++)
    {
      SyncTrie trie(0xff, 8);
      for (size_t i = 1; i < sync_len; i++)
        trie += SyncTrie(0, 8);
      s.set_trie(trie);

      buf.allocate(buf_size);
      buf.zero();
      for (size_t sync_pos = 0; sync_pos <= buf_size - sync_len; sync_pos++)
      {
        buf[sync_pos] = 0xff;

        pos = 0;
        bool result = s.scan_pos(buf, buf_size, pos);
        BOOST_CHECK_EQUAL(result, true);
        BOOST_CHECK_EQUAL(pos, sync_pos);

        pos++;
        result = s.scan_pos(buf, buf_size, pos);
        BOOST_CHECK_EQUAL(result, false);
        BOOST_CHECK_EQUAL(pos, buf_size - sync_len + 1);

        buf[sync_pos] = 0;
      }
    }
}

BOOST_AUTO_TEST_SUITE_END()
