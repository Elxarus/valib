/*
  Mixer test
*/

#include <boost/test/unit_test.hpp>
#include "filters/mixer.h"
#include "filters/gain.h"
#include "filters/filter_graph.h"
#include "source/generator.h"
#include "../../suite.h"

static const int seed = 934759385;
static const size_t block_size = 65536;


static inline const char *ch_text(int ch)
{
  switch (ch)
  {
  case CH_L: return "l";
  case CH_C: return "c";
  case CH_R: return "r";
  case CH_SL: return "sl";
  case CH_SR: return "sr";
  case CH_LFE: return "lfe";
  case CH_CL: return "cl";
  case CH_CR: return "cr";
  case CH_BL: return "bl";
  case CH_BC: return "bc";
  case CH_BR: return "br";
  default: return 0;
  }
}

static string
mask_text(int mask)
{
  string s;
  string space(" ");
  for (int ch = 0; ch < CH_NAMES; ch++)
    if (mask & CH_MASK(ch))
      s += string(ch_text(ch)) + space;

  s.resize(s.size() - 1);
  return s;
}

static string
serialize_matrix(const matrix_t &m)
{
  string s;
  char v[64];
  for (int ch1 = 0; ch1 < CH_NAMES; ch1++)
    for (int ch2 = 0; ch2 < CH_NAMES; ch2++)
      if (m[ch1][ch2] != 0)
      {
        sprintf(v, "%s_%s:%.1f ", ch_text(ch1), ch_text(ch2), m[ch1][ch2]);
        s += string(v);
      }

  s.resize(s.size() - 1);
  return s;
}

static string
serialize_matrix(Mixer &mixer, Speakers from, Speakers to)
{
  matrix_t m;
  mixer.close();
  mixer.set_output(to);
  mixer.open(from);
  mixer.get_matrix(m);
  return serialize_matrix(m);
}

static string
serialize_matrix(Mixer &mixer, int mask_from, int mask_to)
{
  return serialize_matrix(mixer, 
    Speakers(FORMAT_LINEAR, mask_from, 48000),
    Speakers(FORMAT_LINEAR, mask_to, 48000));
}

BOOST_AUTO_TEST_SUITE(mixer)

BOOST_AUTO_TEST_CASE(constructor)
{
  Mixer mixer(1024);

  // Default values
  BOOST_CHECK_EQUAL(mixer.get_buffer_size(), 1024);

  BOOST_CHECK_EQUAL(mixer.get_auto_matrix(), true);
  BOOST_CHECK_EQUAL(mixer.get_normalize_matrix(), true);
  BOOST_CHECK_EQUAL(mixer.get_voice_control(), true);
  BOOST_CHECK_EQUAL(mixer.get_expand_stereo(), true);
  BOOST_CHECK_EQUAL(mixer.get_clev(), 1.0);
  BOOST_CHECK_EQUAL(mixer.get_slev(), 1.0);
  BOOST_CHECK_EQUAL(mixer.get_lfelev(), 1.0);
  BOOST_CHECK_EQUAL(mixer.get_gain(), 1.0);

  sample_t input_gains[CH_NAMES];
  sample_t output_gains[CH_NAMES];
  mixer.get_input_gains(input_gains);
  mixer.get_output_gains(output_gains);
  for (int ch_name = 0; ch_name < CH_NAMES; ch_name++)
  {
    BOOST_CHECK_EQUAL(input_gains[ch_name], 1.0);
    BOOST_CHECK_EQUAL(output_gains[ch_name], 1.0);
  }
}

