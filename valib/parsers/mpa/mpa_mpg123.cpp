#include <sstream>
#include "../../../3rdparty/libmpg123/mpg123.h"
#include "mpa_mpg123.h"

class MPG123Init
{
public:
  MPG123Init()
  { mpg123_init(); }

  ~MPG123Init()
  { mpg123_exit(); }
};

volatile static MPG123Init init;

static inline Speakers get_format(void *mh)
{
  long rate;
  int channels, enc;
  mpg123_getformat((mpg123_handle *)mh, &rate, &channels, &enc);

  int format, mask;
  switch (enc)
  {
    case MPG123_ENC_SIGNED_16: format = FORMAT_PCM16; break;
    case MPG123_ENC_SIGNED_32: format = FORMAT_PCM32; break;
    case MPG123_ENC_SIGNED_24: format = FORMAT_PCM24; break;
    case MPG123_ENC_FLOAT_32:  format = FORMAT_PCMFLOAT; break;
    case MPG123_ENC_FLOAT_64:  format = FORMAT_PCMDOUBLE; break;
    default: return spk_unknown;
  }

  switch (channels)
  {
    case MPG123_MONO: mask = MODE_MONO; break;
    case MPG123_STEREO: mask = MODE_STEREO; break;
    default: return spk_unknown;
  }

  return Speakers(format, mask, rate);
}

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

MPG123Parser::MPG123Parser()
{
  mh = mpg123_new(0, 0);
  if (mh)
  {
    mpg123_param((mpg123_handle*)mh, MPG123_FLAGS, MPG123_FORCE_FLOAT, 0);
  }

  frames = 0;
  errors = 0;
  reset();
}

MPG123Parser::~MPG123Parser()
{
  if (mh) mpg123_delete((mpg123_handle*)mh);
}

///////////////////////////////////////////////////////////////////////////////
// SimpleFilter overrides

bool
MPG123Parser::can_open(Speakers spk) const
{
  return mh && spk.format == FORMAT_MPA;
}

bool
MPG123Parser::init()
{
  reset();
  return true;
}

void 
MPG123Parser::reset()
{
  mpg123_open_feed((mpg123_handle*)mh);
  state = state_feed;
  new_stream_state = no_new_stream;
  sync.reset();
  out_spk = spk_unknown;
}

bool
MPG123Parser::process(Chunk &in, Chunk &out)
{
  off_t num;
  unsigned char *audio;
  size_t bytes;
  int r;

  sync.receive_sync(in);
  while (true) switch (state)
  {
  case state_feed:
    if (in.size == 0)
      return false;

    r = mpg123_feed((mpg123_handle*)mh, in.rawdata, in.size);
    in.clear();
    if (r != MPG123_OK)
    {
      errors++;
      return false;
    }
    state = state_decode;
    continue;

  case state_decode:
    r = mpg123_decode_frame((mpg123_handle*)mh, &num, &audio, &bytes);
    switch (r)
    {
    case MPG123_OK:
      if (out_spk.is_unknown() || new_stream_state == new_stream_next)
      {
        new_stream_state = new_stream_now;
        out_spk = get_format(mh);
      }
      else if (new_stream_state == new_stream_now)
        new_stream_state = no_new_stream;
      break;
    case MPG123_NEED_MORE:
      state = state_feed;
      continue;
    case MPG123_NEW_FORMAT:
      new_stream_state = new_stream_next;
      break;
    default:
      errors++;
      continue;
    }

    if (bytes == 0)
      continue;

    out.set_rawdata(audio, bytes);
    sync.send_frame_sync(out);
    frames++;
    return true;
  }
}

static const char *layer_text(int layer)
{
  switch (layer)
  {
    case 1: return "Layer I";
    case 2: return "Layer II";
    case 3: return "Layer III (MP3)";
    default: return "Unknown";
  }
}

static const char *ver_text(mpg123_version ver)
{
  switch (ver)
  {
    case MPG123_1_0: return "MPEG1";
    case MPG123_2_0: return "MPEG2";
    case MPG123_2_5: return "MPEG2.5";
    default: return "Unknown";
  }
}

static const char *mode_text(mpg123_mode mode)
{
  switch (mode)
  {
    case MPG123_M_STEREO: return "Stereo";
    case MPG123_M_JOINT:  return "Joint Stereo";
    case MPG123_M_DUAL:   return "Dual Channel";
    case MPG123_M_MONO:   return "Mono";
    default: return "Unknown";
  }
}

string
MPG123Parser::info() const 
{
  using std::endl;
  std::stringstream result;
  result << "Format: " << Speakers(FORMAT_MPA, out_spk.mask, out_spk.sample_rate).print() << endl;

  mpg123_frameinfo info;
  if (mpg123_info((mpg123_handle*)mh, &info) == MPG123_OK)
  {
    result << "Version: " << ver_text(info.version) << " " << layer_text(info.layer) << endl;
    result << "Mode: " << mode_text(info.mode) << endl;
    result << "Frame size: " << info.framesize << endl;
    result << "Bitrate: " << info.bitrate << "kbps" << endl;
  }
  return result.str();
}
