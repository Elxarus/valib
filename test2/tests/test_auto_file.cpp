/*
  AutoFile class test
*/

#include "auto_file.h"
#include <boost/test/unit_test.hpp>
#include "auto_file.h"

static const char *good_file = "test.vob";
static const char *bad_file = "it-is-no-such-file";

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

BOOST_AUTO_TEST_SUITE_END()
