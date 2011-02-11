#include "../../buffer.h"
#include "aac_parser.h"
#include "libfaad/neaacdec.h"

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

bool
AACParser::can_open(Speakers spk) const
{
  return spk.format == FORMAT_AAC_FRAME;
}

bool
AACParser::init()
{
  unsigned long freq = 0;
  unsigned char channels = 0;

  uninit();
  h_aac = NeAACDecOpen();
  if (!h_aac) return false;

  NeAACDecConfigurationPtr c = NeAACDecGetCurrentConfiguration(h_aac);
  c->outputFormat = AAC_SAMPLE_FORMAT;
  NeAACDecSetConfiguration(h_aac, c);

  if (spk.data_size)
    NeAACDecInit2(h_aac, spk.format_data.get(), spk.data_size, &freq, &channels);

  out_spk = Speakers();
  new_stream_flag = false;

  return true;
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
  unsigned long freq = 0;
  unsigned char channels = 0;
  NeAACDecInit2(h_aac, 0, 0, &freq, &channels);
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
    void *data = NeAACDecDecode(h_aac, &info, in.rawdata, in.size);
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