BOOST_AUTO_TEST_CASE(auto_matrix)
{
  // Test auto matrix generation without upmixing
  enum {
    mono          = MODE_MONO,
    stereo        = MODE_STEREO,
    front_3ch     = MODE_3_0,
    front_4ch     = MODE_STEREO | CH_MASK_CL | CH_MASK_CR,
    front_5ch     = MODE_5_0,
    surround      = MODE_2_1,
    quadro_side   = MODE_2_2,
    quadro_back   = MODE_2_0_2,
    mode_5ch_side = MODE_3_2,
    mode_5ch_back = MODE_3_0_2,
    mode_7ch_side = MODE_5_2,
    mode_7ch_back = MODE_5_0_2,

    stereo_lfe    = MODE_2_0_LFE,
    mode_5_1      = MODE_3_2_LFE
  };

  struct
  {
    int in_mask, out_mask;
    const char *matrix;
  } tests[] =
  {
    { mono, mono,      "c_c:1.0"},
    { mono, stereo,    "c_l:0.7 c_r:0.7"},
    { mono, front_3ch, "c_c:1.0"},
    { mono, front_4ch, "c_cl:0.7 c_cr:0.7"},
    { mono, mode_5_1,  "c_c:1.0"},

    { stereo, mono,     "l_c:1.0 r_c:1.0"},
    { stereo, stereo,   "l_l:1.0 r_r:1.0"},
    { stereo, mode_5_1, "l_l:1.0 r_r:1.0"},

    { front_3ch, mono,      "l_c:1.0 c_c:1.0 r_c:1.0"},
    { front_3ch, stereo,    "l_l:1.0 c_l:0.7 c_r:0.7 r_r:1.0"},
    { front_3ch, front_3ch, "l_l:1.0 c_c:1.0 r_r:1.0"},
    { front_3ch, front_4ch, "l_l:1.0 c_cl:0.7 c_cr:0.7 r_r:1.0"},
    { front_3ch, front_5ch, "l_l:1.0 c_c:1.0 r_r:1.0"},
    { front_3ch, mode_5_1,  "l_l:1.0 c_c:1.0 r_r:1.0"},

    { front_4ch, mono,      "l_c:1.0 r_c:1.0 cl_c:1.0 cr_c:1.0"},
    { front_4ch, stereo,    "l_l:1.0 r_r:1.0 cl_l:0.9 cl_r:0.5 cr_l:0.5 cr_r:0.9"},
    { front_4ch, front_3ch, "l_l:1.0 r_r:1.0 cl_l:0.7 cl_c:0.7 cr_c:0.7 cr_r:0.7"},
    { front_4ch, front_4ch, "l_l:1.0 r_r:1.0 cl_cl:1.0 cr_cr:1.0"},
    { front_4ch, front_5ch, "l_l:1.0 r_r:1.0 cl_cl:1.0 cr_cr:1.0"},
    { front_4ch, mode_5_1,  "l_l:1.0 r_r:1.0 cl_l:0.7 cl_c:0.7 cr_c:0.7 cr_r:0.7"},

    { surround, mono,        "l_c:1.0 r_c:1.0 bc_c:1.0"},
    { surround, stereo,      "l_l:1.0 r_r:1.0 bc_l:0.7 bc_r:0.7"},
    { surround, quadro_side, "l_l:1.0 r_r:1.0 bc_sl:0.7 bc_sr:0.7"},
    { surround, quadro_back, "l_l:1.0 r_r:1.0 bc_bl:0.7 bc_br:0.7"},
    { surround, mode_5_1,    "l_l:1.0 r_r:1.0 bc_sl:0.7 bc_sr:0.7"},

    { quadro_side, mono,        "l_c:1.0 r_c:1.0 sl_c:1.0 sr_c:1.0"},
    { quadro_side, stereo,      "l_l:1.0 r_r:1.0 sl_l:1.0 sr_r:1.0"},
    { quadro_side, surround,    "l_l:1.0 r_r:1.0 sl_bc:1.0 sr_bc:1.0"},
    { quadro_side, quadro_side, "l_l:1.0 r_r:1.0 sl_sl:1.0 sr_sr:1.0"},
    { quadro_side, quadro_back, "l_l:1.0 r_r:1.0 sl_bl:1.0 sr_br:1.0"},
    { quadro_side, mode_5_1,    "l_l:1.0 r_r:1.0 sl_sl:1.0 sr_sr:1.0"},

    { quadro_back, mono,        "l_c:1.0 r_c:1.0 bl_c:1.0 br_c:1.0"},
    { quadro_back, stereo,      "l_l:1.0 r_r:1.0 bl_l:1.0 br_r:1.0"},
    { quadro_back, surround,    "l_l:1.0 r_r:1.0 bl_bc:1.0 br_bc:1.0"},
    { quadro_back, quadro_side, "l_l:1.0 r_r:1.0 bl_sl:1.0 br_sr:1.0"},
    { quadro_back, quadro_back, "l_l:1.0 r_r:1.0 bl_bl:1.0 br_br:1.0"},
    { quadro_back, mode_5_1,    "l_l:1.0 r_r:1.0 bl_sl:1.0 br_sr:1.0"},

    { mode_5ch_side, stereo,        "l_l:1.0 c_l:0.7 c_r:0.7 r_r:1.0 sl_l:1.0 sr_r:1.0"},
    { mode_5ch_side, mode_5ch_side, "l_l:1.0 c_c:1.0 r_r:1.0 sl_sl:1.0 sr_sr:1.0"},
    { mode_5ch_side, mode_5ch_back, "l_l:1.0 c_c:1.0 r_r:1.0 sl_bl:1.0 sr_br:1.0"},
    { mode_5ch_side, mode_7ch_back, "l_l:1.0 c_c:1.0 r_r:1.0 sl_bl:1.0 sr_br:1.0"},

    { mode_5ch_back, stereo,        "l_l:1.0 c_l:0.7 c_r:0.7 r_r:1.0 bl_l:1.0 br_r:1.0"},
    { mode_5ch_back, mode_5ch_side, "l_l:1.0 c_c:1.0 r_r:1.0 bl_sl:1.0 br_sr:1.0"},
    { mode_5ch_back, mode_5ch_back, "l_l:1.0 c_c:1.0 r_r:1.0 bl_bl:1.0 br_br:1.0"},
    { mode_5ch_back, mode_7ch_side, "l_l:1.0 c_c:1.0 r_r:1.0 bl_sl:1.0 br_sr:1.0"},

    { mode_7ch_side, stereo,        "l_l:1.0 c_l:0.7 c_r:0.7 r_r:1.0 sl_l:1.0 sr_r:1.0 cl_l:0.9 cl_r:0.5 cr_l:0.5 cr_r:0.9"},
    { mode_7ch_side, mode_5ch_side, "l_l:1.0 c_c:1.0 r_r:1.0 sl_sl:1.0 sr_sr:1.0 cl_l:0.7 cl_c:0.7 cr_c:0.7 cr_r:0.7"},
    { mode_7ch_side, mode_5ch_back, "l_l:1.0 c_c:1.0 r_r:1.0 sl_bl:1.0 sr_br:1.0 cl_l:0.7 cl_c:0.7 cr_c:0.7 cr_r:0.7"},
    { mode_7ch_side, mode_7ch_side, "l_l:1.0 c_c:1.0 r_r:1.0 sl_sl:1.0 sr_sr:1.0 cl_cl:1.0 cr_cr:1.0"},
    { mode_7ch_side, mode_7ch_back, "l_l:1.0 c_c:1.0 r_r:1.0 sl_bl:1.0 sr_br:1.0 cl_cl:1.0 cr_cr:1.0"},

    { mode_7ch_back, stereo,        "l_l:1.0 c_l:0.7 c_r:0.7 r_r:1.0 cl_l:0.9 cl_r:0.5 cr_l:0.5 cr_r:0.9 bl_l:1.0 br_r:1.0"},
    { mode_7ch_back, mode_5ch_side, "l_l:1.0 c_c:1.0 r_r:1.0 cl_l:0.7 cl_c:0.7 cr_c:0.7 cr_r:0.7 bl_sl:1.0 br_sr:1.0"},
    { mode_7ch_back, mode_5ch_back, "l_l:1.0 c_c:1.0 r_r:1.0 cl_l:0.7 cl_c:0.7 cr_c:0.7 cr_r:0.7 bl_bl:1.0 br_br:1.0"},
    { mode_7ch_back, mode_7ch_side, "l_l:1.0 c_c:1.0 r_r:1.0 cl_cl:1.0 cr_cr:1.0 bl_sl:1.0 br_sr:1.0"},
    { mode_7ch_back, mode_7ch_back, "l_l:1.0 c_c:1.0 r_r:1.0 cl_cl:1.0 cr_cr:1.0 bl_bl:1.0 br_br:1.0"},

    // LFE
    { stereo, stereo_lfe, "l_l:1.0 r_r:1.0"},
    { stereo, mode_5_1,   "l_l:1.0 r_r:1.0"},
    { stereo_lfe, stereo, "l_l:1.0 r_r:1.0 lfe_l:0.7 lfe_r:0.7"},
    { stereo_lfe, stereo_lfe, "l_l:1.0 r_r:1.0 lfe_lfe:1.0"},
    { stereo_lfe, mode_5_1, "l_l:1.0 r_r:1.0 lfe_lfe:1.0"},
    { stereo_lfe, mode_5ch_side, "l_l:1.0 r_r:1.0 lfe_l:0.7 lfe_r:0.7"},
    { mode_5_1, stereo, "l_l:1.0 c_l:0.7 c_r:0.7 r_r:1.0 sl_l:1.0 sr_r:1.0 lfe_l:0.7 lfe_r:0.7"},
    { mode_5_1, stereo_lfe, "l_l:1.0 c_l:0.7 c_r:0.7 r_r:1.0 sl_l:1.0 sr_r:1.0 lfe_lfe:1.0"},
    { mode_5_1, mode_5_1, "l_l:1.0 c_c:1.0 r_r:1.0 sl_sl:1.0 sr_sr:1.0 lfe_lfe:1.0"},
    { mode_5_1, mode_5ch_side, "l_l:1.0 c_c:1.0 r_r:1.0 sl_sl:1.0 sr_sr:1.0 lfe_l:0.7 lfe_r:0.7"},
  };

  Mixer mixer(1024);
  mixer.set_normalize_matrix(false);
  mixer.set_expand_stereo(false);
  mixer.set_voice_control(true);

  for (int i = 0; i < array_size(tests); i++)
  {
    string matrix = serialize_matrix(mixer, tests[i].in_mask, tests[i].out_mask);
    BOOST_CHECK_MESSAGE(matrix == string(tests[i].matrix),
      string("Bad matrix for ") << mask_text(tests[i].in_mask) <<
      ">"<< mask_text(tests[i].out_mask) << ":"<< matrix);
  }
}

