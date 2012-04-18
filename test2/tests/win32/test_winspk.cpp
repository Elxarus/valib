/*
  WAVEFORMAT conversions test
*/

#include <memory>
#include <boost/test/unit_test.hpp>
#include "../../suite.h"
#include "win32/winspk.h"

static const GUID GUID_DOLBY_AC3_SPDIF    = { 0x00000092, 0x0000, 0x0010, { 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71 } };
static const GUID GUID_SUBTYPE_PCM        = { 0x00000001, 0x0000, 0x0010, { 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71 } };
static const GUID GUID_SUBTYPE_IEEE_FLOAT = { 0x00000003, 0x0000, 0x0010, { 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71 } };

static const size_t wf_size = sizeof(WAVEFORMAT);
static const size_t pcmwf_size = sizeof(PCMWAVEFORMAT);
static const size_t wfe_size = sizeof(WAVEFORMATEX);
static const size_t wfx_size = sizeof(WAVEFORMATEXTENSIBLE);

///////////////////////////////////////////////////////////////////////////////

// PCM number of channel series

static WAVEFORMATEX wfe_pcm16_1_48000 = 
{ WAVE_FORMAT_PCM, 1, 48000, 96000, 2, 16, 0 };
static WAVEFORMATEXTENSIBLE wfx_pcm16_1_48000 =
{ { WAVE_FORMAT_EXTENSIBLE, 1, 48000, 96000, 2, 16, 22 }, 16,
SPEAKER_FRONT_CENTER, KSDATAFORMAT_SUBTYPE_PCM };

static WAVEFORMATEX wfe_pcm16_2_48000 = 
{ WAVE_FORMAT_PCM, 2, 48000, 192000, 4, 16, 0 };
static WAVEFORMATEXTENSIBLE wfx_pcm16_2_48000 =
{ { WAVE_FORMAT_EXTENSIBLE, 2, 48000, 192000, 4, 16, 22 }, 16,
SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT, KSDATAFORMAT_SUBTYPE_PCM };

static WAVEFORMATEX wfe_pcm16_3_48000 = 
{ WAVE_FORMAT_PCM, 3, 48000, 288000, 6, 16, 0 };
static WAVEFORMATEXTENSIBLE wfx_pcm16_3_48000 =
{ { WAVE_FORMAT_EXTENSIBLE, 3, 48000, 288000, 6, 16, 22 }, 16,
SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | SPEAKER_LOW_FREQUENCY, KSDATAFORMAT_SUBTYPE_PCM };

static WAVEFORMATEX wfe_pcm16_4_48000 = 
{ WAVE_FORMAT_PCM, 4, 48000, 384000, 8, 16, 0 };
static WAVEFORMATEXTENSIBLE wfx_pcm16_4_48000 =
{ { WAVE_FORMAT_EXTENSIBLE, 4, 48000, 384000, 8, 16, 22 }, 16,
SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | SPEAKER_SIDE_LEFT | SPEAKER_SIDE_RIGHT, KSDATAFORMAT_SUBTYPE_PCM };

static WAVEFORMATEX wfe_pcm16_5_48000 = 
{ WAVE_FORMAT_PCM, 5, 48000, 480000, 10, 16, 0 };
static WAVEFORMATEXTENSIBLE wfx_pcm16_5_48000 =
{ { WAVE_FORMAT_EXTENSIBLE, 5, 48000, 480000, 10, 16, 22 }, 16,
SPEAKER_FRONT_LEFT | SPEAKER_FRONT_CENTER | SPEAKER_FRONT_RIGHT | SPEAKER_SIDE_LEFT | SPEAKER_SIDE_RIGHT, KSDATAFORMAT_SUBTYPE_PCM };

static WAVEFORMATEX wfe_pcm16_6_48000 = 
{ WAVE_FORMAT_PCM, 6, 48000, 576000, 12, 16, 0 };
static WAVEFORMATEXTENSIBLE wfx_pcm16_6_48000 =
{ { WAVE_FORMAT_EXTENSIBLE, 6, 48000, 576000, 12, 16, 22 }, 16,
SPEAKER_FRONT_LEFT | SPEAKER_FRONT_CENTER | SPEAKER_FRONT_RIGHT | SPEAKER_SIDE_LEFT | SPEAKER_SIDE_RIGHT | SPEAKER_LOW_FREQUENCY, KSDATAFORMAT_SUBTYPE_PCM };

