#ifndef VALIB_MLP_PARSER_H
#define VALIB_MLP_PARSER_H

#include "../ffmpeg_decoder.h"

class MlpParser : public FfmpegDecoder
{
public:
  MlpParser();
};

class TruehdParser : public FfmpegDecoder
{
public:
  TruehdParser();
};

#endif