BOOST_AUTO_TEST_CASE(voice_level)
{
  Mixer mixer(1024);
  mixer.set_normalize_matrix(false);
  mixer.set_expand_stereo(false);
  mixer.set_voice_control(false);

  mixer.set_clev(0.5);
  BOOST_CHECK_EQUAL(mixer.get_clev(), 0.5);

  // No mix, just gain the center.
  BOOST_CHECK_EQUAL(serialize_matrix(mixer, MODE_3_2, MODE_3_2), "l_l:1.0 c_c:0.5 r_r:1.0 sl_sl:1.0 sr_sr:1.0");
  BOOST_CHECK_EQUAL(serialize_matrix(mixer, MODE_MONO, MODE_MONO), "c_c:0.5");
  // Downmix to mono. Gain center, do not gain other channels.
  BOOST_CHECK_EQUAL(serialize_matrix(mixer, MODE_3_2, MODE_MONO), "l_c:1.0 c_c:0.5 r_c:1.0 sl_c:1.0 sr_c:1.0");
  // Upmix from mono.
  BOOST_CHECK_EQUAL(serialize_matrix(mixer, MODE_MONO, MODE_STEREO), "c_l:0.4 c_r:0.4");
  // Mix from center to main channels. Gain center, do not gain other channels.
  BOOST_CHECK_EQUAL(serialize_matrix(mixer, MODE_3_2, MODE_QUADRO), "l_l:1.0 c_l:0.4 c_r:0.4 r_r:1.0 sl_sl:1.0 sr_sr:1.0");
}

