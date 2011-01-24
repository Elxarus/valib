/*
  MultiFIR test
*/

#include "fir/echo_fir.h"
#include "fir/delay_fir.h"
#include "fir/multi_fir.h"
#include "fir/param_fir.h"
#include "bad_fir.h"
#include <boost/scoped_ptr.hpp>
#include <boost/test/unit_test.hpp>

static const int sample_rate = 48000;
static const double gain = 0.5;

static const FIRGen *zero_list[] = { &fir_zero };
static const FIRGen *identity_list[] = { &fir_identity };

bool compare_firs(const FIRInstance *fir1, const FIRInstance *fir2)
{
  if (fir1->length != fir2->length)
    return false;

  for (int i = 0; i < fir1->length; i++)
    if (fir1->data[i] != fir2->data[i])
      return false;

  return true;
}

BOOST_AUTO_TEST_SUITE(multi_fir)

BOOST_AUTO_TEST_CASE(constructor)
{
  boost::scoped_ptr<const FIRInstance> fir;
  MultiFIR gen;

  // Empty generator produces null instance
  fir.reset(gen.make(sample_rate));
  BOOST_CHECK(fir.get() == 0);
}

BOOST_AUTO_TEST_CASE(init_constructor)
{
  boost::scoped_ptr<const FIRInstance> fir;
  MultiFIR gen(zero_list, array_size(zero_list));

  fir.reset(gen.make(sample_rate));
  BOOST_REQUIRE(fir);
  BOOST_CHECK_EQUAL(fir->type(), firt_zero);
}

BOOST_AUTO_TEST_CASE(set)
{
  int ver;
  boost::scoped_ptr<const FIRInstance> fir;
  MultiFIR gen;

  // Set zero list
  ver = gen.version();
  gen.set(zero_list, array_size(zero_list));
  BOOST_CHECK_NE(gen.version(), ver);

  fir.reset(gen.make(sample_rate));
  BOOST_REQUIRE(fir);
  BOOST_CHECK_EQUAL(fir->type(), firt_zero);

  // Set identity list
  ver = gen.version();
  gen.set(identity_list, array_size(identity_list));
  BOOST_CHECK_NE(gen.version(), ver);
  
  fir.reset(gen.make(sample_rate));
  BOOST_REQUIRE(fir);
  BOOST_CHECK_EQUAL(fir->type(), firt_identity);

  // Release
  ver = gen.version();
  gen.release();
  BOOST_CHECK_NE(gen.version(), ver);

  fir.reset(gen.make(sample_rate));
  BOOST_CHECK(fir.get() == 0);
}

BOOST_AUTO_TEST_CASE(version)
{
  // Version must change with change of originating generators
  int ver;
  FIRGain gain1, gain2;
  FIRGen *gain_list[] = { &gain1, &gain2 };
  MultiFIR gen(gain_list, array_size(gain_list));

  ver = gen.version();
  gain1.set_gain(gain);
  BOOST_CHECK_NE(gen.version(), ver);

  ver = gen.version();
  gain2.set_gain(gain);
  BOOST_CHECK_NE(gen.version(), ver);
}

BOOST_AUTO_TEST_CASE(make)
{
  boost::scoped_ptr<const FIRInstance> fir;
  FIRGain gain1(gain), gain2(gain);
  BadFIR bad_gen;
  MultiFIR gen;

  // Null pointers in the list must be ignored
  FIRGen *list_with_null[] = { &gain1, 0, &gain2 };
  gen.set(list_with_null, array_size(list_with_null));

  fir.reset(gen.make(sample_rate));
  BOOST_REQUIRE(fir);
  BOOST_CHECK_EQUAL(fir->type(), firt_gain);
  BOOST_CHECK_EQUAL(fir->data[0], gain1.get_gain() * gain2.get_gain());

  // List of null pointers is equivalent to empty list
  FIRGen *list_of_nulls[] = { 0, 0, 0 };
  gen.set(list_of_nulls, array_size(list_of_nulls));

  fir.reset(gen.make(sample_rate));
  BOOST_CHECK(fir.get() == 0);

  // Incorrect firs in the sequence (when generator returns null)
  // must be ignored.
  FIRGen *list_with_bad[] = { &gain1, &bad_gen, &gain2 };
  gen.set(list_with_bad, array_size(list_with_bad));

  fir.reset(gen.make(sample_rate));
  BOOST_REQUIRE(fir);
  BOOST_CHECK_EQUAL(fir->type(), firt_gain);
  BOOST_CHECK_EQUAL(fir->data[0], gain1.get_gain() * gain2.get_gain());

  // List of bad generators is equivalent to enmpty list
  FIRGen *list_of_bad[] = { &bad_gen, &bad_gen, &bad_gen };
  gen.set(list_of_bad, array_size(list_of_bad));

  fir.reset(gen.make(sample_rate));
  BOOST_CHECK(fir.get() == 0);

  // Zero generator in the list produces zero filter
  FIRGen *list_with_zero[] = { &gain1, &fir_zero, &gain2 };
  gen.set(list_with_zero, array_size(list_with_zero));

  fir.reset(gen.make(sample_rate));
  BOOST_REQUIRE(fir);
  BOOST_CHECK_EQUAL(fir->type(), firt_zero);
}

BOOST_AUTO_TEST_CASE(make_convolution)
{
  // Simple convolution test.
  // Sequence of parametric, gain and delay filters produces the same
  // parametric filter gained and shifted.

  int i;
  const int freq = 8000;   // 8kHz frequency for low-pass filter
  const double att = 50;   // 50dB atteniutaion
  const int trans = 500;   // 500Hz transition bandwidth
  const int delay = 10;    // Delay in samples
  boost::scoped_ptr<const FIRInstance> low_pass_fir;
  boost::scoped_ptr<const FIRInstance> fir;

  ParamFIR low_pass(FIR_LOW_PASS, freq, 0, trans, att);
  DelayFIR delay_gen(vtime_t(delay) / sample_rate);
  FIRGain gain_gen(gain);
  BadFIR bad_gen;

  FIRGen *convolution_list[] = { &low_pass, 0, &gain_gen, &bad_gen, &delay_gen };
  MultiFIR gen(convolution_list, array_size(convolution_list));

  low_pass_fir.reset(low_pass.make(sample_rate));
  BOOST_REQUIRE(low_pass_fir);

  fir.reset(gen.make(sample_rate));
  BOOST_REQUIRE(fir);
  BOOST_CHECK_EQUAL(fir->type(), firt_custom);
  BOOST_CHECK_EQUAL(fir->length, low_pass_fir->length + delay);

  for (i = 0; i < delay; i++)
    if (fir->data[i] != 0.0)
      BOOST_FAIL("Convolution error");

  for (i = 0; i < low_pass_fir->length; i++)
    if (!EQUAL_SAMPLES(low_pass_fir->data[i] * gain, fir->data[i+delay]))
      BOOST_FAIL("Convolution error");
}

BOOST_AUTO_TEST_SUITE_END()