static WAVEFORMATEX wfe_pcm16_7_48000 = 
{ WAVE_FORMAT_PCM, 7, 48000, 672000, 14, 16, 0 };
static WAVEFORMATEXTENSIBLE wfx_pcm16_7_48000 =
{ { WAVE_FORMAT_EXTENSIBLE, 7, 48000, 672000, 14, 16, 22 }, 16,
SPEAKER_FRONT_LEFT | SPEAKER_FRONT_CENTER | SPEAKER_FRONT_RIGHT | SPEAKER_SIDE_LEFT | SPEAKER_SIDE_RIGHT | SPEAKER_BACK_CENTER | SPEAKER_LOW_FREQUENCY, KSDATAFORMAT_SUBTYPE_PCM };

static WAVEFORMATEX wfe_pcm16_8_48000 = 
{ WAVE_FORMAT_PCM, 8, 48000, 768000, 16, 16, 0 };
static WAVEFORMATEXTENSIBLE wfx_pcm16_8_48000 =
{ { WAVE_FORMAT_EXTENSIBLE, 8, 48000, 768000, 16, 16, 22 }, 16,
SPEAKER_FRONT_LEFT | SPEAKER_FRONT_CENTER | SPEAKER_FRONT_RIGHT | SPEAKER_SIDE_LEFT | SPEAKER_SIDE_RIGHT | SPEAKER_BACK_LEFT | SPEAKER_BACK_RIGHT | SPEAKER_LOW_FREQUENCY, KSDATAFORMAT_SUBTYPE_PCM };

// PCM sample format series

static WAVEFORMATEX wfe_pcm24_2_48000 =
{ WAVE_FORMAT_PCM, 2, 48000, 288000, 6, 24, 0 };
static WAVEFORMATEXTENSIBLE wfx_pcm24_2_48000 =
{ { WAVE_FORMAT_EXTENSIBLE, 2, 48000, 288000, 6, 24, 22 }, 24,
SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT, KSDATAFORMAT_SUBTYPE_PCM };

static WAVEFORMATEX wfe_pcm32_2_48000 =
{ WAVE_FORMAT_PCM, 2, 48000, 384000, 8, 32, 0 };
static WAVEFORMATEXTENSIBLE wfx_pcm32_2_48000 =
{ { WAVE_FORMAT_EXTENSIBLE, 2, 48000, 384000, 8, 32, 22 }, 32,
SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT, KSDATAFORMAT_SUBTYPE_PCM };

static WAVEFORMATEX wfe_pcmfloat_2_48000 =
{ WAVE_FORMAT_IEEE_FLOAT, 2, 48000, 384000, 8, 32, 0 };
static WAVEFORMATEXTENSIBLE wfx_pcmfloat_2_48000 =
{ { WAVE_FORMAT_EXTENSIBLE, 2, 48000, 384000, 8, 32, 22 }, 32,
SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT, GUID_SUBTYPE_IEEE_FLOAT };

static WAVEFORMATEX wfe_pcmdouble_2_48000 =
{ WAVE_FORMAT_IEEE_FLOAT, 2, 48000, 768000, 16, 64, 0 };
static WAVEFORMATEXTENSIBLE wfx_pcmdouble_2_48000 =
{ { WAVE_FORMAT_EXTENSIBLE, 2, 48000, 768000, 16, 64, 22 }, 64,
SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT, GUID_SUBTYPE_IEEE_FLOAT };

// PCM sample rate series

static WAVEFORMATEX wfe_pcm16_2_44100 = 
{ WAVE_FORMAT_PCM, 2, 44100, 176400, 4, 16, 0 };
static WAVEFORMATEXTENSIBLE wfx_pcm16_2_44100 =
{ { WAVE_FORMAT_EXTENSIBLE, 2, 44100, 176400, 4, 16, 22 }, 16,
SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT, KSDATAFORMAT_SUBTYPE_PCM };

static WAVEFORMATEX wfe_pcm16_2_32000 = 
{ WAVE_FORMAT_PCM, 2, 32000, 128000, 4, 16, 0 };
static WAVEFORMATEXTENSIBLE wfx_pcm16_2_32000 =
{ { WAVE_FORMAT_EXTENSIBLE, 2, 32000, 128000, 4, 16, 22 }, 16,
SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT, KSDATAFORMAT_SUBTYPE_PCM };

