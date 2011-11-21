#include <sstream>
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

static const char *profile_str[] =
{ "AAC Main", "AAC LC", "AAC SSR", "AAC LTP" };



ADTSParser::ADTSParser()
{
  reset();
}

///////////////////////////////////////////////////////////////////////////////
// SimpleFilter overrides

bool
ADTSParser::can_open(Speakers spk) const
{
  return spk.format == FORMAT_AAC_ADTS;
}

bool
ADTSParser::init()
{
  reset();
  return true;
}

void 
ADTSParser::reset()
{
  out_spk = spk_unknown;
  new_stream_flag = false;
  frame_parser.reset();
  frame_length = 0;
}

bool
ADTSParser::process(Chunk &in, Chunk &out)
{
  bool     sync  = in.sync;
  vtime_t  time  = in.time;
  uint8_t *frame = in.rawdata;
  size_t   size  = in.size;
  in.clear();

  if (size < frame_parser.header_size())
    return false;

  /////////////////////////////////////////////////////////
  // Parse ADTS header

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
    if (size < frame_length)
    {
      // resync
      reset();
      return false;
    }
  }
  else
    return false;

  /////////////////////////////////////////////////////////
  // Format change

  if (frame_parser.in_sync())
  {
    if (frame_parser.next_frame(frame, frame_length))
      new_stream_flag = false;
    else
      reset();
  }

  if (!frame_parser.in_sync())
  {
    if (frame_parser.first_frame(frame, frame_length))
    {
      new_stream_flag = true;

      uint8_t audio_specific_config[2];
      audio_specific_config[0] = ((profile+1) << 3) | (sampling_frequency_index >> 1);
      audio_specific_config[1] = ((sampling_frequency_index & 1) << 7) | (channel_configuration << 3);

      out_spk = Speakers(FORMAT_AAC_FRAME, modes[channel_configuration], sample_rates[sampling_frequency_index]);
      out_spk.set_format_data(audio_specific_config, 2);
    }
    else
      return false;
  }

  /////////////////////////////////////////////////////////
  // Return payload

  if (protection_absent)
    out.set_rawdata(frame+7, frame_length-7, sync, time);
  else
    out.set_rawdata(frame+9, frame_length-9, sync, time);

  return true;
}

string
ADTSParser::info() const 
{
  if (frame_length == 0)
    return string();

  using std::endl;
  std::stringstream result;
  int sample_rate = sample_rates[sampling_frequency_index];
  result << "Format: " << Speakers(FORMAT_AAC_ADTS, modes[channel_configuration], sample_rate).print() << endl;
  result << "Profile: " << profile_str[profile] << endl;
  result << "Frame size: " << frame_length << endl;
  result << "Bitrate: " << int(frame_length * sample_rate * 8 / 1024 / 1000) << "kbps" << endl;
  return result.str();
}
