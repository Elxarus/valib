/*
  ParamFIR test
*/

#include "../../suite.h"
#include "fir/param_fir.h"
#include "filters/convolver.h"
#include "filters/filter_graph.h"
#include "filters/slice.h"
#include "source/generator.h"
#include <boost/scoped_ptr.hpp>
#include <boost/test/unit_test.hpp>

static const int sample_rate = 48000;
static const int f1 = 4000;
static const int f2 = 8000;
static const double a = 100;
static const int df = 100;

BOOST_AUTO_TEST_SUITE(param_fir)

BOOST_AUTO_TEST_CASE(constructor)
{
  boost::scoped_ptr<const FIRInstance> fir;
  ParamFIR gen;

  fir.reset(gen.make(sample_rate));
  BOOST_CHECK(fir.get() == 0);
}

BOOST_AUTO_TEST_CASE(init_constructor)
{
  ParamFIR::filter_t result_type;
  double result_f1, result_f2, result_df, result_a;
  bool result_norm;

  ParamFIR gen(ParamFIR::low_pass, f1, 0, df, a, false);

  gen.get(&result_type, &result_f1, &result_f2, &result_df, &result_a, &result_norm);
  BOOST_CHECK_EQUAL(result_type, ParamFIR::low_pass);
  BOOST_CHECK_EQUAL(result_f1, f1);
  BOOST_CHECK_EQUAL(result_f2, 0);
  BOOST_CHECK_EQUAL(result_df, df);
  BOOST_CHECK_EQUAL(result_a, a);
  BOOST_CHECK_EQUAL(result_norm, false);
}

BOOST_AUTO_TEST_CASE(set)
{
  ParamFIR::filter_t result_type;
  double result_f1, result_f2, result_df, result_a;
  bool result_norm;
  int ver;

  ParamFIR gen;

  // Set 1
  ver = gen.version();
  gen.set(ParamFIR::low_pass, f1/sample_rate, 0, df/sample_rate, a, true);
  BOOST_CHECK_NE(gen.version(), ver);

  gen.get(&result_type, &result_f1, &result_f2, &result_df, &result_a, &result_norm);
  BOOST_CHECK_EQUAL(result_type, ParamFIR::low_pass);
  BOOST_CHECK_EQUAL(result_f1, f1/sample_rate);
  BOOST_CHECK_EQUAL(result_f2, 0);
  BOOST_CHECK_EQUAL(result_df, df/sample_rate);
  BOOST_CHECK_EQUAL(result_a, a);
  BOOST_CHECK_EQUAL(result_norm, true);

  // Set 2
  ver = gen.version();
  gen.set(ParamFIR::band_pass, f1, f2, df, a, false);
  BOOST_CHECK_NE(gen.version(), ver);

  gen.get(&result_type, &result_f1, &result_f2, &result_df, &result_a, &result_norm);
  BOOST_CHECK_EQUAL(result_type, ParamFIR::band_pass);
  BOOST_CHECK_EQUAL(result_f1, f1);
  BOOST_CHECK_EQUAL(result_f2, f2);
  BOOST_CHECK_EQUAL(result_df, df);
  BOOST_CHECK_EQUAL(result_a, a);
  BOOST_CHECK_EQUAL(result_norm, false);

  // Check that frequencies are swapped when f2 < f1
  ver = gen.version();
  gen.set(ParamFIR::band_pass, f2, f1, df, a, false);
  BOOST_CHECK_NE(gen.version(), ver);

  gen.get(&result_type, &result_f1, &result_f2, &result_df, &result_a, &result_norm);
  BOOST_CHECK_EQUAL(result_f1, f1);
  BOOST_CHECK_EQUAL(result_f2, f2);
}