BOOST_AUTO_TEST_CASE(surround_level)
{
  Mixer mixer(1024);
  mixer.set_normalize_matrix(false);
  mixer.set_expand_stereo(false);
  mixer.set_voice_control(true);

  mixer.set_slev(0.5);
  BOOST_CHECK_EQUAL(mixer.get_slev(), 0.5);

  // No mix, just gain the surround.
  BOOST_CHECK_EQUAL(serialize_matrix(mixer, MODE_3_2, MODE_3_2), "l_l:1.0 c_c:1.0 r_r:1.0 sl_sl:0.5 sr_sr:0.5");
  // Downmix to mono. Gain surround, do not gain other channels.
  BOOST_CHECK_EQUAL(serialize_matrix(mixer, MODE_3_2, MODE_MONO), "l_c:1.0 c_c:1.0 r_c:1.0 sl_c:0.5 sr_c:0.5");
  // Intercahnge back and surround channels. Do gain.
  BOOST_CHECK_EQUAL(serialize_matrix(mixer, MODE_3_2, MODE_3_0_2), "l_l:1.0 c_c:1.0 r_r:1.0 sl_bl:0.5 sr_br:0.5");
  // Mix back channel to 2 surround.
  BOOST_CHECK_EQUAL(serialize_matrix(mixer, MODE_3_1, MODE_3_2), "l_l:1.0 c_c:1.0 r_r:1.0 bc_sl:0.4 bc_sr:0.4");
}

