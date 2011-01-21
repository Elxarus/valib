/*
  DelayFIR test
*/

#include "fir/echo_fir.h"
#include <boost/scoped_ptr.hpp>
#include <boost/test/unit_test.hpp>

static const int sample_rate = 48000;
static const vtime_t delay = 0.001; // 1ms
static const double gain = 0.5;

BOOST_AUTO_TEST_SUITE(echo_fir)

BOOST_AUTO_TEST_CASE(constructor)
{
  EchoFIR fir;
  BOOST_CHECK_EQUAL(fir.get_delay(), 0);
  BOOST_CHECK_EQUAL(fir.get_gain(), 0);
}

BOOST_AUTO_TEST_CASE(init_constructor)
{
  EchoFIR fir(delay, gain);
  BOOST_CHECK_EQUAL(fir.get_delay(), delay);
  BOOST_CHECK_EQUAL(fir.get_gain(), gain);
}

BOOST_AUTO_TEST_CASE(set)
{
  static const vtime_t delay2 = delay * 2;
  static const vtime_t gain2 = gain * 2;

  int ver;
  EchoFIR fir;

  ver = fir.version();
  fir.set_delay(delay);
  BOOST_CHECK_EQUAL(fir.get_delay(), delay);
  BOOST_CHECK_NE(fir.version(), ver);

  ver = fir.version();
  fir.set_delay(delay2);
  BOOST_CHECK_EQUAL(fir.get_delay(), delay2);
  BOOST_CHECK_NE(fir.version(), ver);

  ver = fir.version();
  fir.set_gain(gain);
  BOOST_CHECK_EQUAL(fir.get_gain(), gain);
  BOOST_CHECK_NE(fir.version(), ver);

  ver = fir.version();
  fir.set_gain(gain2);
  BOOST_CHECK_EQUAL(fir.get_gain(), gain2);
  BOOST_CHECK_NE(fir.version(), ver);

  ver = fir.version();
  fir.set(delay, gain);
  BOOST_CHECK_EQUAL(fir.get_delay(), delay);
  BOOST_CHECK_EQUAL(fir.get_gain(), gain);
  BOOST_CHECK_NE(fir.version(), ver);

  ver = fir.version();
  fir.set(delay2, gain2);
  BOOST_CHECK_EQUAL(fir.get_delay(), delay2);
  BOOST_CHECK_EQUAL(fir.get_gain(), gain2);
  BOOST_CHECK_NE(fir.version(), ver);

  // Negative delays are prohibited
  fir.set_delay(-1);
  BOOST_CHECK_EQUAL(fir.get_delay(), 0);

  fir.set(-1, 0);
  BOOST_CHECK_EQUAL(fir.get_delay(), 0);
}

BOOST_AUTO_TEST_CASE(make)
{
  boost::scoped_ptr<const FIRInstance> inst;
  EchoFIR fir;

  // Make identity when uninitialized
  inst.reset(fir.make(sample_rate));
  BOOST_CHECK_EQUAL(inst->type(), firt_identity);

  // Make gain with delay = 0
  fir.set(0, gain);
  inst.reset(fir.make(sample_rate));
  BOOST_CHECK_EQUAL(inst->type(), firt_gain);
  BOOST_CHECK_EQUAL(inst->data[0], 1.0 + gain);

  // Make identity with gain = 0
  fir.set(delay, 0);
  inst.reset(fir.make(sample_rate));
  BOOST_CHECK_EQUAL(inst->type(), firt_identity);

  // Make echo filter
  fir.set(delay, gain);
  inst.reset(fir.make(sample_rate));
  BOOST_CHECK_EQUAL(inst->type(), firt_custom);
  BOOST_CHECK_EQUAL(inst->length, sample_rate * delay + 1);
  BOOST_CHECK_EQUAL(inst->center, 0);
  BOOST_CHECK_EQUAL(inst->data[0], 1.0);
  BOOST_CHECK_EQUAL(inst->data[inst->length-1], gain);

  // Gain filter when delay < 1/sample_rate
  fir.set(0.5/sample_rate, gain);
  inst.reset(fir.make(sample_rate));
  BOOST_CHECK_EQUAL(inst->type(), firt_gain);
  BOOST_CHECK_EQUAL(inst->data[0], 1.0 + gain);
}

BOOST_AUTO_TEST_SUITE_END()
