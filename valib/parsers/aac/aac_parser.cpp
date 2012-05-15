#include <sstream>
#include "../../bitstream.h"
#include "../../buffer.h"
#include "aac_parser.h"
#include "../../../3rdparty/faad/neaacdec.h"

static const size_t max_frame_size = FAAD_MIN_STREAMSIZE * NCHANNELS;
static const order_t aac_order = { CH_C, CH_L, CH_R, CH_SL, CH_SR, CH_BL, CH_BR, CH_LFE, CH_NONE, CH_NONE, CH_NONE };

///////////////////////////////////////////////////////////////////////////////
// Match output format of aac decoder to the valib's internal sample format

#ifdef FLOAT_SAMPLE
#  define AAC_SAMPLE_FORMAT FAAD_FMT_FLOAT
#else
#  define AAC_SAMPLE_FORMAT FAAD_FMT_DOUBLE
#endif

static Speakers
info2spk(const NeAACDecFrameInfo &info)
{
  static const int order[CH_NAMES] = {
    FRONT_CHANNEL_CENTER, FRONT_CHANNEL_LEFT, FRONT_CHANNEL_RIGHT,
    SIDE_CHANNEL_LEFT, SIDE_CHANNEL_RIGHT, BACK_CHANNEL_LEFT,
    BACK_CHANNEL_RIGHT, BACK_CHANNEL_CENTER,
    LFE_CHANNEL
  };

  if (info.channels > NCHANNELS)
    return Speakers();

  // HACK: bug in faad2 with stereo sources?
  int mask = 0;
  if(info.channels == 2 && info.channel_position[1] == UNKNOWN_CHANNEL)
    mask = CH_MASK_L | CH_MASK_R;

  for(int i = 0; i < info.channels; i++)
    switch (info.channel_position[i])
    {
      case FRONT_CHANNEL_CENTER: mask |= CH_MASK_C;   break;
      case FRONT_CHANNEL_LEFT:   mask |= CH_MASK_L;   break;
      case FRONT_CHANNEL_RIGHT:  mask |= CH_MASK_R;   break;
      case SIDE_CHANNEL_LEFT:    mask |= CH_MASK_SL;  break;
      case SIDE_CHANNEL_RIGHT:   mask |= CH_MASK_SR;  break;
      case BACK_CHANNEL_LEFT:    mask |= CH_MASK_BL;  break;
      case BACK_CHANNEL_RIGHT:   mask |= CH_MASK_BR;  break;
      case BACK_CHANNEL_CENTER:  mask |= CH_MASK_BC;  break;
      case LFE_CHANNEL:          mask |= CH_MASK_LFE; break;
      default: assert(false);
    }

  return Speakers(FORMAT_LINEAR, mask, info.samplerate);
}

AACParser::AACParser(): h_aac(0)
{}

AACParser::~AACParser()
{
  uninit();
}

bool
AACParser::can_open(Speakers spk) const
{
  return spk.format == FORMAT_AAC_FRAME;
}

bool
AACParser::init_decoder()
{
  uint8_t *format_data = spk.format_data.get();

  if (format_data && spk.data_size > 1)
  {
    // faad does not support AAC SSR
    int object_type = format_data[0] >> 3;
    if (object_type == 3) // AAC_SSR
      return false;
  }

  unsigned long freq = 0;
  unsigned char channels = 0;

  if (h_aac) NeAACDecClose(h_aac);
  h_aac = NeAACDecOpen();
  if (!h_aac) return false;

  NeAACDecConfigurationPtr c = NeAACDecGetCurrentConfiguration(h_aac);
  c->outputFormat = AAC_SAMPLE_FORMAT;
  NeAACDecSetConfiguration(h_aac, c);

  if (spk.data_size)
    NeAACDecInit2(h_aac, format_data, (unsigned long)spk.data_size, &freq, &channels);
  // This allows not to drop the first frame
  NeAACDecPostSeekReset(h_aac, 1);

  is_fresh = true;
  return true;
}

bool
AACParser::init()
{
  out_spk = Speakers();
  new_stream_flag = false;
  return init_decoder();
}

void
AACParser::uninit()
{
  if (h_aac) NeAACDecClose(h_aac);
  h_aac = 0;
}

void
AACParser::reset()
{
  // NeAACDecInit2, NeAACDecPostSeekReset and combination of both do not clear
  // the decoder properly and glitch appears after seek.
  // NeAACDecPostSeekReset(h_aac, 0) is better than NeAACDecInit2 because it
  // forces the decoder to drop the first frame, but it does not eliminate the
  // problem completely. Only full decoder reopen allows to completely avoid
  // unpleasant 'tsch' sound after seek.
  if (!is_fresh)
    if (!init())
      THROW(EDecoderInit());

  out_spk = Speakers();
  new_stream_flag = false;
}

bool
AACParser::process(Chunk &in, Chunk &out)
{
  NeAACDecFrameInfo info;
  bool sync = in.sync;
  vtime_t time = in.time;
  in.set_sync(false, 0);

  while (in.size)
  {
    is_fresh = false;
    void *data = NeAACDecDecode(h_aac, &info, in.rawdata, (unsigned long)in.size);
    if (info.error ||                 // error happen
        info.bytesconsumed == 0 ||    // prevent infinite loop
        info.bytesconsumed > in.size) // decoder used more bytes than we have
    {
      reset();
      in.clear();
      return false;
    }

    in.drop_rawdata(info.bytesconsumed);
    if (data && info.samples)
    {
      // Output format
      Speakers new_out_spk = info2spk(info);
      if (new_out_spk != out_spk)
      {
        new_stream_flag = true;
        out_spk = new_out_spk;
      }
      else
        new_stream_flag = false;

      // Samples
      int nch = info.channels;
      int nsamples = info.samples / info.channels;
      sample_t *s = reinterpret_cast<sample_t *>(data);

      buf.allocate(nch, nsamples);
      for (int ch = 0; ch < info.channels; ch++)
        for (int i = 0, j = ch; i < nsamples; i++, j += nch)
          buf[ch][i] = s[j];

      // Reorder channels
      samples_t samples = buf.samples();
      samples.reorder_to_std(out_spk, aac_order);

      // Return
      out.set_linear(samples, nsamples, sync, time);
      return true;
    }
  }
  return false;
}

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

  uint8_t *format_data = spk.format_data.get();
  std::stringstream result;

  result << "Format data:";
  for (size_t i = 0; i < spk.data_size; i++)
    result << ' ' << digit[format_data[i] >> 4] << digit[format_data[i] & 0xf];
  result << nl;

  ReadBS bs(format_data, 0, spk.data_size * 8);
  int object_type = bs.get(5);
  int sampling_frequency_index = bs.get(4);
  int sampling_frequency = sampling_frequency_tbl[sampling_frequency_index];
  if (sampling_frequency_index == 15)
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
