/*
  Windows-related speakers utilities
*/

#ifndef WINSPK_H
#define WINSPK_H
        
#include <windows.h>
#include "spk.h"

bool wfx2spk(WAVEFORMATEX *wfx, Speakers &spk);
bool spk2wfx(Speakers spk, WAVEFORMATEX *wfx, bool use_extensible);

#endif