// SPDIF

static WAVEFORMATEX wfe_spdif_48000 = 
{ WAVE_FORMAT_DOLBY_AC3_SPDIF, 2, 48000, 192000, 4, 16, 0 };
static WAVEFORMATEXTENSIBLE wfx_spdif_6_48000 =
{ { WAVE_FORMAT_EXTENSIBLE, 2, 48000, 192000, 4, 16, 22 }, 16,
SPEAKER_FRONT_LEFT | SPEAKER_FRONT_CENTER | SPEAKER_FRONT_RIGHT | SPEAKER_SIDE_LEFT | SPEAKER_SIDE_RIGHT | SPEAKER_LOW_FREQUENCY, GUID_DOLBY_AC3_SPDIF };

// Compressed formats

static WAVEFORMATEX wfe_ac3_48000 = 
{ WAVE_FORMAT_AVI_AC3, 0, 48000, 0, 0, 0, 0 };

static WAVEFORMATEX wfe_dts_48000 = 
{ WAVE_FORMAT_AVI_DTS, 0, 48000, 0, 0, 0, 0 };

static WAVEFORMATEX wfe_mpa_48000 = 
{ WAVE_FORMAT_MPEG, 0, 48000, 0, 0, 0, 0 };

static WAVEFORMATEX wfe_mp3_48000 = 
{ WAVE_FORMAT_MPEGLAYER3, 0, 48000, 0, 0, 0, 0 };

static struct {
  WAVEFORMATEX wf;
  uint8_t format_data[2];
}
wfe_aac_48000 = 
{
  { WAVE_FORMAT_AVI_AAC, 0, 48000, 0, 0, 0, 2 },
  { 0x11, 0xb0 }
};

static struct {
  WAVEFORMATEX wf;
  uint8_t format_data[113];
}
wfe_flac_48000 = 
{ 
  { WAVE_FORMAT_FLAC, 0, 48000, 0, 0, 0, 113 },
  {
              0x66, 0x4c, 0x61, 0x43, 0x00, 0x00, 0x00, 0x22, 0x10, 0x00, 0x10, 0x00, 0x00, 0x00,
  0x12, 0x00, 0x56, 0xc8, 0x0b, 0xb8, 0x03, 0x70, 0x04, 0x0a, 0x3d, 0x00, 0xd4, 0xc8, 0xb0, 0xa1,
  0x25, 0x51, 0x35, 0x1e, 0x1f, 0xc9, 0xf6, 0xbe, 0x19, 0x8c, 0xf0, 0xa3, 0x84, 0x00, 0x00, 0x43,
  0x20, 0x00, 0x00, 0x00, 0x72, 0x65, 0x66, 0x65, 0x72, 0x65, 0x6e, 0x63, 0x65, 0x20, 0x6c, 0x69,
  0x62, 0x46, 0x4c, 0x41, 0x43, 0x20, 0x31, 0x2e, 0x32, 0x2e, 0x31, 0x20, 0x32, 0x30, 0x30, 0x37,
  0x30, 0x39, 0x31, 0x37, 0x02, 0x00, 0x00, 0x00, 0x0d, 0x00, 0x00, 0x00, 0x56, 0x41, 0x4c, 0x49,
  0x44, 0x5f, 0x42, 0x49, 0x54, 0x53, 0x3d, 0x32, 0x34, 0x06, 0x00, 0x00, 0x00, 0x48, 0x44, 0x43,
  0x44, 0x3d, 0x30,
  }
};


///////////////////////////////////////////////////////////////////////////////

