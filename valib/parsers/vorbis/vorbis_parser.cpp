#include "vorbis_parser.h"

extern "C"
{
#define __STDC_CONSTANT_MACROS
#include "../../../3rdparty/ffmpeg/include/libavcodec/avcodec.h"
}

struct VORBISFORMAT  //xiph.org
{
  uint32_t vorbisVersion;
  uint32_t samplesPerSec;
  uint32_t minBitsPerSec;
  uint32_t avgBitsPerSec;
  uint32_t maxBitsPerSec;
  uint8_t numChannels;
};

struct VORBISFORMAT2  //matroska.org
{
  uint32_t channels;
  uint32_t samplesPerSec;
  uint32_t bitsPerSample;
  uint32_t headerSize[3];
};

VorbisParser::VorbisParser(): FfmpegDecoder(CODEC_ID_VORBIS, FORMAT_VORBIS)
{}

bool
VorbisParser::init_context(AVCodecContext *avctx)
{
  // FFMPEG does not accept VORBISFORMAT2. Instead, it accpets three raw
  // vorbis headers with 16bit length before each. Thus we have to reformat
  // the format data.

  // VORBISFORMAT2 is expected
  if (spk.data_size < sizeof(VORBISFORMAT2))
    return false;

  VORBISFORMAT2 *vorbis_format = (VORBISFORMAT2 *)spk.format_data.get();
  size_t headers = vorbis_format->headerSize[0] + vorbis_format->headerSize[1] + vorbis_format->headerSize[2];
  if (spk.data_size < headers + sizeof(VORBISFORMAT2))
    return false;

  format_data.allocate(6 + headers);

  uint8_t *in = spk.format_data.get() + sizeof(VORBISFORMAT2);
  uint8_t *out = format_data.begin();
  for (int i = 0; i < 3; i++)
  {
    *(uint16_t *)out = uint2be16(vorbis_format->headerSize[i]);
    memcpy(out + 2, in, vorbis_format->headerSize[i]);
    in += vorbis_format->headerSize[i];
    out += 2 + vorbis_format->headerSize[i];
  }

  avctx->extradata = format_data.begin();
  avctx->extradata_size = format_data.size();

  // FFMPEG 0.8.9 has a bug:
  // without number of channels set, vorbis decoder does not open.
  avctx->channels = vorbis_format->channels;
  avctx->sample_rate = vorbis_format->samplesPerSec;
  return true;
}