BOOST_AUTO_TEST_CASE(make)
{
  const size_t block_size = 65536;
  const Speakers spk(FORMAT_LINEAR, MODE_STEREO, sample_rate);
  boost::scoped_ptr<const FIRInstance> fir;
  double diff, level;
  int len;

  ParamFIR gen;

  // Test source test_src (tone -> convolver -> slice)

  ToneGen tone;
  SliceFilter slice;
  Convolver conv;
  FilterChain test(&conv, &slice);
  conv.set_fir(&gen);

  // Reference source ref_src (tone -> slice)

  ToneGen ref_tone;
  SliceFilter ref_slice;

  /////////////////////////////////////////////////////////
  // Low pass
  /////////////////////////////////////////////////////////

  gen.set(ParamFIR::low_pass, f1, 0, df, a);
  fir.reset(gen.make(sample_rate));
  BOOST_REQUIRE(fir);
  len = fir->length;

  /////////////////////////////////////////////////////////
  // Tone in the pass band must remain unchanged

  tone.init(spk, f1-df, 0, block_size + 2 * len);
  slice.init(len, block_size + len);
  ref_tone.init(spk, f1-df, 0, block_size + 2 * len);
  ref_slice.init(len, len + block_size);
  test.reset();
  ref_slice.reset();

  diff = calc_diff(&tone, &test, &ref_tone, &ref_slice);
  BOOST_CHECK_GT(diff, 0);
  BOOST_CHECK_LT(value2db(diff), -a);

  /////////////////////////////////////////////////////////
  // Tone in the stop band must be filtered out

  tone.init(spk, f1+df, 0, block_size + 2 * len);
  slice.init(len, block_size + len);
  test.reset();

  level = calc_peak(&tone, &test);
  BOOST_CHECK_GT(level, 0);
  BOOST_CHECK_LT(value2db(level), -a);

  /////////////////////////////////////////////////////////
  // High pass
  /////////////////////////////////////////////////////////

  gen.set(ParamFIR::high_pass, f1, 0, df, a);
  fir.reset(gen.make(spk.sample_rate));
  BOOST_REQUIRE(fir);
  len = fir->length;

  /////////////////////////////////////////////////////////
  // Tone in the pass band must remain unchanged

  tone.init(spk, f1+df, 0, block_size + 2 * len);
  slice.init(len, block_size + len);
  ref_tone.init(spk, f1+df, 0, block_size + 2 * len);
  ref_slice.init(len, len + block_size);
  test.reset();
  ref_slice.reset();

  diff = calc_diff(&tone, &test, &ref_tone, &ref_slice);
  BOOST_CHECK_GT(diff, 0);
  BOOST_CHECK_LT(value2db(diff), -a);

  /////////////////////////////////////////////////////////
  // Tone in the stop band must be filtered out

  tone.init(spk, f1-df, 0, block_size + 2 * len);
  slice.init(len, block_size + len);
  test.reset();

  level = calc_peak(&tone, &test);
  BOOST_CHECK_GT(level, 0);
  BOOST_CHECK_LT(value2db(level), -a);

  /////////////////////////////////////////////////////////
  // BandPass
  /////////////////////////////////////////////////////////

  gen.set(ParamFIR::band_pass, f1, f2, df, a);
  fir.reset(gen.make(spk.sample_rate));
  BOOST_REQUIRE(fir);
  len = fir->length;

  /////////////////////////////////////////////////////////
  // Tone in the pass band must remain unchanged

  tone.init(spk, (f1+f2)/2, 0, block_size + 2 * len);
  slice.init(len, block_size + len);
  ref_tone.init(spk, (f1+f2)/2, 0, block_size + 2 * len);
  ref_slice.init(len, len + block_size);
  test.reset();
  ref_slice.reset();

  diff = calc_diff(&tone, &test, &ref_tone, &ref_slice);
  BOOST_CHECK_GT(diff, 0);
  BOOST_CHECK_LT(value2db(diff), -a);

  /////////////////////////////////////////////////////////
  // Tones at stop bands must be filtered out

  tone.init(spk, f1-df, 0, block_size + 2 * len);
  slice.init(len, block_size + len);
  test.reset();

  level = calc_peak(&tone, &test);
  BOOST_CHECK_GT(level, 0);
  BOOST_CHECK_LT(value2db(level), -a);

  tone.init(spk, f2+df, 0, block_size + 2 * len);
  slice.init(len, block_size + len);
  test.reset();

  level = calc_peak(&tone, &test);
  BOOST_CHECK_GT(level, 0);
  BOOST_CHECK_LT(value2db(level), -a);

  /////////////////////////////////////////////////////////
  // BandStop
  /////////////////////////////////////////////////////////

  gen.set(ParamFIR::band_stop, f1, f2, df, a);
  fir.reset(gen.make(spk.sample_rate));
  BOOST_REQUIRE(fir);
  len = fir->length;

  /////////////////////////////////////////////////////////
  // Tones at pass bands must remain unchanged

  tone.init(spk, f1-df, 0, block_size + 2 * len);
  slice.init(len, block_size + len);
  ref_tone.init(spk, f1-df, 0, block_size + 2 * len);
  ref_slice.init(len, len + block_size);
  test.reset();
  ref_slice.reset();

  diff = calc_diff(&tone, &test, &ref_tone, &ref_slice);
  BOOST_CHECK_GT(diff, 0);
  BOOST_CHECK_LT(value2db(diff), -a);

  tone.init(spk, f2+df, 0, block_size + 2 * len);
  slice.init(len, block_size + len);
  ref_tone.init(spk, f2+df, 0, block_size + 2 * len);
  ref_slice.init(len, len + block_size);
  test.reset();
  ref_slice.reset();

  diff = calc_diff(&tone, &test, &ref_tone, &ref_slice);
  BOOST_CHECK_GT(diff, 0);
  BOOST_CHECK_LT(value2db(diff), -a);

  /////////////////////////////////////////////////////////
  // Tone in the stop band must be filtered out

  tone.init(spk, (f1+f2)/2, 0, block_size + 2 * len);
  slice.init(len, block_size + len);
  test.reset();

  level = calc_peak(&tone, &test);
  BOOST_CHECK_GT(level, 0);
  BOOST_CHECK_LT(value2db(level), -a);
}

BOOST_AUTO_TEST_SUITE_END()