static struct {
  Speakers spk;
  int i;
  const WAVEFORMATEX *wfe;
} spk2wfe_tbl[] =
{
  // Number of channels
  { Speakers(FORMAT_PCM16, MODE_MONO, 48000), 0, (WAVEFORMATEX *)&wfe_pcm16_1_48000 },
  { Speakers(FORMAT_PCM16, MODE_MONO, 48000), 1, 0 },
  { Speakers(FORMAT_PCM16, MODE_STEREO, 48000), 0, (WAVEFORMATEX *)&wfe_pcm16_2_48000 },
  { Speakers(FORMAT_PCM16, MODE_STEREO, 48000), 1, 0 },
  { Speakers(FORMAT_PCM16, MODE_2_0_LFE, 48000), 0, (WAVEFORMATEX *)&wfx_pcm16_3_48000 },
  { Speakers(FORMAT_PCM16, MODE_2_0_LFE, 48000), 1, (WAVEFORMATEX *)&wfe_pcm16_3_48000 },
  { Speakers(FORMAT_PCM16, MODE_2_0_LFE, 48000), 2, 0 },
  { Speakers(FORMAT_PCM16, MODE_QUADRO, 48000), 0, (WAVEFORMATEX *)&wfx_pcm16_4_48000 },
  { Speakers(FORMAT_PCM16, MODE_QUADRO, 48000), 1, (WAVEFORMATEX *)&wfe_pcm16_4_48000 },
  { Speakers(FORMAT_PCM16, MODE_QUADRO, 48000), 2, 0 },
  { Speakers(FORMAT_PCM16, MODE_3_2, 48000), 0, (WAVEFORMATEX *)&wfx_pcm16_5_48000 },
  { Speakers(FORMAT_PCM16, MODE_3_2, 48000), 1, (WAVEFORMATEX *)&wfe_pcm16_5_48000 },
  { Speakers(FORMAT_PCM16, MODE_3_2, 48000), 2, 0 },
  { Speakers(FORMAT_PCM16, MODE_5_1, 48000), 0, (WAVEFORMATEX *)&wfx_pcm16_6_48000 },
  { Speakers(FORMAT_PCM16, MODE_5_1, 48000), 1, (WAVEFORMATEX *)&wfe_pcm16_6_48000 },
  { Speakers(FORMAT_PCM16, MODE_5_1, 48000), 2, 0 },
  { Speakers(FORMAT_PCM16, MODE_6_1, 48000), 0, (WAVEFORMATEX *)&wfx_pcm16_7_48000 },
  { Speakers(FORMAT_PCM16, MODE_6_1, 48000), 1, (WAVEFORMATEX *)&wfe_pcm16_7_48000 },
  { Speakers(FORMAT_PCM16, MODE_6_1, 48000), 2, 0 },
  { Speakers(FORMAT_PCM16, MODE_7_1, 48000), 0, (WAVEFORMATEX *)&wfx_pcm16_8_48000 },
  { Speakers(FORMAT_PCM16, MODE_7_1, 48000), 1, (WAVEFORMATEX *)&wfe_pcm16_8_48000 },
  { Speakers(FORMAT_PCM16, MODE_7_1, 48000), 2, 0 },
  // Sample format
  { Speakers(FORMAT_PCM24, MODE_STEREO, 48000), 0, (WAVEFORMATEX *)&wfx_pcm24_2_48000 },
  { Speakers(FORMAT_PCM24, MODE_STEREO, 48000), 1, (WAVEFORMATEX *)&wfe_pcm24_2_48000 },
  { Speakers(FORMAT_PCM24, MODE_STEREO, 48000), 2, 0 },
  { Speakers(FORMAT_PCM32, MODE_STEREO, 48000), 0, (WAVEFORMATEX *)&wfx_pcm32_2_48000 },
  { Speakers(FORMAT_PCM32, MODE_STEREO, 48000), 1, (WAVEFORMATEX *)&wfe_pcm32_2_48000 },
  { Speakers(FORMAT_PCM32, MODE_STEREO, 48000), 2, 0 },
  { Speakers(FORMAT_PCMFLOAT, MODE_STEREO, 48000), 0, (WAVEFORMATEX *)&wfx_pcmfloat_2_48000 },
  { Speakers(FORMAT_PCMFLOAT, MODE_STEREO, 48000), 1, (WAVEFORMATEX *)&wfe_pcmfloat_2_48000 },
  { Speakers(FORMAT_PCMFLOAT, MODE_STEREO, 48000), 2, 0 },
  { Speakers(FORMAT_PCMDOUBLE, MODE_STEREO, 48000), 0, (WAVEFORMATEX *)&wfx_pcmdouble_2_48000 },
  { Speakers(FORMAT_PCMDOUBLE, MODE_STEREO, 48000), 1, (WAVEFORMATEX *)&wfe_pcmdouble_2_48000 },
  { Speakers(FORMAT_PCMDOUBLE, MODE_STEREO, 48000), 2, 0 },
  // Sample rate
  { Speakers(FORMAT_PCM16, MODE_STEREO, 44100), 0, (WAVEFORMATEX *)&wfe_pcm16_2_44100 },
  { Speakers(FORMAT_PCM16, MODE_STEREO, 44100), 1, 0 },
  { Speakers(FORMAT_PCM16, MODE_STEREO, 32000), 0, (WAVEFORMATEX *)&wfe_pcm16_2_32000 },
  { Speakers(FORMAT_PCM16, MODE_STEREO, 32000), 1, 0 },
  // SPDIF
  { Speakers(FORMAT_SPDIF, MODE_5_1, 48000), 0, (WAVEFORMATEX *)&wfx_spdif_6_48000 },
  { Speakers(FORMAT_SPDIF, MODE_5_1, 48000), 1, (WAVEFORMATEX *)&wfe_spdif_48000 },
  { Speakers(FORMAT_SPDIF, MODE_5_1, 48000), 2, 0 },
};

