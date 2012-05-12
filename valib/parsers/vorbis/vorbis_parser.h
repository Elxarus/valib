#ifndef VALIB_VORBIS_PARSER_H
#define VALIB_VORBIS_PARSER_H

#include "../ffmpeg_decoder.h"

class VorbisParser : public FfmpegDecoder
{
public:
  VorbisParser();

protected:
  virtual bool init_context(AVCodecContext *avctx);  
  Rawdata format_data;
};

#endif
