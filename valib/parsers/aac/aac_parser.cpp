#include <sstream>
#include "../../bitstream.h"
#include "aac_parser.h"

extern "C"
{
#define __STDC_CONSTANT_MACROS
#include "../../../3rdparty/ffmpeg/include/libavcodec/avcodec.h"
}

AACParser::AACParser(): FfmpegDecoder(CODEC_ID_AAC, FORMAT_AAC_FRAME)
{}

static const char digit[16] =
{ '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f' };

static const int sampling_frequency_tbl[16] =
{
  96000, 88200, 64000, 48000, 44100, 32000,
  24000, 22050, 16000, 12000, 11025, 8000,
  0, 0, 0, 0
};

static const char *channel_configuration_tbl[16] =
{
  "",
  "1 channel: C",
  "2 channels: L R",
  "3 channels: L C R",
  "4 channels: L C R S",
  "5 channels: L C R BL BR",
  "6 channels: L C R BL BR LFE",
  "8 channels: L C R SL SR BL BR LFE",
  "","","","","","","",""
};

string
AACParser::info() const
{
  if (spk.format != FORMAT_AAC_FRAME)
    return string("No sync");

  if (spk.data_size == 0)
    // No AudioSpecificConfig
    return string();

  uint8_t *format_data = spk.format_data.get();
  std::stringstream result;

  result << "Format data:";
  for (size_t i = 0; i < spk.data_size; i++)
    result << ' ' << digit[format_data[i] >> 4] << digit[format_data[i] & 0xf];
  result << nl;

  if (spk.data_size < 2)
    // Bad AudioSpecificConfig
    return result.str();

  ReadBS bs(format_data, 0, spk.data_size * 8);
  int object_type = bs.get(5);
  int sampling_frequency_index = bs.get(4);
  int sampling_frequency = sampling_frequency_tbl[sampling_frequency_index];
  if (sampling_frequency_index == 15)
    if (spk.data_size < 5)
      // Bad AudioSpecificConfig
      return result.str();
    else
      sampling_frequency = bs.get(24);
  int channel_configuration = bs.get(4);

  result << "Audio Specific Config:" << nl;
  result << "  Object type: ";
  switch (object_type)
  {
    case 1:  result << "AAC Main"; break;
    case 2:  result << "AAC LC";   break;
    case 3:  result << "AAC SSR";  break;
    case 4:  result << "AAC LTP";  break;
    default: result << object_type; break;
  }
  result << nl;
  result << "  Sampling frequency: " << sampling_frequency << nl;
  result << "  Channel configuration: " << channel_configuration_tbl[channel_configuration];
  return result.str();
}
