/*
  DelayFIR test
*/

#include "fir/delay_fir.h"
#include <boost/scoped_ptr.hpp>
#include <boost/test/unit_test.hpp>

static const int sample_rate = 48000;
static const vtime_t delay = 0.001; // 1ms


BOOST_AUTO_TEST_SUITE(delay_fir)

BOOST_AUTO_TEST_CASE(constructor)
{
  DelayFIR fir;
  BOOST_CHECK_EQUAL(fir.get_delay(), 0);
}

BOOST_AUTO_TEST_CASE(init_constructor)
{
  DelayFIR fir(delay);
  BOOST_CHECK_EQUAL(fir.get_delay(), delay);
}

BOOST_AUTO_TEST_CASE(set_delay)
{
  static const vtime_t delay2 = delay * 2;

  int ver;
  DelayFIR fir;

  ver = fir.version();
  fir.set_delay(delay);
  BOOST_CHECK_EQUAL(fir.get_delay(), delay);
  BOOST_CHECK_NE(fir.version(), ver);

  ver = fir.version();
  fir.set_delay(delay2);
  BOOST_CHECK_EQUAL(fir.get_delay(), delay2);
  BOOST_CHECK_NE(fir.version(), ver);

  // Negative delays are prohibited
  fir.set_delay(-1);
  BOOST_CHECK_EQUAL(fir.get_delay(), 0);
}

BOOST_AUTO_TEST_CASE(make)
{
  boost::scoped_ptr<const FIRInstance> inst;
  DelayFIR fir;

  // Make identity when uninitialized
  inst.reset(fir.make(sample_rate));
  BOOST_CHECK_EQUAL(inst->type(), firt_identity);

  // Make delay filter
  fir.set_delay(delay);
  inst.reset(fir.make(sample_rate));
  BOOST_CHECK_EQUAL(inst->type(), firt_custom);
  BOOST_CHECK_EQUAL(inst->length, sample_rate * delay + 1);
  BOOST_CHECK_EQUAL(inst->center, 0);
  BOOST_CHECK_EQUAL(inst->data[0], 0.0);
  BOOST_CHECK_EQUAL(inst->data[inst->length-1], 1.0);
}

BOOST_AUTO_TEST_SUITE_END()
