/*
  AutoFile class test
*/

#include <limits>
#include "auto_file.h"
#include "../noise_buf.h"
#include <boost/test/unit_test.hpp>

static const int seed = 687474;

static const char *good_file = "test.vob";
static const char *bad_file = "it-is-no-such-file";
static const char *temp_file = "temp.tmp";
static const size_t temp_file_size = 1000;

static void test_open(AutoFile &f)
{
  BOOST_CHECK( f.is_open() );
  BOOST_CHECK( !f.eof() );
  BOOST_CHECK( f.size() > 0 );
  BOOST_CHECK( f.fh() != 0 );
}

static void test_closed(AutoFile &f)
{
  BOOST_CHECK( !f.is_open() );
  BOOST_CHECK( f.eof() );
  BOOST_CHECK( f.size() == 0 );
  BOOST_CHECK( f.fh() == 0 );
}

BOOST_AUTO_TEST_SUITE(auto_file)

BOOST_AUTO_TEST_CASE(default_constructor)
{
  AutoFile f;
  test_closed(f);

  // Destructor of a default constructed file
  // should not fail after leaving this block
}

BOOST_AUTO_TEST_CASE(init_constructor)
{
  AutoFile f(good_file);
  test_open(f);

  // Test automatic cast to FILE *
  FILE *fh = f;

  // Destructor of an init constructed file
  // should not fail after leaving this block
}

BOOST_AUTO_TEST_CASE(init_constructor_failure)
{
  AutoFile f(bad_file);
  test_closed(f);
}

BOOST_AUTO_TEST_CASE(open)
{
  AutoFile f;

  BOOST_CHECK( f.open(good_file) );
  test_open(f);

  f.close();
  test_closed(f);
}

BOOST_AUTO_TEST_CASE(open_fail)
{
  AutoFile f;

  BOOST_CHECK( !f.open(bad_file) );
  test_closed(f);
}

BOOST_AUTO_TEST_CASE(fsize_type)
{
  if (sizeof(AutoFile::fsize_t) > sizeof(size_t))
  {
    // 32bit
    AutoFile::fsize_t large_size = std::numeric_limits<AutoFile::fsize_t>::max() - 1;
    AutoFile::fsize_t good_size = std::numeric_limits<size_t>::max() - 1;

    BOOST_CHECK(AutoFile::is_large(large_size));
    BOOST_CHECK_EQUAL(good_size, AutoFile::size_cast(good_size));
  }
  else
  {
    // 64bit
    AutoFile::fsize_t large_size = std::numeric_limits<AutoFile::fsize_t>::max() - 1;
    BOOST_CHECK(!AutoFile::is_large(large_size));
    BOOST_CHECK_EQUAL(large_size, AutoFile::size_cast(large_size));
  }
}

BOOST_AUTO_TEST_CASE(read_write)
{
  AutoFile f(temp_file, "wb");
  BOOST_REQUIRE( f.is_open() );

  RawNoise write_data(temp_file_size, seed);
  
  size_t write_size = f.write(write_data, write_data.size());
  BOOST_CHECK_EQUAL(write_size, write_data.size());
  f.close();

  f.open(temp_file, "rb");
  BOOST_REQUIRE( f.is_open() );

  Rawdata read_data(temp_file_size);
  size_t read_size = f.read(read_data, temp_file_size);

  BOOST_REQUIRE_EQUAL(read_size, temp_file_size);
  BOOST_CHECK(memcmp(read_data, write_data, temp_file_size) == 0);
  f.close();

  MemFile memfile(temp_file);
  BOOST_REQUIRE_EQUAL(memfile.size(), temp_file_size);
  BOOST_CHECK(memcmp(memfile, write_data, temp_file_size) == 0);

  remove(temp_file);
}

BOOST_AUTO_TEST_SUITE_END()
