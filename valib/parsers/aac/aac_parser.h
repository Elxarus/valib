#ifndef AAC_PARSER_H
#define AAC_PARSER_H

#include "../ffmpeg_decoder.h"

class AACParser : public FfmpegDecoder
{
public:
  AACParser();
  virtual string info() const;
};

#endif
