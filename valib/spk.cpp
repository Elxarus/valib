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

extern const Speakers def_spk = Speakers(FORMAT_PCM16, MODE_STEREO, 48000, 32767, NO_RELATION);
extern const Speakers err_spk = Speakers(FORMAT_UNKNOWN, 0, 0, 0, 0);
extern const Speakers unk_spk = Speakers(FORMAT_UNKNOWN, 0, 0, 0, 0);

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
