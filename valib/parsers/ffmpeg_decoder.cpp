#ifdef _MSC_VER
#include <excpt.h>
#endif
#include <sstream>
#include "../log.h"
#include "../filters/convert_func.h"
#include "ffmpeg_decoder.h"

extern "C"
{
#define __STDC_CONSTANT_MACROS
#include "../../3rdparty/ffmpeg/include/libavcodec/avcodec.h"
}

static const string module = "FfmpegDecoder";

static void ffmpeg_log(void *, int level, const char *fmt, va_list args)
{
  // Here we use separate module name to distinguish ffmpeg and FfmpegDecoder messages.
  static string module = "ffmpeg";
  valib_vlog(log_event, module, fmt, args);
}

static bool init_ffmpeg()
{
  static bool ffmpeg_initialized = false;
  static bool ffmpeg_failed = false;

  if (!ffmpeg_initialized && !ffmpeg_failed)
  {
#ifdef _MSC_VER
    // Delayed ffmpeg dll loading support and gracefull error handling.
    // Works for any: static linking, dynamic linking, delayed linking.
    // No need to know actual ffmpeg dll name and no need to patch it here
    // after upgrade.
    const unsigned long CODE_MOD_NOT_FOUND  = 0xC06D007E; // = VcppException(ERROR_SEVERITY_ERROR, ERROR_MOD_NOT_FOUND)
    const unsigned long CODE_PROC_NOT_FOUND = 0xC06D007f; // = VcppException(ERROR_SEVERITY_ERROR, ERROR_PROC_NOT_FOUND)
    __try {
      avcodec_register_all();
      av_log_set_callback(ffmpeg_log);
      ffmpeg_initialized = true;
    }
    __except ((GetExceptionCode() == CODE_MOD_NOT_FOUND ||
               GetExceptionCode() == CODE_PROC_NOT_FOUND)?
               EXCEPTION_EXECUTE_HANDLER:
               EXCEPTION_CONTINUE_SEARCH)
    {
      ffmpeg_failed = true;
      return false;
    }
#else
    avcodec_register_all();
    av_log_set_callback(ffmpeg_log);
    ffmpeg_initialized = true;
#endif
  }
  return !ffmpeg_failed;
}

static bool is_planar(AVSampleFormat sample_format)
{
  switch (sample_format)
  {
    case AV_SAMPLE_FMT_S16P:
    case AV_SAMPLE_FMT_S32P:
    case AV_SAMPLE_FMT_FLTP:
    case AV_SAMPLE_FMT_DBLP:
      return true;
  }
  return false;
}

static const Speakers get_format(AVCodecContext *avctx)
{
  int format = FORMAT_UNKNOWN;
  switch (avctx->sample_fmt)
  {
    case AV_SAMPLE_FMT_S16:  format = FORMAT_PCM16;     break;
    case AV_SAMPLE_FMT_S32:  format = FORMAT_PCM32;     break;
    case AV_SAMPLE_FMT_FLT:  format = FORMAT_PCMFLOAT;  break;
    case AV_SAMPLE_FMT_DBL:  format = FORMAT_PCMDOUBLE; break;

    case AV_SAMPLE_FMT_S16P: format = FORMAT_PCM16;     break;
    case AV_SAMPLE_FMT_S32P: format = FORMAT_PCM32;     break;
    case AV_SAMPLE_FMT_FLTP: format = FORMAT_PCMFLOAT;  break;
    case AV_SAMPLE_FMT_DBLP: format = FORMAT_PCMDOUBLE; break;

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
    if (avctx->channel_layout & AV_CH_FRONT_LEFT) mask |= CH_MASK_L;
    if (avctx->channel_layout & AV_CH_FRONT_RIGHT) mask |= CH_MASK_R;
    if (avctx->channel_layout & AV_CH_FRONT_CENTER) mask |= CH_MASK_C;
    if (avctx->channel_layout & AV_CH_LOW_FREQUENCY) mask |= CH_MASK_LFE;
    if (avctx->channel_layout & AV_CH_BACK_LEFT) mask |= CH_MASK_BL;
    if (avctx->channel_layout & AV_CH_BACK_RIGHT) mask |= CH_MASK_BR;
    if (avctx->channel_layout & AV_CH_FRONT_LEFT_OF_CENTER) mask |= CH_MASK_CL;
    if (avctx->channel_layout & AV_CH_FRONT_RIGHT_OF_CENTER) mask |= CH_MASK_CR;
    if (avctx->channel_layout & AV_CH_BACK_CENTER) mask |= CH_MASK_BC;
    if (avctx->channel_layout & AV_CH_SIDE_LEFT) mask |= CH_MASK_SL;
    if (avctx->channel_layout & AV_CH_SIDE_RIGHT) mask |= CH_MASK_SR;
  }

  return Speakers(format, mask, avctx->sample_rate);
}

FfmpegDecoder::FfmpegDecoder(CodecID ffmpeg_codec_id_, int format_)
{
  ffmpeg_codec_id = ffmpeg_codec_id_;
  format  = format_;
  avcodec = 0;
  avctx   = 0;
}

FfmpegDecoder::~FfmpegDecoder()
{
  uninit();
}

bool
FfmpegDecoder::can_open(Speakers spk) const
{
  return spk.format == format;
}

