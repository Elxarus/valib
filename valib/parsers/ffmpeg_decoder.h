#ifndef VALIB_FFMPEG_DECODER_H
#define VALIB_FFMPEG_DECODER_H

#include "../buffer.h"
#include "../filter.h"

struct AVCodec;
struct AVCodecContext;
enum CodecID;

class FfmpegDecoder : public SimpleFilter
{
protected:
  // This class is not intended to be used directly.
  // Make a descendant class for the specific format.

  FfmpegDecoder(CodecID ffmpeg_codec_id, int format);
  ~FfmpegDecoder();

public:
  /////////////////////////////////////////////////////////
  // SimpleFilter overrides

  bool can_open(Speakers spk) const;
  bool init();
  void uninit();

  void reset();
  bool process(Chunk &in, Chunk &out);

  bool new_stream() const
  { return new_stream_flag; }

  Speakers get_output() const
  { return out_spk; }

protected:
  virtual bool init_context(AVCodecContext *avctx);

  Rawdata buf;      // output buffer
  Speakers out_spk; // output format
  bool new_stream_flag;

  CodecID ffmpeg_codec_id;
  int format;

  AVCodec *avcodec;
  AVCodecContext *avctx;
};

#endif
