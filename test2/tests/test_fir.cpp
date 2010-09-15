/*
  Base FIR classes test
*/

#include <boost/test/unit_test.hpp>
#include "fir.h"

static const int sample_rate = 48000;

BOOST_AUTO_TEST_SUITE(fir)

///////////////////////////////////////////////////////////////////////////////
// FIRInstance classes
///////////////////////////////////////////////////////////////////////////////

BOOST_AUTO_TEST_SUITE(instance)

BOOST_AUTO_TEST_CASE(static_fir_instance)
{
  double static_fir[] = { 0, 0, 1.0 };

  /////////////////////////////////////////////////////////
  // Create and delete StaticFIRInstance.
  // So we can ensure that we don't delete the array along
  // with the instance.

  StaticFIRInstance *fir = 
    new StaticFIRInstance(sample_rate, array_size(static_fir), array_size(static_fir) / 2, static_fir);

  BOOST_CHECK_EQUAL(fir->sample_rate, sample_rate);
  BOOST_CHECK_EQUAL(fir->length, array_size(static_fir));
  BOOST_CHECK_EQUAL(fir->center, array_size(static_fir) / 2);
  BOOST_CHECK(memcmp(fir->data, static_fir, sizeof(static_fir)) == 0);

  safe_delete(fir);
}

BOOST_AUTO_TEST_CASE(dynamic_fir_instance)
{
  static const int length = 1000;
  static const int center = 100;

  DynamicFIRInstance *fir =
    new DynamicFIRInstance(sample_rate, length, center);

  BOOST_CHECK_EQUAL(fir->sample_rate, sample_rate);
  BOOST_CHECK_EQUAL(fir->length, length);
  BOOST_CHECK_EQUAL(fir->center, center);

  // Ensure that fir function is initialized with zeros
  for (int i = 0; i < length; i++)
    if (fir->buf[i] != 0)
      BOOST_FAIL("DynamicFIRInstance data must be initialized with zeros");

  safe_delete(fir);
}

BOOST_AUTO_TEST_CASE(zero_fir_instance)
{
  ZeroFIRInstance fir(sample_rate);
  BOOST_CHECK_EQUAL(fir.sample_rate, sample_rate);
  BOOST_CHECK_EQUAL(fir.length, 1);
  BOOST_CHECK_EQUAL(fir.center, 0);
  BOOST_CHECK_EQUAL(fir.data[0], 0.0);
  BOOST_CHECK_EQUAL(fir.type(), firt_zero);
}

BOOST_AUTO_TEST_CASE(identity_fir_instance)
{
  IdentityFIRInstance fir(sample_rate);
  BOOST_CHECK_EQUAL(fir.sample_rate, sample_rate);
  BOOST_CHECK_EQUAL(fir.length, 1);
  BOOST_CHECK_EQUAL(fir.center, 0);
  BOOST_CHECK_EQUAL(fir.data[0], 1.0);
  BOOST_CHECK_EQUAL(fir.type(), firt_identity);
}

BOOST_AUTO_TEST_CASE(gain_fir_instance)
{
  static const double gain = 1.0 / 3.0;

  GainFIRInstance fir(sample_rate, gain);
  BOOST_CHECK_EQUAL(fir.sample_rate, sample_rate);
  BOOST_CHECK_EQUAL(fir.length, 1);
  BOOST_CHECK_EQUAL(fir.center, 0);
  BOOST_CHECK_EQUAL(fir.data[0], gain);
  BOOST_CHECK_EQUAL(fir.type(), firt_gain);
}


BOOST_AUTO_TEST_SUITE_END()

///////////////////////////////////////////////////////////////////////////////
// FIRGen classes
///////////////////////////////////////////////////////////////////////////////

BOOST_AUTO_TEST_SUITE(gen)

BOOST_AUTO_TEST_CASE(fir_zero)
{
  const FIRInstance *fir = FIRZero().make(sample_rate);
  BOOST_CHECK_EQUAL(fir->sample_rate, sample_rate);
  BOOST_CHECK_EQUAL(fir->length, 1);
  BOOST_CHECK_EQUAL(fir->center, 0);
  BOOST_CHECK_EQUAL(fir->data[0], 0.0);
  BOOST_CHECK_EQUAL(fir->type(), firt_zero);
  safe_delete(fir);
}

