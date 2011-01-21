/*
  EqFIR test
*/

#include "../../suite.h"
#include "fir/eq_fir.h"
#include "filters/convolver.h"
#include "filters/gain.h"
#include "filters/slice.h"
#include "source/generator.h"
#include "source/source_filter.h"
#include <boost/scoped_ptr.hpp>
#include <boost/test/unit_test.hpp>

static const int sample_rate = 48000;
static const EqBand bands[] = { { 1000, 1.0 }, { 2000, 2.0 }, { 3000, 1e-5 }, { 4000, 1.0 } };
static const size_t nbands = array_size(bands);

BOOST_TEST_DONT_PRINT_LOG_VALUE(EqBand);

BOOST_AUTO_TEST_SUITE(eq_fir)

BOOST_AUTO_TEST_CASE(constructor)
{
  EqBand band;
  EqFIR fir;
  BOOST_CHECK_EQUAL(fir.get_nbands(), 0);
  BOOST_CHECK_EQUAL(fir.get_bands(&band, 0, 1), 0);
}

BOOST_AUTO_TEST_CASE(init_constructor)
{
  EqBand result_bands[nbands];
  EqFIR fir(bands, nbands);

  BOOST_CHECK_EQUAL(fir.get_nbands(), nbands);
  BOOST_CHECK_EQUAL(fir.get_bands(result_bands, 0, nbands), nbands);
  for (size_t i = 0; i < nbands; i++)
  {
    BOOST_CHECK_EQUAL(result_bands[i].freq, bands[i].freq);
    BOOST_CHECK_EQUAL(result_bands[i].gain, bands[i].gain);
  }
}

BOOST_AUTO_TEST_CASE(set_bands)
{
  EqBand result_bands[nbands];
  int ver;
  EqFIR fir;

  ver = fir.version();
  fir.set_bands(bands, nbands);
  BOOST_CHECK_NE(fir.version(), ver);

  // Get all bands at once
  BOOST_CHECK_EQUAL(fir.get_nbands(), nbands);
  BOOST_CHECK_EQUAL(fir.get_bands(result_bands, 0, nbands), nbands);
  for (size_t i = 0; i < nbands; i++)
  {
    BOOST_CHECK_EQUAL(result_bands[i].freq, bands[i].freq);
    BOOST_CHECK_EQUAL(result_bands[i].gain, bands[i].gain);
  }

  // Get bands one by one
  for (size_t i = 0; i < nbands; i++)
  {
    BOOST_CHECK_EQUAL(fir.get_bands(result_bands, i, 1), 1);
    BOOST_CHECK_EQUAL(result_bands[0].freq, bands[i].freq);
    BOOST_CHECK_EQUAL(result_bands[0].gain, bands[i].gain);
  }

  // Clear bands
  ver = fir.version();
  fir.clear_bands();
  BOOST_CHECK_NE(fir.version(), ver);
  BOOST_CHECK_EQUAL(fir.get_nbands(), 0);
  BOOST_CHECK_EQUAL(fir.get_bands(result_bands, 0, 1), 0);
}

BOOST_AUTO_TEST_CASE(set_ripple)
{
  double ripple;
  int ver;
  EqFIR fir;

  ripple = fir.get_ripple() / 2;
  ver = fir.version();
  fir.set_ripple(ripple);
  BOOST_CHECK_NE(fir.version(), ver);
  BOOST_CHECK_EQUAL(fir.get_ripple(), ripple);

  ripple = ripple / 2;
  ver = fir.version();
  fir.set_ripple(ripple);
  BOOST_CHECK_NE(fir.version(), ver);
  BOOST_CHECK_EQUAL(fir.get_ripple(), ripple);
}

// Uninitialized filter produces passthrough response
BOOST_AUTO_TEST_CASE(make_identity)
{
  EqFIR fir;
  boost::scoped_ptr<const FIRInstance> inst;

  inst.reset(fir.make(sample_rate));
  BOOST_CHECK_EQUAL(inst->type(), firt_identity);
}

// Equalizer with one band set produces gain response
BOOST_AUTO_TEST_CASE(make_gain)
{
  const double gain = 2.0;
  const EqBand band = { sample_rate / 4, gain };

  EqFIR fir;
  boost::scoped_ptr<const FIRInstance> inst;

  fir.set_bands(&band, 1);
  inst.reset(fir.make(sample_rate));
  BOOST_CHECK_EQUAL(inst->type(), firt_gain);
  BOOST_CHECK_EQUAL(inst->data[0], gain);
}

// Equalizer with multiple bands.
// Test with help of convolver filter and sine wave generator.
BOOST_AUTO_TEST_CASE(make_eq)
{
  const size_t block_size = 65536;
  const Speakers spk(FORMAT_LINEAR, MODE_STEREO, sample_rate);

  // Test source test_src (tone -> convolver -> slice)
  ToneGen tone;
  Convolver conv;
  SliceFilter slice;
  SourceFilter test_src1(&tone, &conv);
  SourceFilter test_src(&test_src1, &slice);

  // Reference source ref_src (tone -> gain -> slice)
  ToneGen ref_tone;
  Gain ref_gain;
  SliceFilter ref_slice;
  SourceFilter ref_src1(&ref_tone, &ref_gain);
  SourceFilter ref_src(&ref_src1, &ref_slice);

  // EqFIR
  EqFIR fir(bands, nbands);
  conv.set_fir(&fir);

  /////////////////////////////////////////////////////////
  // Determine filter length

  boost::scoped_ptr<const FIRInstance> inst;
  inst.reset(fir.make(sample_rate));
  BOOST_REQUIRE(inst);
  size_t len = inst->length;

  /////////////////////////////////////////////////////////
  // Tone in the pass band must remain unchanged

  for (size_t iband = 0; iband < nbands; iband++)
  {
    int freq = bands[iband].freq;
    double gain = bands[iband].gain;

    tone.init(spk, freq, 0, block_size + 2 * len);
    slice.init(len, block_size + len);
    ref_tone.init(spk, freq, 0, block_size + 2 * len);
    ref_gain.gain = gain;
    ref_slice.init(len, len + block_size);

    test_src.reset();
    ref_src.reset();

    double diff = calc_diff(&test_src, &ref_src);
    BOOST_CHECK_GT(diff, 0);
    BOOST_CHECK_LT(value2db(diff), fir.get_ripple());
  }
}

BOOST_AUTO_TEST_SUITE_END()