static struct
{
  WAVEFORMAT *wf;
  size_t size;     // original structure size
  size_t bad_size; // bad (small) structure size
  Speakers spk;
}
wf2spk_tbl[] =
{
  // Number of channels
  { (WAVEFORMAT *)&wfe_pcm16_1_48000, pcmwf_size, pcmwf_size-1, Speakers(FORMAT_PCM16, MODE_MONO, 48000) },
  { (WAVEFORMAT *)&wfe_pcm16_1_48000, wfe_size,   pcmwf_size-1, Speakers(FORMAT_PCM16, MODE_MONO, 48000) },
  { (WAVEFORMAT *)&wfx_pcm16_1_48000, wfx_size,   wfx_size-1,   Speakers(FORMAT_PCM16, MODE_MONO, 48000) },
  { (WAVEFORMAT *)&wfe_pcm16_2_48000, pcmwf_size, pcmwf_size-1, Speakers(FORMAT_PCM16, MODE_STEREO, 48000) },
  { (WAVEFORMAT *)&wfe_pcm16_2_48000, wfe_size,   pcmwf_size-1, Speakers(FORMAT_PCM16, MODE_STEREO, 48000) },
  { (WAVEFORMAT *)&wfx_pcm16_2_48000, wfx_size,   wfx_size-1,   Speakers(FORMAT_PCM16, MODE_STEREO, 48000) },
  { (WAVEFORMAT *)&wfe_pcm16_3_48000, pcmwf_size, pcmwf_size-1, Speakers(FORMAT_PCM16, MODE_2_0_LFE, 48000) },
  { (WAVEFORMAT *)&wfe_pcm16_3_48000, wfe_size,   pcmwf_size-1, Speakers(FORMAT_PCM16, MODE_2_0_LFE, 48000) },
  { (WAVEFORMAT *)&wfx_pcm16_3_48000, wfx_size,   wfx_size-1,   Speakers(FORMAT_PCM16, MODE_2_0_LFE, 48000) },
  { (WAVEFORMAT *)&wfe_pcm16_4_48000, pcmwf_size, pcmwf_size-1, Speakers(FORMAT_PCM16, MODE_QUADRO, 48000) },
  { (WAVEFORMAT *)&wfe_pcm16_4_48000, wfe_size,   pcmwf_size-1, Speakers(FORMAT_PCM16, MODE_QUADRO, 48000) },
  { (WAVEFORMAT *)&wfx_pcm16_4_48000, wfx_size,   wfx_size-1,   Speakers(FORMAT_PCM16, MODE_QUADRO, 48000) },
  { (WAVEFORMAT *)&wfe_pcm16_5_48000, pcmwf_size, pcmwf_size-1, Speakers(FORMAT_PCM16, MODE_3_2, 48000) },
  { (WAVEFORMAT *)&wfe_pcm16_5_48000, wfe_size,   pcmwf_size-1, Speakers(FORMAT_PCM16, MODE_3_2, 48000) },
  { (WAVEFORMAT *)&wfx_pcm16_5_48000, wfx_size,   wfx_size-1,   Speakers(FORMAT_PCM16, MODE_3_2, 48000) },
  { (WAVEFORMAT *)&wfe_pcm16_6_48000, pcmwf_size, pcmwf_size-1, Speakers(FORMAT_PCM16, MODE_5_1, 48000) },
  { (WAVEFORMAT *)&wfe_pcm16_6_48000, wfe_size,   pcmwf_size-1, Speakers(FORMAT_PCM16, MODE_5_1, 48000) },
  { (WAVEFORMAT *)&wfx_pcm16_6_48000, wfx_size,   wfx_size-1,   Speakers(FORMAT_PCM16, MODE_5_1, 48000) },
  { (WAVEFORMAT *)&wfe_pcm16_7_48000, pcmwf_size, pcmwf_size-1, Speakers(FORMAT_PCM16, MODE_6_1, 48000) },
  { (WAVEFORMAT *)&wfe_pcm16_7_48000, wfe_size,   pcmwf_size-1, Speakers(FORMAT_PCM16, MODE_6_1, 48000) },
  { (WAVEFORMAT *)&wfx_pcm16_7_48000, wfx_size,   wfx_size-1,   Speakers(FORMAT_PCM16, MODE_6_1, 48000) },
  { (WAVEFORMAT *)&wfe_pcm16_8_48000, pcmwf_size, pcmwf_size-1, Speakers(FORMAT_PCM16, MODE_7_1, 48000) },
  { (WAVEFORMAT *)&wfe_pcm16_8_48000, wfe_size,   pcmwf_size-1, Speakers(FORMAT_PCM16, MODE_7_1, 48000) },
  { (WAVEFORMAT *)&wfx_pcm16_8_48000, wfx_size,   wfx_size-1,   Speakers(FORMAT_PCM16, MODE_7_1, 48000) },
  // Sample format
  { (WAVEFORMAT *)&wfe_pcm24_2_48000, pcmwf_size, pcmwf_size-1, Speakers(FORMAT_PCM24, MODE_STEREO, 48000) },
  { (WAVEFORMAT *)&wfe_pcm24_2_48000, wfe_size,   pcmwf_size-1, Speakers(FORMAT_PCM24, MODE_STEREO, 48000) },
  { (WAVEFORMAT *)&wfx_pcm24_2_48000, wfx_size,   wfx_size-1,   Speakers(FORMAT_PCM24, MODE_STEREO, 48000) },
  { (WAVEFORMAT *)&wfe_pcm32_2_48000, pcmwf_size, pcmwf_size-1, Speakers(FORMAT_PCM32, MODE_STEREO, 48000) },
  { (WAVEFORMAT *)&wfe_pcm32_2_48000, wfe_size,   pcmwf_size-1, Speakers(FORMAT_PCM32, MODE_STEREO, 48000) },
  { (WAVEFORMAT *)&wfx_pcm32_2_48000, wfx_size,   wfx_size-1,   Speakers(FORMAT_PCM32, MODE_STEREO, 48000) },
  { (WAVEFORMAT *)&wfe_pcmfloat_2_48000, pcmwf_size, pcmwf_size-1, Speakers(FORMAT_PCMFLOAT, MODE_STEREO, 48000) },
  { (WAVEFORMAT *)&wfe_pcmfloat_2_48000, wfe_size,   pcmwf_size-1, Speakers(FORMAT_PCMFLOAT, MODE_STEREO, 48000) },
  { (WAVEFORMAT *)&wfx_pcmfloat_2_48000, wfx_size,   wfx_size-1,   Speakers(FORMAT_PCMFLOAT, MODE_STEREO, 48000) },
  { (WAVEFORMAT *)&wfe_pcmdouble_2_48000, pcmwf_size, pcmwf_size-1, Speakers(FORMAT_PCMDOUBLE, MODE_STEREO, 48000) },
  { (WAVEFORMAT *)&wfe_pcmdouble_2_48000, wfe_size,   pcmwf_size-1, Speakers(FORMAT_PCMDOUBLE, MODE_STEREO, 48000) },
  { (WAVEFORMAT *)&wfx_pcmdouble_2_48000, wfx_size,   wfx_size-1,   Speakers(FORMAT_PCMDOUBLE, MODE_STEREO, 48000) },
  // Sample rate
  { (WAVEFORMAT *)&wfe_pcm16_2_44100, pcmwf_size, pcmwf_size-1, Speakers(FORMAT_PCM16, MODE_STEREO, 44100) },
  { (WAVEFORMAT *)&wfe_pcm16_2_44100, wfe_size,   pcmwf_size-1, Speakers(FORMAT_PCM16, MODE_STEREO, 44100) },
  { (WAVEFORMAT *)&wfx_pcm16_2_44100, wfx_size,   wfx_size-1,   Speakers(FORMAT_PCM16, MODE_STEREO, 44100) },
  { (WAVEFORMAT *)&wfe_pcm16_2_32000, pcmwf_size, pcmwf_size-1, Speakers(FORMAT_PCM16, MODE_STEREO, 32000) },
  { (WAVEFORMAT *)&wfe_pcm16_2_32000, wfe_size,   pcmwf_size-1, Speakers(FORMAT_PCM16, MODE_STEREO, 32000) },
  { (WAVEFORMAT *)&wfx_pcm16_2_32000, wfx_size,   wfx_size-1,   Speakers(FORMAT_PCM16, MODE_STEREO, 32000) },
  // SPDIF
  { (WAVEFORMAT *)&wfe_spdif_48000, wf_size,    wf_size-1,    Speakers(FORMAT_SPDIF, 0, 48000) },
  { (WAVEFORMAT *)&wfe_spdif_48000, wfe_size,   wf_size-1,    Speakers(FORMAT_SPDIF, 0, 48000) },
  { (WAVEFORMAT *)&wfx_spdif_6_48000, wfx_size, wfx_size-1,   Speakers(FORMAT_SPDIF, MODE_5_1, 48000) },
  // Compressed formats
  { (WAVEFORMAT *)&wfe_ac3_48000, wf_size, wf_size-1, Speakers(FORMAT_AC3_EAC3, 0, 48000) },
  { (WAVEFORMAT *)&wfe_dts_48000, wf_size, wf_size-1, Speakers(FORMAT_DTS, 0, 48000) },
  { (WAVEFORMAT *)&wfe_mpa_48000, wf_size, wf_size-1, Speakers(FORMAT_MPA, 0, 48000) },
  { (WAVEFORMAT *)&wfe_mp3_48000, wf_size, wf_size-1, Speakers(FORMAT_MPA, 0, 48000) },
  { (WAVEFORMAT *)&wfe_aac_48000, wf_size, wf_size-1, Speakers(FORMAT_AAC_FRAME, 0, 48000) },
  { (WAVEFORMAT *)&wfe_flac_48000, wf_size, wf_size-1, Speakers(FORMAT_FLAC, 0, 48000) },
};

