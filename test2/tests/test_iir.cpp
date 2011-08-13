/*
  Base IIR classes test
*/

#include <boost/test/unit_test.hpp>
#include "iir.h"
#include "../noise_buf.h"

static const int sample_rate = 48000;
static const int seed = 3498756;
static const size_t noise_size = 1024;

BOOST_AUTO_TEST_SUITE(iir)

///////////////////////////////////////////////////////////////////////////////
// Biquad
///////////////////////////////////////////////////////////////////////////////

BOOST_AUTO_TEST_SUITE(biquad)

BOOST_AUTO_TEST_CASE(default_constructor)
{
  Biquad bi;

  BOOST_CHECK_EQUAL(bi.a[0], 1.0);
  BOOST_CHECK_EQUAL(bi.a[1], 0.0);
  BOOST_CHECK_EQUAL(bi.a[2], 0.0);
  BOOST_CHECK_EQUAL(bi.b[0], 1.0);
  BOOST_CHECK_EQUAL(bi.b[1], 0.0);
  BOOST_CHECK_EQUAL(bi.b[2], 0.0);
}

BOOST_AUTO_TEST_CASE(gain_constructor)
{
  double gain = 2.0;
  Biquad bi(gain);

  BOOST_CHECK_EQUAL(bi.a[0], 1.0);
  BOOST_CHECK_EQUAL(bi.a[1], 0.0);
  BOOST_CHECK_EQUAL(bi.a[2], 0.0);
  BOOST_CHECK_EQUAL(bi.b[0], gain);
  BOOST_CHECK_EQUAL(bi.b[1], 0.0);
  BOOST_CHECK_EQUAL(bi.b[2], 0.0);
}

BOOST_AUTO_TEST_CASE(init_constructor)
{
  const double a[3] = { 1.0, 2.0, 3.0 };
  const double b[3] = { 1.5, 2.5, 3.5 };
  Biquad bi(a[0], a[1], a[2], b[0], b[1], b[2]);

  BOOST_CHECK_EQUAL(bi.a[0], a[0]);
  BOOST_CHECK_EQUAL(bi.a[1], a[1]);
  BOOST_CHECK_EQUAL(bi.a[2], a[2]);
  BOOST_CHECK_EQUAL(bi.b[0], b[0]);
  BOOST_CHECK_EQUAL(bi.b[1], b[1]);
  BOOST_CHECK_EQUAL(bi.b[2], b[2]);
}

BOOST_AUTO_TEST_CASE(set)
{
  const double a[3] = { 1.0, 2.0, 3.0 };
  const double b[3] = { 1.5, 2.5, 3.5 };
  Biquad bi;
  bi.set(a[0], a[1], a[2], b[0], b[1], b[2]);

  BOOST_CHECK_EQUAL(bi.a[0], a[0]);
  BOOST_CHECK_EQUAL(bi.a[1], a[1]);
  BOOST_CHECK_EQUAL(bi.a[2], a[2]);
  BOOST_CHECK_EQUAL(bi.b[0], b[0]);
  BOOST_CHECK_EQUAL(bi.b[1], b[1]);
  BOOST_CHECK_EQUAL(bi.b[2], b[2]);
}

BOOST_AUTO_TEST_CASE(normalize)
{
  Biquad bi(2.0, 4.0, 6.0, 1.0, 3.0, 5.0);
  bi.normalize();

  BOOST_CHECK_EQUAL(bi.a[0], 1.0);
  BOOST_CHECK_EQUAL(bi.a[1], 2.0);
  BOOST_CHECK_EQUAL(bi.a[2], 3.0);
  BOOST_CHECK_EQUAL(bi.b[0], 0.5);
  BOOST_CHECK_EQUAL(bi.b[1], 1.5);
  BOOST_CHECK_EQUAL(bi.b[2], 2.5);
}

BOOST_AUTO_TEST_CASE(get_gain)
{
  Biquad bi;
  BOOST_CHECK_EQUAL(bi.get_gain(), 1.0);

  bi.set(1.0, 0.0, 0.0, 2.0, 0.0, 0.0);
  BOOST_CHECK_EQUAL(bi.get_gain(), 2.0);

  bi.set(2.0, 0.0, 0.0, 2.0, 0.0, 0.0);
  BOOST_CHECK_EQUAL(bi.get_gain(), 1.0);

  bi.set(1.0, 2.0, 2.0, 1.0, 2.0, 2.0);
  BOOST_CHECK_EQUAL(bi.get_gain(), 1.0);

  // infinity biquad: do not fail and return zero
  bi.set(0.0, 1.0, 1.0, 1.0, 1.0, 1.0);
  BOOST_CHECK_EQUAL(bi.get_gain(), 0.0);
}

BOOST_AUTO_TEST_CASE(apply_gain)
{
  Biquad bi(2.0);

  BOOST_CHECK_EQUAL(bi.get_gain(), 2.0);

  bi.apply_gain(0.5);
  BOOST_CHECK_EQUAL(bi.get_gain(), 1.0);
}

