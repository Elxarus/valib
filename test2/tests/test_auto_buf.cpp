/*
  AutoBuf class test
*/


#include <limits>
#include "auto_buf.h"

// bad_size must be less than a half of address space
// otherwise VC++ throws "bad allocation size" exception
// instead of std::bad_alloc in debug builds
static const size_t bad_size = std::numeric_limits<size_t>::max() / 2 - 1;
static const size_t data_size  = 100;
static const size_t data_size2 = 200;

#define BOOST_TEST_MODULE auto_buf
#include <boost/test/included/unit_test.hpp>

template <class T>
static void test_deallocated(AutoBuf<T> &buf)
{
  BOOST_CHECK( !buf.is_allocated() );
  BOOST_CHECK( buf.size() == 0 );
  BOOST_CHECK( buf.allocated() == 0 );
  BOOST_CHECK( buf.begin() == 0 );
}

template <class T>
static void test_allocated(AutoBuf<T> &buf)
{
  BOOST_CHECK( buf.is_allocated() );
  BOOST_CHECK( buf.size() > 0 );
  BOOST_CHECK( buf.allocated() >= buf.size() );
  BOOST_CHECK( buf.begin() != 0 );
}

BOOST_AUTO_TEST_CASE(default_constructor)
{
  AutoBuf<uint8_t> buf;
  test_deallocated(buf);

  // Destructor of an default constructed buffer
  // should not fail after leaving of this block
}

BOOST_AUTO_TEST_CASE(init_constructor)
{
  AutoBuf<uint8_t> buf(data_size);
  test_allocated(buf);

  // Test automatic cast to T*
  uint8_t *ptr = buf;

  // Destructor of an init constructed buffer
  // should not fail after leaving of this block
}

BOOST_AUTO_TEST_CASE(init_constructor_failure)
{
  BOOST_CHECK_THROW( AutoBuf<uint8_t> buf(bad_size), std::bad_alloc);
}

BOOST_AUTO_TEST_CASE(allocate)
{
  AutoBuf<uint8_t> buf;

  // Just allocate a buffer
  buf.allocate(data_size);
  test_allocated(buf);

  // Allocate more data
  buf.allocate(data_size2);
  test_allocated(buf);

  // Allocate less data
  // (buffer should not be reallocated)
  uint8_t *old_data = buf;
  buf.allocate(data_size);
  test_allocated(buf);

  // Free buffer
  buf.free();
  test_deallocated(buf);
}

BOOST_AUTO_TEST_CASE(allocate_failure)
{
  AutoBuf<uint8_t> buf;

  // Allocate too much (fail allocation)
  // when buffer is not allocated
  BOOST_CHECK_THROW( buf.allocate(bad_size), std::bad_alloc);

  // Allocate too much (fail allocation)
  // when buffer is allocated
  buf.allocate(data_size);
  BOOST_CHECK_THROW( buf.allocate(bad_size), std::bad_alloc);
}

BOOST_AUTO_TEST_CASE(reallocate)
{
  AutoBuf<uint8_t> buf;

  // Allocate the buffer with reallocate()
  buf.reallocate(data_size);
  test_allocated(buf);

  // Fill some data
  buf.zero();
  buf[0] = 1;
  buf[data_size-1] = 2;

  // Reallocate
  // (Must keep the data at the buffer)

  buf.reallocate(data_size2);
  test_allocated(buf);

  BOOST_CHECK( buf[0] == 1 );
  BOOST_CHECK( buf[data_size-1] == 2 );
}

BOOST_AUTO_TEST_CASE(zero)
{
  AutoBuf<uint8_t> buf(data_size);
  test_allocated(buf);

  buf[0] = 1;
  buf[data_size-1] = 1;
  buf.zero();

  BOOST_CHECK( buf[0] == 0 );
  BOOST_CHECK( buf[data_size-1] == 0 );
}
