#ifndef VALIB_FFMPEG_DECODER_H
#define VALIB_FFMPEG_DECODER_H

#include "../buffer.h"
#include "../filter.h"

struct AVCodec;
struct AVCodecContext;
enum AVCodecID;

class FfmpegDecoder : public SimpleFilter
{
protected:
  // This class is not intended to be used directly.
  // Make a descendant class for the specific format.

  FfmpegDecoder(AVCodecID ffmpeg_codec_id, int format);
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

  Rawdata buf;         // output buffer for ffmpeg
  Speakers ffmpeg_spk; // ffmpeg data format
  Speakers out_spk;    // output format
  bool new_stream_flag;

  AVCodecID ffmpeg_codec_id;
  int format;

  AVCodec *avcodec;
  AVCodecContext *avctx;

  SampleBuf samples;
  void (*convert_func)(uint8_t *, samples_t, size_t);
  void convert_to_linear(uint8_t *data, int data_size, samples_t &out_samples, size_t &out_nsamples);
};

#endif
