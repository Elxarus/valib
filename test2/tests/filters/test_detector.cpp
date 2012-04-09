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

BOOST_AUTO_TEST_CASE(open)
{
  static const Speakers good[] =
  {
    Speakers(FORMAT_RAWDATA, 0, 0),
    Speakers(FORMAT_PCM16, MODE_STEREO, 48000),
    Speakers(FORMAT_AAC_ADTS, 0, 0),
    Speakers(FORMAT_AC3,   0, 0),
    Speakers(FORMAT_EAC3,  0, 0),
    Speakers(FORMAT_DTS,   0, 0),
    Speakers(FORMAT_MPA,   0, 0),
    Speakers(FORMAT_SPDIF, 0, 0)
  };
  static const Speakers bad[] =
  {
    Speakers(FORMAT_UNKNOWN, MODE_STEREO, 48000),
    Speakers(FORMAT_PCM16, MODE_5_1, 48000),
    Speakers(FORMAT_LINEAR, MODE_STEREO, 48000)
  };

  int i;
  Detector f;

  for (i = 0; i < array_size(good); i++)
    BOOST_CHECK_MESSAGE(f.open(good[i]), "Cannot open good format " << good[i].print());

  for (i = 0; i < array_size(bad); i++)
    BOOST_CHECK_MESSAGE(!f.open(bad[i]), "Can open bad format " << bad[i].print());
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
  Speakers spk_dts(FORMAT_SPDIF, MODE_5_1, 48000);
  RAWSource pcm(spk_pcm, "a.dts.03f.spdifdts");
  RAWSource spdif(spk_dts, "a.dts.03f.spdifdts");

  Detector f;
  compare(&pcm, &f, &spdif, 0);
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