bool
FfmpegDecoder::init_context(AVCodecContext *avctx)
{
  // Supported ffmpeg formats with priority.
  // Higher the index, higher the priority.
  static const AVSampleFormat sample_fmts[] =
  {
    AV_SAMPLE_FMT_S16,
    AV_SAMPLE_FMT_S16P,
    AV_SAMPLE_FMT_S32,
    AV_SAMPLE_FMT_S32P,

#ifndef FLOAT_SAMPLE
    // When sample_t is double AV_SAMPLE_FMT_DBL(P) is the preferred format
    AV_SAMPLE_FMT_FLT,
    AV_SAMPLE_FMT_FLTP,
    AV_SAMPLE_FMT_DBL,
    AV_SAMPLE_FMT_DBLP,
#else
    // When sample_t is float AV_SAMPLE_FMT_FLT(P) is the preferred format
    AV_SAMPLE_FMT_DBL,
    AV_SAMPLE_FMT_DBLP,
    AV_SAMPLE_FMT_FLT,
    AV_SAMPLE_FMT_FLTP,
#endif
  };

  if (spk.data_size && spk.format_data.get())
  {
    avctx->extradata = spk.format_data.get();
    avctx->extradata_size = (int)spk.data_size;
  }

  if (avcodec->sample_fmts)
  {
    avctx->request_sample_fmt = AV_SAMPLE_FMT_NONE;
    for (int i = 0; i < array_size(sample_fmts); i++)
      for (int j = 0; avcodec->sample_fmts[j] != AV_SAMPLE_FMT_NONE; j++)
        if (sample_fmts[i] == avcodec->sample_fmts[j])
          avctx->request_sample_fmt = avcodec->sample_fmts[j];

    // No supported format found
    if (avctx->request_sample_fmt == AV_SAMPLE_FMT_NONE)
      return false;
  }

  return true;
}

bool
FfmpegDecoder::init()
{
  if (!init_ffmpeg())
  {
    valib_log(log_error, module, "Cannot load ffmpeg library");
    return false;
  }

  if (!avcodec)
  {
    avcodec = avcodec_find_decoder(ffmpeg_codec_id);
    if (!avcodec)
    {
      valib_log(log_error, module, "avcodec_find_decoder(%i) failed", ffmpeg_codec_id);
      return false;
    }
  }

  if (avctx) uninit();
  avctx = avcodec_alloc_context3(avcodec);
  if (!avctx)
  {
    valib_log(log_error, module, "avcodec_alloc_context() failed");
    return false;
  }

  if (!init_context(avctx) ||
      avcodec_open2(avctx, avcodec, 0) < 0)
  {
    uninit();
    return false;
  }

  buf.allocate(AVCODEC_MAX_AUDIO_FRAME_SIZE);
  return true;
}

void
FfmpegDecoder::uninit()
{
  if (avctx)
  {
    avcodec_close(avctx);
    av_free(avctx);
  }
  avctx = 0;
}

void
FfmpegDecoder::reset()
{
  ffmpeg_spk = Speakers();
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
    int data_size = (int)buf.size();
    int gone = avcodec_decode_audio3(avctx, (int16_t *)buf.begin(), &data_size, &avpkt);
    if (gone < 0 || gone == 0)
      return false;

    in.drop_rawdata(gone);
    if (!data_size)
      continue;

    Speakers new_spk = get_format(avctx);
    if (new_spk.is_unknown())
      continue;

    if (new_spk != ffmpeg_spk)
    {
      new_stream_flag = true;
      ffmpeg_spk = new_spk;
      out_spk = new_spk;
      out_spk.format = FORMAT_LINEAR;

      if (is_planar(avctx->sample_fmt))
        convert_func = find_pcm2linear(ffmpeg_spk.format, 1);
      else
        convert_func = find_pcm2linear(ffmpeg_spk.format, ffmpeg_spk.nch());
    }
    else
      new_stream_flag = false;

    samples_t out_samples;
    size_t out_nsamples;
    convert_to_linear(buf.begin(), data_size, out_samples, out_nsamples);
    out.set_linear(out_samples, out_nsamples, sync, time);

    return true;
  }
  return false;
}

void
FfmpegDecoder::convert_to_linear(uint8_t *data, int data_size, samples_t &out_samples, size_t &out_nsamples)
{
  assert(data && data_size > 0);
  assert(convert_func);

  int nch = out_spk.nch();
  size_t sample_size = ffmpeg_spk.sample_size();
  size_t nsamples = (size_t)data_size / nch / sample_size;
  assert(nsamples * nch * sample_size == (size_t)data_size);

  if ((int)samples.nch() < nch || samples.nsamples() < nsamples)
    samples.allocate(nch, nsamples);

  if (is_planar(avctx->sample_fmt))
  {
    samples_t temp_samples;
    size_t plane_size = nsamples * sample_size;
    for (int ch = 0; ch < nch; ch++)
    {
      temp_samples[0] = samples[ch];
      convert_func(data + plane_size * ch, temp_samples, nsamples);
    }
  }
  else
    convert_func(data, samples, nsamples);

  out_samples = samples;
  out_samples.reorder_to_std(out_spk, win_order);
  out_nsamples = nsamples;
}
