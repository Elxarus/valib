#include "spk.h"

/*
#define FORMAT_UNKNOWN     0
#define FORMAT_LINEAR      1

#define FORMAT_PCM16       2
#define FORMAT_PCM24       3
#define FORMAT_PCM32       4
#define FORMAT_PCMFLOAT    5

#define FORMAT_PCM16_LE    6
#define FORMAT_PCM24_LE    7
#define FORMAT_PCM32_LE    8
#define FORMAT_PCMFLOAT_LE 9

// data containers
#define FORMAT_PES        10
#define FORMAT_SPDIF      11

// compressed formats
#define FORMAT_AC3        12
#define FORMAT_MPA        13
#define FORMAT_DTS        14
#define FORMAT_AAC        15
#define FORMAT_OGG        16
*/

extern const int std_order[NCHANNELS] = 
{ 0, 1, 2, 3, 4, 5 };

extern const int win_order[NCHANNELS] = 
{ CH_L, CH_R, CH_C, CH_LFE, CH_SL, CH_SR };

extern const Speakers spk_unknown = Speakers(FORMAT_UNKNOWN, 0, 0, 0, 0);
/*
extern const Speakers def_spk = Speakers(FORMAT_LINEAR, MODE_STEREO, 48000, 1.0, NO_RELATION);
extern const Speakers err_spk = Speakers(FORMAT_UNKNOWN, 0, 0, 0, 0);
extern const Speakers unk_spk = Speakers(FORMAT_UNKNOWN, 0, 0, 0, 0);
*/

extern const Speakers stereo_spk = Speakers(FORMAT_PCM16, MODE_STEREO, 48000, 32768.0, NO_RELATION); // stereo 16bit

