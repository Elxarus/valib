#include <sstream>
#include "ffmpeg_decoder.h"
#include "../log.h"

extern "C"
{
#define __STDC_CONSTANT_MACROS
#include "../../3rdparty/ffmpeg/include/libavcodec/avcodec.h"
}

class FFMPEG
{
public:
  FFMPEG()
  {
    avcodec_init();
    avcodec_register_all();
    av_log_set_callback(avlog);
  }

  static void avlog(void *, int level, const char *fmt, va_list args)
  {
    static string log_module = "ffmpeg";
    valib_vlog(log_event, log_module, fmt, args);
  }
};

volatile static const FFMPEG ffmpeg;

static const Speakers get_format(AVCodecContext *avctx)
{
  int format;
  switch (avctx->sample_fmt)
  {
    case SAMPLE_FMT_S16: format = FORMAT_PCM16;     break;
    case SAMPLE_FMT_S32: format = FORMAT_PCM32;     break;
    case SAMPLE_FMT_FLT: format = FORMAT_PCMFLOAT;  break;
    case SAMPLE_FMT_DBL: format = FORMAT_PCMDOUBLE; break;
    default: return Speakers();
  }
  int mask = 0;
  if (avctx->channel_layout == 0)
  {
    if      (avctx->channels == 1) mask = MODE_MONO;
    else if (avctx->channels == 2) mask = MODE_STEREO;
    else if (avctx->channels == 3) mask = MODE_2_1;
    else if (avctx->channels == 4) mask = MODE_QUADRO;
    else if (avctx->channels == 5) mask = MODE_3_2;
    else if (avctx->channels == 6) mask = MODE_5_1;
    else if (avctx->channels == 7) mask = MODE_6_1;
    else if (avctx->channels == 8) mask = MODE_7_1;
    else return Speakers();
  }
  else
  {
    if (avctx->channel_layout & CH_FRONT_LEFT) mask |= CH_MASK_L;
    if (avctx->channel_layout & CH_FRONT_RIGHT) mask |= CH_MASK_R;
    if (avctx->channel_layout & CH_FRONT_CENTER) mask |= CH_MASK_C;
    if (avctx->channel_layout & CH_LOW_FREQUENCY) mask |= CH_MASK_LFE;
    if (avctx->channel_layout & CH_BACK_LEFT) mask |= CH_MASK_BL;
    if (avctx->channel_layout & CH_BACK_RIGHT) mask |= CH_MASK_BR;
    if (avctx->channel_layout & CH_FRONT_LEFT_OF_CENTER) mask |= CH_MASK_CL;
    if (avctx->channel_layout & CH_FRONT_RIGHT_OF_CENTER) mask |= CH_MASK_CR;
    if (avctx->channel_layout & CH_BACK_CENTER) mask |= CH_MASK_BC;
    if (avctx->channel_layout & CH_SIDE_LEFT) mask |= CH_MASK_SL;
    if (avctx->channel_layout & CH_SIDE_RIGHT) mask |= CH_MASK_SR;
  }
  return Speakers(format, mask, avctx->sample_rate);
}


FfmpegDecoder::FfmpegDecoder(CodecID ffmpeg_codec_id_, int format_)
{
  buf.allocate(AVCODEC_MAX_AUDIO_FRAME_SIZE);
  ffmpeg_codec_id = ffmpeg_codec_id_;
  format  = format_;
  avcodec = avcodec_find_decoder(ffmpeg_codec_id);
  avctx   = 0;
}

FfmpegDecoder::~FfmpegDecoder()
{
  uninit();
}

bool
FfmpegDecoder::can_open(Speakers spk) const
{
  return avcodec && spk.format == format;
}


bool
FfmpegDecoder::init_context(AVCodecContext *avctx)
{
  if (spk.data_size)
  {
    avctx->extradata = spk.format_data.get();
    avctx->extradata_size = (int)spk.data_size;
  }
  return true;
}

bool
FfmpegDecoder::init()
{
  if (avctx) av_free(avctx);
  avctx = avcodec_alloc_context();
  if (!avctx) return false;

  if (!init_context(avctx) ||
      avcodec_open(avctx, avcodec) < 0)
  {
    av_free(avctx);
    avctx = 0;
    return false;
  }

  return true;
}

void
FfmpegDecoder::uninit()
{
  if (avctx)
    av_free(avctx);
  avctx = 0;
}

void
FfmpegDecoder::reset()
{
  out_spk = Speakers();
  new_stream_flag = false;
  avcodec_flush_buffers(avctx);
}

bool
FfmpegDecoder::process(Chunk &in, Chunk &out)
{
  bool sync = in.sync;
  vtime_t time = in.time;
  in.set_sync(false, 0);

  AVPacket avpkt;
  while (in.size)
  {
    av_init_packet(&avpkt);
    avpkt.data = in.rawdata;
    avpkt.size = (int)in.size;
    int out_size = (int)buf.size();
    int gone = avcodec_decode_audio3(avctx, (int16_t *)buf.begin(), &out_size, &avpkt);
    if (gone < 0 || gone == 0)
      return false;

    in.drop_rawdata(gone);
    if (!out_size)
      continue;

    Speakers new_spk = get_format(avctx);
    if (new_spk != out_spk)
    {
      out_spk = new_spk;
      new_stream_flag = true;
    }
    else
      new_stream_flag = false;

    out.set_rawdata(buf.begin(), out_size, sync, time);
    return true;
  }
  return false;
}
