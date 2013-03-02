#include "eac3_parser.h"

extern "C"
{
#define __STDC_CONSTANT_MACROS
#include "../../../3rdparty/ffmpeg/include/libavcodec/avcodec.h"
}

EAC3Parser::EAC3Parser(): FfmpegDecoder(CODEC_ID_EAC3, FORMAT_EAC3)
{}
