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
// Any structure with size >= sizeof(WAVEFORMATEX) is considered to be an
// extension of WAVEFORMATEX and conversion ensures that additional data does
// not exceed the total structure size. WAVEFORMATEXTERNSIBLE conversion also
// checks wFormatTag.

template<class T>
T *wf_cast(void *wf, size_t size)
{
  if (wf == 0 || size < sizeof(T)) return 0;

  // consider any structure with size >= sizeof(WAVEFORMATEX) as WAVEFORMATEX
  // and check destination structure size.
  if (sizeof(T) >= sizeof(WAVEFORMATEX))
  {
    WAVEFORMATEX *wfe = (WAVEFORMATEX *)wf;
    if (sizeof(WAVEFORMATEX) + wfe->cbSize > size)
      return 0;
  }
  return (T*)wf;
}

template<>
WAVEFORMATEXTENSIBLE *wf_cast<WAVEFORMATEXTENSIBLE>(void *wf, size_t size);

///////////////////////////////////////////////////////////////////////////////
// Conversion between DirestSound channel mask and Speakers::mask

int ds2mask(int ds_mask);
int mask2ds(int spk_mask);

///////////////////////////////////////////////////////////////////////////////
// Conversion between WAVEFORMAT and Speakers.

Speakers wf2spk(WAVEFORMAT *wave_format, size_t size);
Speakers wf2spk(WAVEFORMATEX *wave_format, size_t size);
Speakers wf2spk(WAVEFORMATEXTENSIBLE *wave_format, size_t size);
WAVEFORMATEX *spk2wfe(Speakers spk, int i);

///////////////////////////////////////////////////////////////////////////////
// Compare 2 WAVEFORMAT structures
// Returns true if structures are equal and false otherwize.
// * Structures with size < WAVEFORMATEX are always compared binary. Structures
//   with different sizes are considered as different.
// * Structures with size >= WAVEFORMATEX are considered as WAVEFORMATEX
//   extensions. Compared binary up to cbSize extra bytes. Structures with
//   different cbSize are considered as different. If wf_cast() to WAVEFORMATEX
//   fails for any structure, structures considered as different. (I.e.
//   comparison of incorrect structures always fails).
// * Comparison if two zero sized structures is considered as comparison of two
//   uninitialized WAVEFORMAT's and thus equal. This case includes null for
//   both or one of pointers.
// * If at least one of the pointers is null, comparison fails and result is
//   false (with exception when both sizes are zero, see previous case).

bool wf_equal(WAVEFORMAT *wf1, size_t size1, WAVEFORMAT *wf2, size_t size2);

///////////////////////////////////////////////////////////////////////////////
// Compare WAVEFORMATEX and Speakers
// Speakers may be converted into WAVEFORMATEX in several ways, and thus we
// cannot compare it directly. Speakers and WAVEFORMAT are *compatible* when
// Speakers may be converted into structure that is equal to WAVEFORMAT given.

bool is_compatible(Speakers spk, WAVEFORMAT *wf, size_t size);

#endif
