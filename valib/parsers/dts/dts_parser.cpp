#include "dts_parser.h"

extern "C"
{
#define __STDC_CONSTANT_MACROS
#include "../../../3rdparty/ffmpeg/include/libavcodec/avcodec.h"
}

DTSParser::DTSParser(): FfmpegDecoder(CODEC_ID_DTS, FORMAT_DTS)
{}
