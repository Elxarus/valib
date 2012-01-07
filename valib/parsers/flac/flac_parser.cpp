#include "flac_parser.h"

extern "C"
{
#define __STDC_CONSTANT_MACROS
#include "../../../3rdparty/ffmpeg/include/libavcodec/avcodec.h"
}

FlacParser::FlacParser(): FfmpegDecoder(CODEC_ID_FLAC, FORMAT_FLAC)
{}