BOOST_AUTO_TEST_CASE(is_null)
{
  Biquad bi;
  BOOST_CHECK(!bi.is_null());

  bi.set(0.0, 0.0, 0.0, 0.0, 0.0, 0.0);
  BOOST_CHECK(bi.is_null());

  bi.set(1.0, 1.0, 1.0, 0.0, 1.0, 1.0);
  BOOST_CHECK(bi.is_null());
}

BOOST_AUTO_TEST_CASE(is_gain)
{
  Biquad bi;
  BOOST_CHECK(bi.is_gain());

  bi.set(2.0, 0.0, 0.0, 4.0, 0.0, 0.0);
  BOOST_CHECK(bi.is_gain());

  bi.set(1.0, 1.0, 0.0, 1.0, 0.0, 0.0);
  BOOST_CHECK(!bi.is_gain());

  bi.set(1.0, 0.0, 0.0, 1.0, 1.0, 0.0);
  BOOST_CHECK(!bi.is_gain());
}

BOOST_AUTO_TEST_CASE(is_identity)
{
  Biquad bi;
  BOOST_CHECK(bi.is_identity());

  bi.set(2.0, 0.0, 0.0, 2.0, 0.0, 0.0);
  BOOST_CHECK(bi.is_identity());

  bi.set(1.0, 0.0, 0.0, 2.0, 0.0, 0.0);
  BOOST_CHECK(!bi.is_identity());

  bi.set(1.0, 0.0, 0.0, 1.0, 1.0, 0.0);
  BOOST_CHECK(!bi.is_identity());
}

BOOST_AUTO_TEST_CASE(is_infinity)
{
  Biquad bi;
  BOOST_CHECK(!bi.is_infinity());

  bi.set(0.0, 1.0, 1.0, 1.0, 1.0, 1.0);
  BOOST_CHECK(bi.is_infinity());
}

BOOST_AUTO_TEST_SUITE_END()

///////////////////////////////////////////////////////////////////////////////
// IIRInstance
///////////////////////////////////////////////////////////////////////////////

BOOST_AUTO_TEST_SUITE(iir_instance)

BOOST_AUTO_TEST_CASE(constructor)
{
  const double gain = 2.0;
  IIRInstance iir(sample_rate, gain);
  BOOST_CHECK_EQUAL(iir.sample_rate, sample_rate);
  BOOST_CHECK_EQUAL(iir.gain, gain);
  BOOST_CHECK(iir.sections.empty());
}

BOOST_AUTO_TEST_CASE(normalize)
{
  IIRInstance iir(sample_rate);
  iir.sections.push_back(Biquad(2.0, 0.0, 0.0, 2.0, 0.0, 0.0));
  iir.sections.push_back(Biquad(4.0, 0.0, 0.0, 4.0, 0.0, 0.0));

  iir.normalize();
  BOOST_CHECK_EQUAL(iir.sections[0].a[0], 1.0);
  BOOST_CHECK_EQUAL(iir.sections[1].a[0], 1.0);
}

BOOST_AUTO_TEST_CASE(apply_gain)
{
  const double gain = 2.0;
  IIRInstance iir(sample_rate);
  iir.apply_gain(gain);
  BOOST_CHECK_EQUAL(iir.gain, gain);
}

BOOST_AUTO_TEST_CASE(get_gain)
{
  const double gain = 2.0;
  IIRInstance iir(sample_rate);
  BOOST_CHECK_EQUAL(iir.get_gain(), 1.0);

  iir.apply_gain(gain);
  BOOST_CHECK_EQUAL(iir.get_gain(), gain);

  iir.sections.push_back(Biquad(gain));
  BOOST_CHECK_EQUAL(iir.get_gain(), gain * gain);

  iir.sections.push_back(Biquad(gain));
  BOOST_CHECK_EQUAL(iir.get_gain(), gain * gain * gain);
}

BOOST_AUTO_TEST_CASE(is_null)
{
  IIRInstance null_iir(sample_rate, 0.0);
  BOOST_CHECK(null_iir.is_null());

  null_iir.sections.push_back(Biquad());
  BOOST_CHECK(null_iir.is_null());

  IIRInstance iir(sample_rate);
  BOOST_CHECK(!iir.is_null());

  iir.sections.push_back(Biquad());
  BOOST_CHECK(!iir.is_null());

  iir.sections.push_back(Biquad(0.0));
  BOOST_CHECK(iir.is_null());
}

BOOST_AUTO_TEST_CASE(is_gain)
{
  IIRInstance iir(sample_rate);
  BOOST_CHECK(iir.is_gain());

  iir.sections.push_back(Biquad());
  BOOST_CHECK(iir.is_gain());

  iir.sections.push_back(Biquad(1.0, 0.0, 0.0, 1.0, 1.0, 1.0));
  BOOST_CHECK(!iir.is_gain());
}

BOOST_AUTO_TEST_CASE(is_identity)
{
  IIRInstance iir(sample_rate);
  BOOST_CHECK(iir.is_identity());

  iir.sections.push_back(Biquad());
  BOOST_CHECK(iir.is_identity());

  iir.sections.push_back(Biquad(2.0));
  BOOST_CHECK(!iir.is_identity());
}