extern const int sample_size_tbl[32] = 
{
  0,
  sizeof(sample_t), 

  sizeof(int16_t),
  sizeof(int24_t),
  sizeof(int32_t),
  sizeof(float),

  sizeof(int16_t),
  sizeof(int24_t),
  sizeof(int32_t),
  sizeof(float),
  1, 1,             // PES/SPDIF
  1, 1, 1, 1, 1,    // compresed formats

  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

extern const int mask_nch_tbl[64] = 
{
  0, 1, 1, 2, 1, 2, 2, 3, 
  1, 2, 2, 3, 2, 3, 3, 4, 
  1, 2, 2, 3, 2, 3, 3, 4, 
  2, 3, 3, 4, 3, 4, 4, 5, 
  1, 2, 2, 3, 2, 3, 3, 4, 
  2, 3, 3, 4, 3, 4, 4, 5, 
  2, 3, 3, 4, 3, 4, 4, 5, 
  3, 4, 4, 5, 4, 5, 5, 6 
};

extern const int mask_order_tbl[64][6] =
{
{ CH_NONE },
{ CH_L },
{ CH_C },
{ CH_L, CH_C },
{ CH_R },
{ CH_L, CH_R },
{ CH_C, CH_R },
{ CH_L, CH_C, CH_R },
{ CH_SL },
{ CH_L, CH_SL },
{ CH_C, CH_SL },
{ CH_L, CH_C, CH_SL },
{ CH_R, CH_SL },
{ CH_L, CH_R, CH_SL },
{ CH_C, CH_R, CH_SL },
{ CH_L, CH_C, CH_R, CH_SL },
{ CH_SR },
{ CH_L, CH_SR },
{ CH_C, CH_SR },
{ CH_L, CH_C, CH_SR },
{ CH_R, CH_SR },
{ CH_L, CH_R, CH_SR },
{ CH_C, CH_R, CH_SR },
{ CH_L, CH_C, CH_R, CH_SR },
{ CH_SL, CH_SR },
{ CH_L, CH_SL, CH_SR },
{ CH_C, CH_SL, CH_SR },
{ CH_L, CH_C, CH_SL, CH_SR },
{ CH_R, CH_SL, CH_SR },
{ CH_L, CH_R, CH_SL, CH_SR },
{ CH_C, CH_R, CH_SL, CH_SR },
{ CH_L, CH_C, CH_R, CH_SL, CH_SR },
{ CH_LFE },
{ CH_L, CH_LFE },
{ CH_C, CH_LFE },
{ CH_L, CH_C, CH_LFE },
{ CH_R, CH_LFE },
{ CH_L, CH_R, CH_LFE },
{ CH_C, CH_R, CH_LFE },
{ CH_L, CH_C, CH_R, CH_LFE },
{ CH_SL, CH_LFE },
{ CH_L, CH_SL, CH_LFE },
{ CH_C, CH_SL, CH_LFE },
{ CH_L, CH_C, CH_SL, CH_LFE },
{ CH_R, CH_SL, CH_LFE },
{ CH_L, CH_R, CH_SL, CH_LFE },
{ CH_C, CH_R, CH_SL, CH_LFE },
{ CH_L, CH_C, CH_R, CH_SL, CH_LFE },
{ CH_SR, CH_LFE },
{ CH_L, CH_SR, CH_LFE },
{ CH_C, CH_SR, CH_LFE },
{ CH_L, CH_C, CH_SR, CH_LFE },
{ CH_R, CH_SR, CH_LFE },
{ CH_L, CH_R, CH_SR, CH_LFE },
{ CH_C, CH_R, CH_SR, CH_LFE },
{ CH_L, CH_C, CH_R, CH_SR, CH_LFE },
{ CH_SL, CH_SR, CH_LFE },
{ CH_L, CH_SL, CH_SR, CH_LFE },
{ CH_C, CH_SL, CH_SR, CH_LFE },
{ CH_L, CH_C, CH_SL, CH_SR, CH_LFE },
{ CH_R, CH_SL, CH_SR, CH_LFE },
{ CH_L, CH_R, CH_SL, CH_SR, CH_LFE },
{ CH_C, CH_R, CH_SL, CH_SR, CH_LFE },
{ CH_L, CH_C, CH_R, CH_SL, CH_SR, CH_LFE },
};

extern const char *mode_text[64] =
{
  "-", 
  "{ L }",
  "1/0 (mono)",
  "{ L, C }",
  "{ R }",
  "2/0 (stereo)",
  "{ C, R }",
  "3/0",
  "{ SL }",
  "{ L, SL }",
  "{ C, SL }",
  "{ L, C, SL }",
  "{ R, SL }",
  "2/1 (surround)",
  "{ C, R, SL }",
  "3/1 (surround)",
  "{ SR }",
  "{ L, SR }",
  "{ C, SR }",
  "{ L, C, SR }",
  "{ R, SR }",
  "{ L, R, SR }",
  "{ C, R, SR }",
  "{ L, C, R, SR }",
  "{ SL, SR }",
  "{ L, SL, SR }",
  "{ C, SL, SR }",
  "{ L, C, SL, SR }",
  "{ R, SL, SR }",
  "2/2 (quadro)",
  "{ C, R, SL, SR }",
  "3/2 (5 channels)",
  "{ LFE }",
  "{ L, LFE }",
  "1/0.1",
  "{ L, C, LFE }",
  "{ R, LFE }",
  "2/0.1 (2.1)",
  "{ C, R, LFE }",
  "3/0.1",
  "{ SL, LFE }",
  "{ L, SL, LFE }",
  "{ C, SL, LFE }",
  "{ L, C, SL, LFE }",
  "{ R, SL, LFE }",
  "2/1.1",
  "{ C, R, SL, LFE }",
  "3/1.1",
  "{ SR, LFE }",
  "{ L, SR, LFE }",
  "{ C, SR, LFE }",
  "{ L, C, SR, LFE }",
  "{ R, SR, LFE }",
  "{ L, R, SR, LFE }",
  "{ C, R, SR, LFE }",
  "{ L, C, R, SR, LFE }",
  "{ SL, SR, LFE }",
  "{ L, SL, SR, LFE }",
  "{ C, SL, SR, LFE }",
  "{ L, C, SL, SR, LFE }",
  "{ R, SL, SR, LFE }",
  "2/2.1 (4.1)",
  "{ C, R, SL, SR, LFE }",
  "3/2.1 (5.1)"
};