BOOST_AUTO_TEST_CASE(lfe_level)
{
  Mixer mixer(1024);
  mixer.set_normalize_matrix(false);
  mixer.set_expand_stereo(false);
  mixer.set_voice_control(true);

  mixer.set_lfelev(0.5);
  BOOST_CHECK_EQUAL(mixer.get_lfelev(), 0.5);

  // No mix, just gain LFE channel.
  BOOST_CHECK_EQUAL(serialize_matrix(mixer, MODE_2_0_LFE, MODE_2_0_LFE), "l_l:1.0 r_r:1.0 lfe_lfe:0.5");
  // Downmix to mono
  BOOST_CHECK_EQUAL(serialize_matrix(mixer, MODE_2_0_LFE, MODE_MONO), "l_c:1.0 r_c:1.0 lfe_c:0.5");
  // Downmix to stereo
  BOOST_CHECK_EQUAL(serialize_matrix(mixer, MODE_2_0_LFE, MODE_STEREO), "l_l:1.0 r_r:1.0 lfe_l:0.4 lfe_r:0.4");
}

BOOST_AUTO_TEST_CASE(voice_control)
{
  Mixer mixer(1024);
  mixer.set_normalize_matrix(false);
  mixer.set_expand_stereo(false);
  mixer.set_voice_control(true);

  mixer.set_clev(0.5);
  // No mixing
  BOOST_CHECK_EQUAL(serialize_matrix(mixer, MODE_STEREO, MODE_STEREO), "l_l:0.8 l_r:-0.3 r_l:-0.3 r_r:0.8");
  BOOST_CHECK_EQUAL(serialize_matrix(mixer, MODE_QUADRO, MODE_QUADRO), "l_l:0.8 l_r:-0.3 r_l:-0.3 r_r:0.8 sl_sl:1.0 sr_sr:1.0");
  // Downmix
  BOOST_CHECK_EQUAL(serialize_matrix(mixer, MODE_QUADRO, MODE_STEREO), "l_l:0.8 l_r:-0.3 r_l:-0.3 r_r:0.8 sl_l:1.0 sr_r:1.0");
  // Upmix
  BOOST_CHECK_EQUAL(serialize_matrix(mixer, MODE_STEREO, MODE_QUADRO), "l_l:0.8 l_r:-0.3 r_l:-0.3 r_r:0.8");
}

BOOST_AUTO_TEST_CASE(expand_stereo)
{
  Mixer mixer(1024);
  mixer.set_normalize_matrix(false);
  mixer.set_expand_stereo(true);
  mixer.set_voice_control(false);

  mixer.set_clev(0.5);
  mixer.set_slev(2.0);

  // Fill center channel
  BOOST_CHECK_EQUAL(serialize_matrix(mixer, MODE_STEREO, MODE_3_0),   "l_l:1.0 l_c:0.4 r_c:0.4 r_r:1.0");
  BOOST_CHECK_EQUAL(serialize_matrix(mixer, MODE_QUADRO, MODE_3_2),   "l_l:1.0 l_c:0.4 r_c:0.4 r_r:1.0 sl_sl:2.0 sr_sr:2.0");
  // Fill side channels
  BOOST_CHECK_EQUAL(serialize_matrix(mixer, MODE_STEREO, MODE_2_2),   "l_l:1.0 l_sl:1.0 l_sr:-1.0 r_r:1.0 r_sl:-1.0 r_sr:1.0");
  BOOST_CHECK_EQUAL(serialize_matrix(mixer, MODE_3_0,    MODE_3_2),   "l_l:1.0 l_sl:1.0 l_sr:-1.0 c_c:0.5 r_r:1.0 r_sl:-1.0 r_sr:1.0");
  // Fill back channels
  BOOST_CHECK_EQUAL(serialize_matrix(mixer, MODE_STEREO, MODE_2_0_2), "l_l:1.0 l_bl:1.0 l_br:-1.0 r_r:1.0 r_bl:-1.0 r_br:1.0");
  BOOST_CHECK_EQUAL(serialize_matrix(mixer, MODE_3_0,    MODE_3_0_2), "l_l:1.0 l_bl:1.0 l_br:-1.0 c_c:0.5 r_r:1.0 r_bl:-1.0 r_br:1.0");
  // Fill single surround channel
  BOOST_CHECK_EQUAL(serialize_matrix(mixer, MODE_STEREO, MODE_2_1),   "l_l:1.0 l_bc:1.4 r_r:1.0 r_bc:-1.4");
  BOOST_CHECK_EQUAL(serialize_matrix(mixer, MODE_3_0,    MODE_3_1),   "l_l:1.0 l_bc:1.4 c_c:0.5 r_r:1.0 r_bc:-1.4");

  // Do not fill the center
  BOOST_CHECK_EQUAL(serialize_matrix(mixer, MODE_STEREO, MODE_MONO),  "l_c:1.0 r_c:1.0");
  // Do not fill side channels
  BOOST_CHECK_EQUAL(serialize_matrix(mixer, MODE_2_1,    MODE_2_2),   "l_l:1.0 r_r:1.0 bc_sl:1.4 bc_sr:1.4");
  BOOST_CHECK_EQUAL(serialize_matrix(mixer, MODE_2_0_2,  MODE_2_2),   "l_l:1.0 r_r:1.0 bl_sl:2.0 br_sr:2.0");
  // Do not fill back channels
  BOOST_CHECK_EQUAL(serialize_matrix(mixer, MODE_2_1,    MODE_2_0_2), "l_l:1.0 r_r:1.0 bc_bl:1.4 bc_br:1.4");
  BOOST_CHECK_EQUAL(serialize_matrix(mixer, MODE_2_2,    MODE_2_0_2), "l_l:1.0 r_r:1.0 sl_bl:2.0 sr_br:2.0");
  // Do not fill surround channel
  BOOST_CHECK_EQUAL(serialize_matrix(mixer, MODE_2_2,    MODE_2_1),   "l_l:1.0 r_r:1.0 sl_bc:2.0 sr_bc:2.0");
  BOOST_CHECK_EQUAL(serialize_matrix(mixer, MODE_2_0_2,  MODE_2_1),   "l_l:1.0 r_r:1.0 bl_bc:2.0 br_bc:2.0");

  // Stereo -> 5.1
  BOOST_CHECK_EQUAL(serialize_matrix(mixer, MODE_STEREO, MODE_5_1),   "l_l:1.0 l_c:0.4 l_sl:1.0 l_sr:-1.0 r_c:0.4 r_r:1.0 r_sl:-1.0 r_sr:1.0");
}