BOOST_AUTO_TEST_CASE(is_infinity)
{
  IIRInstance iir(sample_rate);
  BOOST_CHECK(!iir.is_infinity());

  iir.sections.push_back(Biquad());
  BOOST_CHECK(!iir.is_infinity());

  iir.sections.push_back(Biquad(0.0, 0.0, 0.0, 2.0, 0.0, 0.0));
  BOOST_CHECK(iir.is_infinity());
}

BOOST_AUTO_TEST_SUITE_END()

///////////////////////////////////////////////////////////////////////////////
// IIRGen classes
///////////////////////////////////////////////////////////////////////////////

BOOST_AUTO_TEST_SUITE(gen)

BOOST_AUTO_TEST_CASE(iir_zero)
{
  const IIRInstance *iir = IIRZero().make(sample_rate);
  BOOST_CHECK_EQUAL(iir->sample_rate, sample_rate);
  BOOST_CHECK_EQUAL(iir->gain, 0.0);
  BOOST_CHECK(iir->sections.empty());
  safe_delete(iir);
}

BOOST_AUTO_TEST_CASE(iir_identity)
{
  const IIRInstance *iir = IIRIdentity().make(sample_rate);
  BOOST_CHECK_EQUAL(iir->sample_rate, sample_rate);
  BOOST_CHECK_EQUAL(iir->gain, 1.0);
  BOOST_CHECK(iir->sections.empty());
  safe_delete(iir);
}

BOOST_AUTO_TEST_CASE(iir_gain)
{
  static const double gain1 = 0.5;
  static const double gain2 = 1.0 / 3.0;

  // Ensure that default constructor makes a generator with no gain (1.0)
  IIRGain default_iir_gain;
  BOOST_CHECK_EQUAL(default_iir_gain.get_gain(), 1.0);

  // Ensure that init constructor makes a generator with the gain given
  IIRGain iir_gain(gain1);
  BOOST_CHECK_EQUAL(iir_gain.get_gain(), gain1);

  // set_gain() function must change version.
  int ver = iir_gain.version();
  iir_gain.set_gain(gain2);
  BOOST_CHECK_NE(ver, iir_gain.version());

  // Check iir building
  IIRInstance *iir = iir_gain.make(sample_rate);
  BOOST_CHECK_EQUAL(iir->sample_rate, sample_rate);
  BOOST_CHECK_EQUAL(iir->gain, gain2);
  BOOST_CHECK(iir->sections.empty());
  safe_delete(iir);
}

BOOST_AUTO_TEST_SUITE_END()

///////////////////////////////////////////////////////////////////////////////
// IIRFilter classes
///////////////////////////////////////////////////////////////////////////////

BOOST_AUTO_TEST_SUITE(iir_filter)

BOOST_AUTO_TEST_CASE(default_filter)
{
  // Default filter does not change the signal
  SamplesNoise signal(noise_size, seed);
  SamplesNoise reference(noise_size, seed);

  IIRFilter().process(signal, signal.size());
  sample_t diff = peak_diff(signal, reference, noise_size);
  BOOST_CHECK_EQUAL(diff, 0.0);
}

BOOST_AUTO_TEST_CASE(zero_filter)
{
  SamplesNoise signal(noise_size, seed);
  Samples reference(noise_size);
  reference.zero();

  IIRFilter(&IIRInstance(sample_rate, 0.0)).process(signal, signal.size());
  sample_t diff = peak_diff(signal, reference, noise_size);
  BOOST_CHECK_EQUAL(diff, 0.0);
}

BOOST_AUTO_TEST_CASE(identity_filter)
{
  SamplesNoise signal(noise_size, seed);
  SamplesNoise reference(noise_size, seed);

  IIRFilter(&IIRInstance(sample_rate)).process(signal, signal.size());
  sample_t diff = peak_diff(signal, reference, noise_size);
  BOOST_CHECK_EQUAL(diff, 0.0);
}

BOOST_AUTO_TEST_CASE(gain_filter)
{
  double gain = 2.0;
  SamplesNoise signal(noise_size, seed);
  SamplesNoise reference(noise_size, seed);
  gain_samples(gain, reference, reference.size());

  IIRFilter(&IIRInstance(sample_rate, gain)).process(signal, signal.size());
  sample_t diff = peak_diff(signal, reference, noise_size);
  BOOST_CHECK_LT(diff, SAMPLE_THRESHOLD);
}

BOOST_AUTO_TEST_CASE(running_average_filter)
{
  SamplesNoise signal(noise_size, seed);
  SamplesNoise reference(noise_size, seed);

  // IIRFilter processing
  IIRInstance iir(sample_rate);
  iir.sections.push_back(Biquad(1.0, -0.5, 0.0, 0.5, 0.0, 0.0));
  IIRFilter(&iir).process(signal, signal.size());

  // Direct running average implementation
  reference[0] = reference[0] / 2;
  for (size_t i = 1; i < reference.size(); i++)
    reference[i] = (reference[i-1] + reference[i]) / 2;

  sample_t diff = peak_diff(signal, reference, noise_size);
  BOOST_CHECK_LT(diff, SAMPLE_THRESHOLD);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
