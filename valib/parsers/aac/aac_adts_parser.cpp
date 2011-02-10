#include "aac_adts_header.h"
#include "aac_adts_parser.h"

static const int sample_rates[] =
{
  96000, 88200, 64000, 48000, 44100, 32000,
  24000, 22050, 16000, 12000, 11025, 8000
};

static const int modes[] =
{
  0,
  MODE_MONO,
  MODE_STEREO,
  MODE_3_0,
  MODE_3_1,
  MODE_3_2,
  MODE_3_2_LFE,
  MODE_5_2_LFE
};



ADTSParser::ADTSParser()
{
  reset();
}

ADTSParser::~ADTSParser()
{}

///////////////////////////////////////////////////////////////////////////////
// FrameParser overrides

const HeaderParser *
ADTSParser::header_parser() const
{
  return &adts_header;
}

void 
ADTSParser::reset()
{
  data = 0;
  data_size = 0;
}

bool
ADTSParser::process(uint8_t *frame, size_t size)
{
  int protection_absent;
  int profile;
  int sampling_frequency_index;
  int channel_configuration;
  int frame_length;

  if ((frame[0] == 0xff)         && // sync
     ((frame[1] & 0xf0) == 0xf0) && // sync
     ((frame[1] & 0x06) == 0x00) && // layer
     ((frame[2] & 0x3c) <  0x30) && // sample rate index
     ((((frame[2] & 1) << 2) | (frame[3] >> 6)) != 0)) // channel config
  {
    protection_absent = frame[1] & 1;
    profile = frame[2] >> 6;
    sampling_frequency_index = (frame[2] >> 2) & 0xf;
    channel_configuration = ((frame[2] & 1) << 2) | (frame[3] >> 6);
    frame_length = ((frame[3] & 3) << 11) | (frame[4] << 3) | (frame[5] >> 5);
  }
  else
    return false;

  if (protection_absent)
  {
    data = frame + 7;
    data_size = frame_length - 7;
  }
  else
  {
    data = frame + 9;
    data_size = frame_length - 9;
  }

  uint8_t audio_specific_config[2];
  audio_specific_config[0] = ((profile+1) << 3) | (sampling_frequency_index >> 1);
  audio_specific_config[1] = ((sampling_frequency_index & 1) << 7) | (channel_configuration << 3);

  spk = Speakers(FORMAT_AAC_FRAME, modes[channel_configuration], sample_rates[sampling_frequency_index]);
  spk.set_format_data(audio_specific_config, 2);

  return true;
}

string
ADTSParser::info() const 
{
  return string();
}