///////////////////////////////////////////////////////////////////////////////

BOOST_AUTO_TEST_SUITE(winspk)

BOOST_AUTO_TEST_CASE(test_spk2wfe)
{
  for (int i = 0; i < array_size(spk2wfe_tbl); i++)
  {
    std::auto_ptr<WAVEFORMATEX> wfe(spk2wfe(spk2wfe_tbl[i].spk, spk2wfe_tbl[i].i));
    BOOST_CHECK_EQUAL(wfe.get() == 0, spk2wfe_tbl[i].wfe == 0);
    if (wfe.get() && spk2wfe_tbl[i].wfe)
    {
      BOOST_CHECK_EQUAL(wfe->cbSize, spk2wfe_tbl[i].wfe->cbSize);
      if (wfe->cbSize == spk2wfe_tbl[i].wfe->cbSize)
        BOOST_CHECK(memcmp(wfe.get(), spk2wfe_tbl[i].wfe, sizeof(WAVEFORMATEX) + wfe->cbSize) == 0);
    }
  }
}

BOOST_AUTO_TEST_CASE(test_wf2spk)
{
  for (int i = 0; i < array_size(wf2spk_tbl); i++)
  {
    Speakers spk = wf2spk(wf2spk_tbl[i].wf, wf2spk_tbl[i].size);
    BOOST_CHECK_EQUAL(spk, wf2spk_tbl[i].spk);
    if (spk.data_size > 0)
    {
      WAVEFORMATEX *wfe = wf_cast<WAVEFORMATEX>(wf2spk_tbl[i].wf, wf2spk_tbl[i].size);
      if (wfe)
      {
        if (spk.data_size != wfe->cbSize)
          BOOST_ERROR("Format data size differs");
        else
          BOOST_CHECK(memcmp(spk.format_data.get(), (wfe+1), spk.data_size) == 0);
      }
      else
        BOOST_ERROR("Cannot cast to WAVEFORMATEX");
    }
    Speakers bad_spk = wf2spk(wf2spk_tbl[i].wf, wf2spk_tbl[i].bad_size);
    BOOST_CHECK(bad_spk.is_unknown());
  }
}

BOOST_AUTO_TEST_SUITE_END()
