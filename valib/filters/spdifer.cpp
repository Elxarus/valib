#include <stdio.h>
#include "crc.h"
#include "spdifer.h"
#include "bitstream.h"

///////////////////////////////////////////////////////////////////////////////
// To sync on compressed stream we first try to catch 3 syncpoints of the same
// format. 
//
// To catch 3 syncpoints we need buffer of size
// (1) sync_buffer_size = max_frame_size * 2 + header_size
// where 
//   max_frame_size - maximum frame size among all formats
//   header_size - minimum data size required to catch a probable syncpoint
//
// Frame can be sent through SPDIF only if required bitrate is less than
// bitrate used by stereo 16bit PCM with the same number of samples, i.e.:
// (2) frame_size + spdif_header <= nsamples * 4
// where
//   frame_size - size of the compressed frame
//   nsamples - number of samples this frame contains
//   spdif_header - size of spdif header
//
// From the equation above we can also find maximum spdif frame size:
// (3) max_spdif_frame_size = max_nsamples * 4
// where 
//   max_nsamples - maximum number of samples per spdif frame allowed
//
// After we catch 3 syncpoints and spdif conditions are fulfilled we must send
// first frame. To do this we need some space for padding, but not more than
// maximum spdif frame size, i.e.:
// (4) total_buffer_size = sync_buffer_size + max_spdif_frame_size
//
// Spdif frame has following format:
// +--------------+---------------------------+---------------+
// | spdif_header | frame                     | padding       |
// +--------------+---------------------------+---------------+
// where
//   spdif_header - 8-byte structure contains syncronization signature,
//                  data format and size
//   frame - compressed frame
//   padding - zero padding
//
// We load only raw frame from input stream. To avoid memmove() operation
// we should place spdif header first, so having following buffer structure:
//
// During initial syncronization:
//
//                +---- sync_buffer_size -----+
//                v                           v
// +--------------+---------------------------+---------------+
// | spdif_header | sync_buffer               |    unused     |
// +--------------+---------------------------+---------------+
//                ^                                           ^
//                +- sync_buffer_size + max_spdif_frame_size -+
//
// When we send first 2 spdif frames pading is inserted in between of current 
// frame end and other data rmaining at sync buffer:
//
//                +----------+---- frame_size
//                v          v
// +--------------+----------+---------+-------------+--------+
// | spdif_header | frame    | padding | sync_data   | unused |
// +--------------+----------+---------+-------------+--------+
// ^                                   ^
// +----------- nsamples * 4 ----------+
//
// When we send following frames only one frame is loaded at the same time and
// the rest of the buffer is unused:
//
// +--------------+--------+-----------+----------------------+
// | spdif_header | frame  | padding   |       unused         |
// +--------------+--------+-----------+----------------------+
// ^                                   ^
// +----------- nsamples * 4 ----------+
//
// Maximum frame size and number of samples (defined by standard):
//
// format | max_frame_size | max_nsamples | header_size |
// -------+----------------+--------------+-------------+
//  ac3   |      3840      |     1536     |       7     |
//  dts   |     16384      |     4096     |      16     |
//  mpa   |                |     1152     |       4     |
// -------+----------------+--------------+-------------+
//
// For DTS format thre're 3 types of SPDIF frames: 512, 1024 and 2048 samples.
// So regardless of the fact that the maximum number of samples per DTS frame
// is 4096, the maximum number of samples per SPDIF frame (spdif_max_nsamples)
// is only 2048. 
//
// So from this table we can conclude all constants mentoined above:
// spdif_max_samples = 2048
// spdif_max_frame_size = spdif_max_samples * 4 = 8192
// max_frame_size = 16384
// sync_buffer_size = max_frame_size * 2 + header_size = 16400
// total_buffer_size = sync_buffer_size + spdif_max_frame_size = 24592
//
// DTS stream with 4096 samples per frame and DTS stream that does not satisfy
// the equation (2) cannot be transmitted over spdif. Such DTS streams are
// passed throu unchanged and output format is indicated as DTS.

#define SPDIF_MAX_NSAMPLES   2048   // max 2048 sampels per spdif frame
#define SPDIF_MAX_FRAME_SIZE 8192   // 2048 samples max * 4 (16bit stereo PCM)
#define SPDIF_HEADER_SIZE    16     // SPDIF header size
#define HEADER_SIZE          16     // header size required to catch syncpoint
#define SYNC_BUFFER_SIZE     16400  // sync buffer size

// formats supported
static const int format_mask = FORMAT_MASK_UNKNOWN | FORMAT_MASK_MPA | FORMAT_MASK_AC3 | FORMAT_MASK_DTS | FORMAT_MASK_SPDIF;

