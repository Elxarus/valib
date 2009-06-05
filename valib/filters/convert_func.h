#ifndef VALIB_CONVERT_FUNC_H
#define VALIB_CONVERT_FUNC_H

#include "../spk.h"

typedef void (*convert_t)(uint8_t *, samples_t, size_t);
convert_t find_pcm2linear(int pcm_format, int nch);
convert_t find_linear2pcm(int pcm_format, int nch);

#endif