BOOST_AUTO_TEST_CASE(normalize)
{
  Mixer mixer(1024);
  mixer.set_normalize_matrix(true);
  mixer.set_expand_stereo(false);
  mixer.set_voice_control(true);

  // No normalization
  BOOST_CHECK_EQUAL(serialize_matrix(mixer, MODE_STEREO, MODE_STEREO), "l_l:1.0 r_r:1.0");
  // Decrease loudness
  BOOST_CHECK_EQUAL(serialize_matrix(mixer, MODE_STEREO, MODE_MONO), "l_c:0.5 r_c:0.5");
  // Increase loudness
  mixer.set_clev(0.5);
  BOOST_CHECK_EQUAL(serialize_matrix(mixer, MODE_MONO, MODE_MONO), "c_c:1.0");
  mixer.set_clev(1.0);
  // 5.1 -> stereo
  BOOST_CHECK_EQUAL(serialize_matrix(mixer, MODE_5_1, MODE_STEREO), "l_l:0.3 c_l:0.2 c_r:0.2 r_r:0.3 sl_l:0.3 sr_r:0.3 lfe_l:0.2 lfe_r:0.2");
}

BOOST_AUTO_TEST_CASE(gain)
{
  // Mixer with gain must work as a mixer without gain plus gain.
  // Tested chain:    Mixer
  // Reference chain: Mixer -> Gain
  const double gain = 0.3;
  const Speakers in_spk(FORMAT_LINEAR, MODE_5_1, 48000);
  const Speakers out_spk(FORMAT_LINEAR, MODE_STEREO, 48000);
  
  Mixer test;
  Mixer mixer;
  Gain  gain_filter;

  NoiseGen test_src(in_spk, seed, block_size);
  NoiseGen ref_src(in_spk, seed, block_size);

  test.set_output(out_spk);
  test.set_gain(gain);

  mixer.set_output(out_spk);
  gain_filter.gain = gain;

  compare(&test_src, &test, &ref_src, &FilterChain(&mixer, &gain_filter));
}

BOOST_AUTO_TEST_CASE(gains)
{
  // Test input/output gains effect.
  // Set gain and check the effect for each channel.
  BOOST_WARN_MESSAGE(false, "Not implemented");
}

BOOST_AUTO_TEST_CASE(matrix)
{
  // Test mixing matrix works right
  // Set custom matrix and check the effect.
  // Test each mixing mode, inplace/buffered modes.
  BOOST_WARN_MESSAGE(false, "Not implemented");
}

BOOST_AUTO_TEST_SUITE_END()
