/*
  Windows-related speakers utilities
*/

#ifndef WINSPK_H
#define WINSPK_H
        
#include <windows.h>
#include "spk.h"

#ifndef WAVE_FORMAT_DOLBY_AC3_SPDIF 
#define WAVE_FORMAT_DOLBY_AC3_SPDIF 0x0092
#endif

#ifndef WAVE_FORMAT_AC3
#define WAVE_FORMAT_AC3 0x2000
#endif

#ifndef WAVE_FORMAT_DTS
#define WAVE_FORMAT_DTS 0x2001
#endif

bool wfx2spk(WAVEFORMATEX *wfx, Speakers &spk);
bool spk2wfx(Speakers spk, WAVEFORMATEX *wfx, bool use_extensible);

#endif
