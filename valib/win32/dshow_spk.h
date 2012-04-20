#ifndef VALIB_DSHOW_SPK_H
#define VALIB_DSHOW_SPK_H

#include <streams.h>
#include "../spk.h"

bool mt2spk(CMediaType mt, Speakers &spk);
bool spk2mt(Speakers spk, CMediaType &mt, int i);

#endif