BOOST_AUTO_TEST_CASE(fir_identity)
{
  const FIRInstance *fir = FIRIdentity().make(sample_rate);
  BOOST_CHECK_EQUAL(fir->sample_rate, sample_rate);
  BOOST_CHECK_EQUAL(fir->length, 1);
  BOOST_CHECK_EQUAL(fir->center, 0);
  BOOST_CHECK_EQUAL(fir->data[0], 1.0);
  BOOST_CHECK_EQUAL(fir->type(), firt_identity);
  safe_delete(fir);
}

BOOST_AUTO_TEST_CASE(fir_gain)
{
  static const double gain1 = 0.5;
  static const double gain2 = 1.0 / 3.0;

  // Ensure that default constructor makes a generator with no gain (1.0)
  FIRGain default_fir_gain;
  BOOST_CHECK_EQUAL(default_fir_gain.get_gain(), 1.0);

  // Ensure that init constructor makes a generator with the gain given
  FIRGain fir_gain(gain1);
  BOOST_CHECK_EQUAL(fir_gain.get_gain(), gain1);

  // set_gain() function must change version.
  int ver = fir_gain.version();
  fir_gain.set_gain(gain2);
  BOOST_CHECK_NE(ver, fir_gain.version());

  // Check fir building
  const FIRInstance *fir = fir_gain.make(sample_rate);
  BOOST_CHECK_EQUAL(fir->sample_rate, sample_rate);
  BOOST_CHECK_EQUAL(fir->length, 1);
  BOOST_CHECK_EQUAL(fir->center, 0);
  BOOST_CHECK_EQUAL(fir->data[0], gain2);
  BOOST_CHECK_EQUAL(fir->type(), firt_gain);
  safe_delete(fir);
}

BOOST_AUTO_TEST_SUITE_END()

///////////////////////////////////////////////////////////////////////////////
// FIRRef test
///////////////////////////////////////////////////////////////////////////////

BOOST_AUTO_TEST_SUITE(fir_ref)

BOOST_AUTO_TEST_CASE(default_constructor)
{
  // Default constructor test
  FIRRef ref;
  BOOST_CHECK(ref.get() == 0);
  BOOST_CHECK(ref.make(sample_rate) == 0);
}

BOOST_AUTO_TEST_CASE(init_constructor)
{
  // Init constructor test
  FIRRef ref(&fir_zero);
  BOOST_CHECK_EQUAL(ref.get(), &fir_zero);
}

BOOST_AUTO_TEST_CASE(copy_constructor)
{
  // Copy constructor test
  FIRRef ref1(&fir_zero);
  FIRRef ref2(ref1);
  BOOST_CHECK_EQUAL(ref2.get(), &fir_zero);
}

BOOST_AUTO_TEST_CASE(set)
{
  // Generator change test
  int ver;
  FIRRef ref;

  // void set(const FIRGen *new_fir)
  ver = ref.version();
  ref.set(&fir_zero);
  BOOST_CHECK_EQUAL(ref.get(), &fir_zero);
  BOOST_CHECK_NE(ref.version(), ver);

  // FIRRef &operator =(const FIRGen *new_fir) 
  ver = ref.version();
  ref = &fir_identity;
  BOOST_CHECK_EQUAL(ref.get(), &fir_identity);
  BOOST_CHECK_NE(ref.version(), ver);

  // FIRRef &operator =(const FIRRef &ref)
  ver = ref.version();
  ref = FIRRef(&fir_zero);
  BOOST_CHECK_EQUAL(ref.get(), &fir_zero);
  BOOST_CHECK_NE(ref.version(), ver);
}

BOOST_AUTO_TEST_CASE(release)
{
  // Release test
  FIRRef ref(&fir_zero);
  int ver = ref.version();

  ref.release();
  BOOST_CHECK(ref.get() == 0);
  BOOST_CHECK_NE(ref.version(), ver);
}

BOOST_AUTO_TEST_CASE(version)
{
  // Genreator version change test
  const double gain1 = 0.5;
  const double gain2 = 1.0;

  int ver;
  FIRGain gain(gain1);
  FIRRef ref(&gain);

  ver = ref.version();
  gain.set_gain(gain2);
  BOOST_CHECK_NE(ref.version(), ver);

  ver = ref.version();
  gain.set_gain(gain1);
  BOOST_CHECK_NE(ref.version(), ver);
}

BOOST_AUTO_TEST_CASE(make)
{
  // Generation test
  FIRRef ref(&fir_zero);

  const FIRInstance *fir = ref.make(sample_rate);
  BOOST_CHECK_EQUAL(fir->type(), firt_zero);
  safe_delete(fir);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