struct spdif_header_t
{
  uint32_t zero1;
  uint32_t zero2;
  uint16_t sync1;   // Pa sync word 1
  uint16_t sync2;   // Pb sync word 2
  uint16_t type;    // Pc data type
  uint16_t len;     // Pd length-code (bits)

  inline void set(uint16_t _type, size_t _len_bits)
  {
    zero1 = 0;
    zero2 = 0;
    sync1 = 0xf872;
    sync2 = 0x4e1f;
    type  = _type;
    len   = _len_bits;
  }
};

const uint8_t spdif_pause[] =
{
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x72, 0xf8, 0x1f, 0x4e, // Sync
  0x00, 0x00,             // type = null data
  0x40, 0x00,             // length = 64bit
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

Spdifer::Spdifer()
{
  sync_spk = unk_spk;
  frame_buf.allocate(SPDIF_HEADER_SIZE + SYNC_BUFFER_SIZE + SPDIF_MAX_FRAME_SIZE);
  frame_data = 0;
  frames = 0;

  stream_spk = unk_spk;
  frame_size = 0;
  nsamples = 0;

  state = state_sync;
}

int
Spdifer::get_frames()
{
  return frames;
}


void
Spdifer::reset()
{
  NullFilter::reset();

  sync_spk = unk_spk;
  frame_data = 0;
  frames = 0;

  stream_spk = unk_spk;
  frame_size = 0;
  nsamples = 0;

  sync_helper.reset();
  state = state_sync;
}

bool
Spdifer::query_input(Speakers _spk) const
{
  return (FORMAT_MASK(_spk.format) & format_mask) != 0;
}

Speakers
Spdifer::get_output() const
{
  Speakers spk_tmp = sync_spk;
  spk_tmp.format = FORMAT_SPDIF;
  return spk_tmp;
}


bool
Spdifer::load_frame()
{
  bool use_spdif_header = true;

  uint8_t *frame_ptr = frame_buf.get_data() + SPDIF_HEADER_SIZE;
  size_t spdif_frame_size;

  #define LOAD(amount)                  \
  if (frame_data < (amount))            \
  {                                     \
    size_t len = (amount) - frame_data; \
    if (size < len)                     \
    {                                   \
      /* have no enough data */         \
      memcpy(frame_ptr + frame_data, rawdata, size);  \
      frame_data += size;               \
      rawdata += size;                  \
      size = 0;                         \
      return false;                     \
    }                                   \
    else                                \
    {                                   \
      memcpy(frame_ptr + frame_data, rawdata, len);   \
      frame_data += len;                \
      rawdata += len;                   \
      size -= len;                      \
    }                                   \
  }

  while (1) switch (state)
  {
    ///////////////////////////////////////////////////////
    // Syncronization (catch at least 3 syncpoints)
    // frame_data - data size at sync buffer
    // other: ignored

    case state_sync:
    {
      LOAD(HEADER_SIZE);

      // check sync
      if (!frame_sync(frame_ptr) || !frame_syncinfo(frame_ptr))
      {
        memmove(frame_ptr, frame_ptr + 1, frame_data - 1);
        frame_data--;
        continue;
      }

      LOAD(SYNC_BUFFER_SIZE);

      // count syncpoints
      Speakers old_spk = stream_spk;
      uint8_t *pos = frame_ptr + frame_size;
      int sync_count = 0;

      while (pos < frame_ptr + SYNC_BUFFER_SIZE - HEADER_SIZE)
      {
        if (frame_sync(pos) && frame_syncinfo(pos))          
        {
          if (stream_spk != old_spk) break;
          pos += frame_size;
          sync_count++;
        }
        else
          pos++;
      }

      // resync
      if (sync_count < 3 || pos < frame_ptr + SYNC_BUFFER_SIZE - HEADER_SIZE)
      {
        memmove(frame_ptr, frame_ptr + 1, frame_data - 1);
        frame_data--;
        continue;
      }

      // switch state
      sync_spk = old_spk;
      state = state_frame;
      continue;
    }

    ///////////////////////////////////////////////////////
    // Load frame
    // sync_spk - stream config we're synced on
    // frame_data - data size at frame buffer

    case state_frame:
    {
      LOAD(HEADER_SIZE);

      // check sync
      if (!frame_sync(frame_ptr) || !frame_syncinfo(frame_ptr))
      {
        memmove(frame_ptr, frame_ptr + 1, frame_data - 1);
        frame_data--;
        continue;
      }

      // resync
      if (stream_spk != sync_spk)
      {
        state = state_sync;
        continue;
      }

      // load frame
      LOAD(frame_size);

      // decide usage of spdif header and find spdif frame size
      if (use_spdif_header)
      {
        spdif_header = SPDIF_HEADER_SIZE;
        spdif_frame_size = nsamples * 4 - SPDIF_HEADER_SIZE;
      }
      else
      {
        spdif_header = 0;
        spdif_frame_size = nsamples * 4;
      }

      // send passthrough frame
      if (frame_size > spdif_frame_size || nsamples > SPDIF_MAX_NSAMPLES)
      {
        spdif_header = 0;
        state = state_send;
        return true;
      }

      // fill spdif header
      spdif_header_t *hdr = (spdif_header_t *)frame_buf.get_data();
      hdr->set(magic, frame_size * 8);

      // move data and fill padding
      if (frame_data > frame_size)
        memmove(frame_ptr + spdif_frame_size, frame_ptr + frame_size, frame_data - frame_size);
      memset(frame_ptr + frame_size, 0, spdif_frame_size - frame_size);
      frame_data += spdif_frame_size - frame_size;

      // convert stream format
      if (bs_type == BITSTREAM_16LE ||
          bs_type == BITSTREAM_14LE ||
          bs_type == BITSTREAM_8)
      {
        // swap bytes
        uint16_t *pos = (uint16_t *)frame_ptr;
        uint16_t *end = (uint16_t *)(frame_ptr + frame_size + 1);
        while (pos < end)
        {
          *pos = swab_u16(*pos);
          pos++;
        }
      }

      // return data
      frames++;
      frame_size = spdif_frame_size;
      state = state_send;
      return true;
    }

    ///////////////////////////////////////////////////////
    // Send frame
    // This state is used to say that we're ready to send
    // frame loaded. So if we're here again we have this
    // frame sent and need to switch back to state_frame.
    //
    // frame_data - data size at frame buffer
    // frame_size - spdif frame size (excluding header)

    case state_send:
    {
      if (frame_data > frame_size)
        memmove(frame_ptr, frame_ptr + frame_size, frame_data - frame_size);

      frame_data -= frame_size;
      state = state_frame;
      continue;
    }
  }

  // never be here
  return false;
}


bool 
Spdifer::get_chunk(Chunk *_chunk)
{
  // receive sync
  if (sync)
  {
    sync_helper.receive_sync(sync, time);
    sync = false;
  }

  if (load_frame())
    _chunk->set(get_output(), frame_buf + SPDIF_HEADER_SIZE - spdif_header, frame_size + spdif_header, false, 0, flushing && !size);
  else
    _chunk->set(get_output(), 0, 0, false, 0, flushing && !size);

  // timing
  sync_helper.send_sync(_chunk);
  sync_helper.set_syncing(true);

  // end-of-stream
  flushing = flushing && size;
  return true;
};

bool
Spdifer::frame_syncinfo(const uint8_t *_buf)
{
  if (ac3_sync(_buf))
    return ac3_syncinfo(_buf);
  if (dts_sync(_buf))
    return dts_syncinfo(_buf);
  if (mpa_sync(_buf))
    return mpa_syncinfo(_buf);

  return false;
}


bool
Spdifer::ac3_syncinfo(const uint8_t *_buf)
{
  static const int halfrate_tbl[12] = 
  { 
    0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3
  };
  static const int lfe_mask[] = 
  { 
    16, 16, 4, 4, 4, 1, 4, 1
  };
  static const int bitrate_tbl[] = 
  { 
    32,  40,  48,  56,  64,  80,  96, 112,
   128, 160, 192, 224, 256, 320, 384, 448,
   512, 576, 640 
  };
  static const int acmod2mask_tbl[] = 
  {
    MODE_2_0,
    MODE_1_0, 
    MODE_2_0,
    MODE_3_0,
    MODE_2_1,
    MODE_3_1,
    MODE_2_2,
    MODE_3_2,
    MODE_2_0 | CH_MASK_LFE,
    MODE_1_0 | CH_MASK_LFE, 
    MODE_2_0 | CH_MASK_LFE,
    MODE_3_0 | CH_MASK_LFE,
    MODE_2_1 | CH_MASK_LFE,
    MODE_3_1 | CH_MASK_LFE,
    MODE_2_2 | CH_MASK_LFE,
    MODE_3_2 | CH_MASK_LFE,
  };

  int fscod;
  int frmsizecod;

  int acmod;
  int dolby = NO_RELATION;

  int halfrate;
  int bitrate;
  int sample_rate;

  /////////////////////////////////////////////////////////
  // 8 bit or 16 bit little endian stream sync
  if ((_buf[0] == 0x0b) && (_buf[1] == 0x77))
  {
    fscod      = _buf[4] >> 6;
    frmsizecod = _buf[4] & 0x3f;
    acmod      = _buf[6] >> 5;

    if (acmod == 2 && (_buf[6] & 0x18) == 0x10)
      dolby = RELATION_DOLBY;

    if (_buf[6] & lfe_mask[acmod])
      acmod |= 8;

    halfrate   = halfrate_tbl[_buf[5] >> 3];
    bitrate    = bitrate_tbl[frmsizecod >> 1];

    bs_type = BITSTREAM_8;
  }
  /////////////////////////////////////////////////////////
  // 16 bit big endian stream sync
  else if ((_buf[1] == 0x0b) && (_buf[0] == 0x77))
  {
    fscod      = _buf[5] >> 6;
    frmsizecod = _buf[5] & 0x3f;
    acmod      = _buf[7] >> 5;

    if (acmod == 2 && (_buf[7] & 0x18) == 0x10)
      dolby = RELATION_DOLBY;

    if (_buf[7] & lfe_mask[acmod])
      acmod |= 8;

    halfrate   = halfrate_tbl[_buf[4] >> 3];
    bitrate    = bitrate_tbl[frmsizecod >> 1];

    bs_type = BITSTREAM_16BE;
  }
  else
    return false;

  switch (fscod) 
  {
    case 0:    
      frame_size = 4 * bitrate;
      sample_rate = 48000 >> halfrate;
      break;

    case 1: 
      frame_size = 2 * (320 * bitrate / 147 + (frmsizecod & 1));
      sample_rate = 44100 >> halfrate;
      break;

    case 2: 
      frame_size = 6 * bitrate;
      sample_rate = 32000 >> halfrate;
  }

  nsamples = 1536;
  stream_spk = Speakers(FORMAT_AC3, acmod2mask_tbl[acmod], sample_rate, 1.0, dolby);
  magic = 1; // Pc burst-info (data type = AC3) 
  return true;
}


bool
Spdifer::mpa_syncinfo(const uint8_t *_buf)
{
  static const int bitrate_tbl[2][3][15] =
  {
    { // MPEG1
      { 0, 32, 64, 96, 128, 160, 192, 224, 256, 288, 320, 352, 384, 416, 448 },
      { 0, 32, 48, 56,  64,  80,  96, 112, 128, 160, 192, 224, 256, 320, 384 },
      { 0, 32, 40, 48,  56,  64,  80,  96, 112, 128, 160, 192, 224, 256, 320 }
    }, 
    { // MPEG2 LSF
      { 0, 32, 48, 56,  64,  80,  96, 112, 128, 144, 160, 176, 192, 224, 256 },
      { 0,  8, 16, 24,  32,  40,  48,  56,  64,  80,  96, 112, 128, 144, 160 },
      { 0,  8, 16, 24,  32,  40,  48,  56,  64,  80,  96, 112, 128, 144, 160 }
    }
  };
  static const int freq_tbl[2][3] =
  {
    { 44100, 48000, 32000 },  // MPEG1
    { 22050, 24000, 16000 }   // MPEG2 LSF
  };
  static const int slots_tbl[3] = { 12, 144, 144 };

  MPAHeader h = swab_u32(*(int32_t *)_buf);

  // common information
  int ver = 1 - h.version;
  int layer = 3 - h.layer;
  int bitrate = bitrate_tbl[ver][layer][h.bitrate_index] * 1000;
  int sample_rate = freq_tbl[ver][h.sampling_frequency];

  // frame size calculation
  frame_size = bitrate * slots_tbl[layer] / sample_rate + h.padding;
  if (layer == 0) // MPA_LAYER_I
    frame_size *= 4;

  if (ver)
  {
    // MPEG2 LSF
    if (layer == 0)
      magic = 0x0008; // Pc burst-info (data type = MPEG2 Layer I LSF) 
    else
      magic = 0x0009; // Pc burst-info (data type = MPEG2 Layer II/III LSF) 
  }
  else
  {
    // MPEG1
    if (layer == 0)
      magic = 0x0004; // Pc burst-info (data type = MPEG1 Layer I) 
    else
      magic = 0x0005; // Pc burst-info (data type = MPEG1/2 Layer II/III) 
  }

  nsamples = layer == 0? 384: 1152;
  stream_spk = Speakers(FORMAT_MPA, (h.mode == 3)? MODE_MONO: MODE_STEREO, sample_rate);
  bs_type = BITSTREAM_8;
  return true;
}

bool
Spdifer::dts_syncinfo(const uint8_t *_buf)
{
  static const int dts_sample_rates[] =
  {
    0, 8000, 16000, 32000, 0, 0, 11025, 22050, 44100, 0, 0,
    12000, 24000, 48000, 96000, 192000
  };

  static const amode2mask_tbl[] = 
  {
    MODE_MONO,   MODE_STEREO,  MODE_STEREO,  MODE_STEREO,  MODE_STEREO,
    MODE_3_0,    MODE_2_1,     MODE_3_1,     MODE_2_2,     MODE_3_2
  };

  static const amode2rel_tbl[] = 
  {
    NO_RELATION,   NO_RELATION,  NO_RELATION,  RELATION_SUMDIFF, RELATION_DOLBY,
    NO_RELATION,   NO_RELATION,  NO_RELATION,  NO_RELATION,      NO_RELATION,
  };


  ReadBS bs_tmp;
  switch (_buf[0])
  {
    case 0xff: bs_type = BITSTREAM_14BE; break;
    case 0x1f: bs_type = BITSTREAM_14LE; break;
    case 0xfe: bs_type = BITSTREAM_16BE; break;
    case 0x7f: bs_type = BITSTREAM_16LE; break;
    default: return false;
  }

  bs_tmp.set_ptr(_buf, bs_type);
  bs_tmp.get(32);                         // Sync
  bs_tmp.get(6);                          // Frame type(1), Deficit sample count(5)
  int cpf = bs_tmp.get(1);                // CRC present flag

  int nblks = bs_tmp.get(7) + 1;          // Number of PCM sample blocks
  if (nblks < 5) return false;            // constraint

  frame_size = bs_tmp.get(14) + 1;        // Primary frame byte size
  if (frame_size < 95) return false;      // constraint

  if (bs_tmp.get_type() == BITSTREAM_14LE ||
      bs_tmp.get_type() == BITSTREAM_14BE)
    frame_size = frame_size * 16 / 14;


  int amode = bs_tmp.get(6);              // Audio channel arrangement
  if (amode > 0xc) return false;          // we don't work with more than 6 channels

  int sfreq = bs_tmp.get(4);              // Core audio sampling frequency
  if (!dts_sample_rates[sfreq])           // constraint
    return false; 

  bs_tmp.get(15);                         // Transmission bit rate(5), and other flags....

  int lff = bs_tmp.get(2);                // Low frequency effects flag
  if (lff == 3) return false;             // constraint

  if (cpf)
  {
    int hcrc = bs_tmp.get(16);            // Header CRC
    // todo: header CRC check
  }

  nsamples = nblks * 32;
  switch (nsamples)
  {
    case 512:  magic = 11; break;
    case 1024: magic = 12; break;
    case 2048: magic = 13; break;
    default:   magic = 13; break;
  }

  int sample_rate = dts_sample_rates[sfreq];
  int mask = amode2mask_tbl[amode];
  int relation = amode2rel_tbl[amode];
  if (lff) mask |= CH_MASK_LFE;
  stream_spk = Speakers(FORMAT_DTS, mask, sample_rate, 1.0, relation);

  return true;
}


void 
Spdifer::get_info(char *_buf, size_t _len)
{
  char info[1024];
  const char *format = 0;
  switch (stream_spk.format)
  {
    case FORMAT_AC3: format = "SPDIF/AC3"; break;
    case FORMAT_MPA: format = "SPDIF/MPEG Audio"; break;
    case FORMAT_DTS: format = "SPDIF/DTS"; break;
    default:
      sprintf(info, "-");
      memcpy(_buf, info, MIN(_len, strlen(info)+1));
      return;
  }

  const char *bs_type_text = 0;
  switch (bs_type)
  {
    case BITSTREAM_8:    bs_type_text = "8bit"; break;
    case BITSTREAM_14LE: bs_type_text = "14bit LE"; break;
    case BITSTREAM_14BE: bs_type_text = "14bit BE"; break;
    case BITSTREAM_16LE: bs_type_text = "16bit LE"; break;
    case BITSTREAM_16BE: bs_type_text = "16bit BE"; break;
  }

  sprintf(info,
    "%s\n"
    "speakers: %s\n"
    "sample rate: %iHz\n"
    "stream: %s\n"
    "frame size: %i bytes\n"
    "nsamples: %i\n",
    format,
    stream_spk.mode_text(),
    stream_spk.sample_rate,
    bs_type_text,
    frame_size,
    nsamples);
  memcpy(_buf, info, MIN(_len, strlen(info)+1));
}
