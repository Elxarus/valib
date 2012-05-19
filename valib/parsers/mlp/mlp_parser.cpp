#include "mlp_parser.h"

extern "C"
{
#define __STDC_CONSTANT_MACROS
#include "../../../3rdparty/ffmpeg/include/libavcodec/avcodec.h"
}

MlpParser::MlpParser(): FfmpegDecoder(CODEC_ID_MLP, FORMAT_MLP)
{}

TruehdParser::TruehdParser(): FfmpegDecoder(CODEC_ID_TRUEHD, FORMAT_TRUEHD)
{}
