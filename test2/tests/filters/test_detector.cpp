/*
  Detector class test
*/

#include <boost/test/unit_test.hpp>
#include "filters/detector.h"
#include "source/generator.h"
#include "source/raw_source.h"
#include "../../suite.h"

static const int seed = 894756987;
static const size_t block_size = 65536;

BOOST_AUTO_TEST_SUITE(detector)

BOOST_AUTO_TEST_CASE(constructor)
{
  Detector f;
}

BOOST_AUTO_TEST_CASE(pcm_passthrough)
{
  Speakers spk(FORMAT_PCM16, MODE_STEREO, 48000);
  NoiseGen ref(spk, seed, block_size);
  NoiseGen src(spk, seed, block_size);

  Detector f;
  compare(&src, &f, &ref, 0);
}

BOOST_AUTO_TEST_CASE(spdif_detection)
{
  Speakers spk_pcm(FORMAT_PCM16, MODE_STEREO, 48000);
  Speakers spk_spdif(FORMAT_SPDIF, MODE_5_1, 48000);
  RAWSource pcm(spk_pcm, "a.ac3.03f.spdif");
  RAWSource spdif(spk_spdif, "a.ac3.03f.spdif");

  Detector f;
  compare(&pcm, &f, &spdif, 0);
}

BOOST_AUTO_TEST_CASE(dts_detection)
{
  Speakers spk_pcm(FORMAT_PCM16, MODE_STEREO, 48000);
  Speakers spk_dts(FORMAT_DTS, MODE_5_1, 48000);
  RAWSource pcm(spk_pcm, "a.dts.03f.dts");
  RAWSource dts(spk_dts, "a.dts.03f.dts");

  Detector f;
  compare(&pcm, &f, &dts, 0);
}

BOOST_AUTO_TEST_CASE(ac3_detection)
{
  Speakers spk_unk(FORMAT_RAWDATA, 0, 0);
  Speakers spk_ac3(FORMAT_AC3, MODE_5_1, 48000);
  RAWSource unk(spk_unk, "a.ac3.03f.ac3");
  RAWSource ac3(spk_ac3, "a.ac3.03f.ac3");

  Detector f;
  compare(&unk, &f, &ac3, 0);
}

BOOST_AUTO_TEST_SUITE_END()
