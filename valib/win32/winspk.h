/*
  Windows-related speakers utilities
*/

#ifndef VALIB_WINSPK_H
#define VALIB_WINSPK_H

#include "../spk.h"

#ifdef _WIN32
#include <windows.h>
#include <ks.h>
#include <ksmedia.h>
#else

typedef struct
{
  uint32_t Data1;
  uint16_t Data2;
  uint16_t Data3;
  uint8_t  Data4[8];
} GUID;

typedef struct 
{
  uint16_t wFormatTag;
  uint16_t nChannels;
  uint32_t nSamplesPerSec;
  uint32_t nAvgBytesPerSec;
  uint16_t nBlockAlign;
  uint16_t wBitsPerSample;
  uint16_t cbSize;
} WAVEFORMATEX, *PWAVEFORMATEX;

typedef struct {
  WAVEFORMATEX Format;
  union {
    uint16_t wValidBitsPerSample;
    uint16_t wSamplesPerBlock;
    uint16_t wReserved;
  } Samples;
  uint32_t dwChannelMask; 
  GUID     SubFormat;
} WAVEFORMATEXTENSIBLE, *PWAVEFORMATEXTENSIBLE;

#endif

#define WAVE_FORMAT_IEEE_FLOAT 0x0003
#define WAVE_FORMAT_EXTENSIBLE 0xFFFE
#define WAVE_FORMAT_MPEG 0x0050
#define WAVE_FORMAT_MPEGLAYER3 0x0055
#define WAVE_FORMAT_AVI_AC3 0x2000
#define WAVE_FORMAT_AVI_DTS 0x2001
#define WAVE_FORMAT_AVI_AAC 0x00FF
#define WAVE_FORMAT_DOLBY_AC3_SPDIF 0x0092
#define WAVE_FORMAT_FLAC 0xF1AC

///////////////////////////////////////////////////////////////////////////////
// Cast different WAVEFORMAT's with size check.
// WAVEFORMATEX conversion ensures that additional data does not exceed total
// structure size. WAVEFORMATEXTERNSIBLE conversion also checks wFormatTag.

template<class T>
T *wf_cast(WAVEFORMAT *wf, size_t size)
{
  if (wf == 0 || size < sizeof(T)) return 0;
  return (T*)wf;
}

template<>
WAVEFORMATEX *wf_cast<WAVEFORMATEX>(WAVEFORMAT *wf, size_t size);

template<>
WAVEFORMATEXTENSIBLE *wf_cast<WAVEFORMATEXTENSIBLE>(WAVEFORMAT *wf, size_t size);

///////////////////////////////////////////////////////////////////////////////
// Conversion between DirestSound channel mask and Speakers::mask

int ds2mask(int ds_mask);
int mask2ds(int spk_mask);

///////////////////////////////////////////////////////////////////////////////
// Conversion between WAVEFORMAT and Speakers.

Speakers wf2spk(WAVEFORMAT *wave_format, size_t size);
WAVEFORMATEX *spk2wfe(Speakers spk, int i);

bool wfx2spk(WAVEFORMATEX *wfx, Speakers &spk);
bool spk2wfx(Speakers spk, WAVEFORMATEX *wfx, bool use_extensible);
bool is_compatible(Speakers spk, WAVEFORMATEX *wfx);

#endif
