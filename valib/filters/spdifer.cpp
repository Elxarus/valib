#include <stdio.h>
#include "crc.h"
#include "spdifer.h"
#include "bitstream.h"

#define SPDIF_MAX_FRAME_SIZE 2048*4 // 2048 samples max
#define SPDIF_HEADER_SIZE    8      // SPDIF header size
#define HEADER_SIZE          16     // header size required
#define SYNC_COUNT           5      // number of packets to sync

Spdifer::Spdifer()
{
  spk = unk_spk;
  stream_spk = unk_spk;

  frames = 0;
  frame_buf.allocate(SPDIF_MAX_FRAME_SIZE);
  reset();
}

int
Spdifer::get_frames()
{
  return frames;
}


void
Spdifer::reset()
{
  chunk.set_empty();

//  sync_count = 0;
  stream_spk = unk_spk;
  sync_helper.reset();
  state = state_sync;
  frame_size = 0;
  frame_data = 0;
}

bool
Spdifer::query_input(Speakers _spk) const
{
  int format_mask = FORMAT_MASK_UNKNOWN | FORMAT_MASK_MPA | FORMAT_MASK_AC3 | FORMAT_MASK_DTS | FORMAT_MASK_SPDIF;
  return (FORMAT_MASK(_spk.format) & format_mask) != 0;
}

bool
Spdifer::set_input(Speakers _spk)
{
  if (NullFilter::set_input(_spk))
  {
//    sync_count = 0;
    return true;
  }
  else
    return false;
}

bool 
Spdifer::process(const Chunk *_chunk)
{
  if (!NullFilter::process(_chunk))
    return false;

  if (!chunk.is_empty())
    sync_helper.receive_timestamp(chunk.timestamp, chunk.time);

  return true;
}

bool 
Spdifer::get_chunk(Chunk *_out)
{
  if (spk.format == FORMAT_SPDIF)
  {
    // passthrough spdif data
    *_out = chunk;
    chunk.drop(chunk.size);
    return true;
  }
  else
    _out->set_empty();

  uint8_t *buf = frame_buf.data() + SPDIF_HEADER_SIZE;
  int buf_size = frame_buf.size() - SPDIF_HEADER_SIZE;

  uint8_t *read_buf = chunk.buf;
  int read_len = chunk.size;
//  Speakers stream_spk_old = stream_spk;

  while (read_len && _out->is_empty())
    switch (state)
    {
      case state_sync:
      {
        ///////////////////////////////////////////////////
        // Sync

        // load header
        if (frame_data < HEADER_SIZE)
        {
          int l = MIN(read_len, HEADER_SIZE - frame_data);
          memcpy(buf + frame_data, read_buf, l);
          frame_data += l;
          read_buf += l;
          read_len -= l;
          continue;
        }

        // check sync
        if (!sync(buf) || !syncinfo(buf))
        {
          memmove(buf, buf+1, HEADER_SIZE - 1);
          frame_data--;
          continue;
        }
        frames++;

//        if (stream_spk != stream_spk_old)
//          sync_count = SYNC_COUNT;
//        stream_spk_old = stream_spk;

        {
          // init spdif header
          uint16_t *hdr = (uint16_t *)frame_buf.data();
          hdr[0] = 0xf872;          // Pa  sync word 1 
          hdr[1] = 0x4e1f;          // Pb  sync word 2 
          hdr[2] = magic;           // Pc  data type
          hdr[3] = frame_size * 8;  // Pd  length-code (bits)
        }

        // switch state
//        if (sync_count)
//        {
//          sync_count--;
//          state = state_drop;
//        }
//        else 
        if (frame_size > buf_size || frame_size > nsamples * 4)
          state = state_passthrough;
        else
          state = state_spdif;
        continue;

      }

      case state_drop:
      {
        ///////////////////////////////////////////////////
        // Drop packet

        // switch state
        if (!frame_size)
        {
          frame_data = 0;
          state = state_sync;
          continue;
        }

        if (frame_data)
        {
          frame_size -= frame_data;
          frame_data = 0;
        }

        // drop data
        int l = MIN(frame_size, read_len);
        read_buf += l;
        read_len -= l;
        frame_size -= l;
        continue; 
      }

      case state_spdif:
      {
        ///////////////////////////////////////////////////
        // Format spdif frame

        // load frame
        if (frame_data < frame_size)
        {
          int l = MIN(read_len, frame_size - frame_data);
          memcpy(buf + frame_data, read_buf, l);
          frame_data += l;
          read_buf += l;
          read_len -= l;
          if (frame_data < frame_size)
            continue;
        }

        // zero stuffing
        memset(buf + frame_size, 0, nsamples * 4 - frame_size - SPDIF_HEADER_SIZE);

        // convert stream format
        if (bs_type == BITSTREAM_16LE ||
            bs_type == BITSTREAM_14LE ||
            bs_type == BITSTREAM_8)
        {
          // swap bytes
          uint16_t *pos = (uint16_t *)buf;
          uint16_t *end = (uint16_t *)(buf + frame_size + 1);
          while (pos < end)
          {
            *pos = swab16(*pos);
            pos++;
          }
        }

        // prepare output chunk
        _out->set_spk(stream_spk);
        _out->spk.format = FORMAT_SPDIF;
        _out->set_buf(frame_buf.data(), nsamples * 4);
        sync_helper.set_time(_out);

        // switch state
        state = state_sync;
        frame_data = 0;
        continue;
      }

      case state_passthrough:
      {
        ///////////////////////////////////////////////////
        // Passthrough unspdifable data

        // switch state
        if (!frame_size)
        {
          state = state_sync;
          continue;
        }

        if (frame_data)
        {
          // send header
          _out->set_spk(stream_spk);
          _out->set_buf(buf, frame_data);
          sync_helper.set_time(_out);
          frame_data = 0;
          continue;
        }

        // send data
        int l = MIN(frame_size, read_len);
        _out->set_spk(stream_spk);
        _out->set_buf(read_buf, l);
        _out->set_time(false);

        read_buf += l;
        read_len -= l;
        frame_size -= l;
        continue; 
      }
    } // switch (state)
  // while (read_len && _out->is_empty())

  chunk.drop(read_buf - chunk.buf);
  return true;
};

Speakers
Spdifer::get_output()
{
  Speakers spk_tmp = stream_spk;
  spk_tmp.format = FORMAT_SPDIF;
  return spk_tmp;
}


bool
Spdifer::syncinfo(const uint8_t *_buf)
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
  // 8 bit or 16 bit little endian steram sync
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
  // 16 bit big endian steram sync
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
    return true;
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

  MPAHeader h = swab32(*(int32_t *)_buf);

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
  if (lff == 3) return 0;                 // constraint

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
Spdifer::get_info(char *_buf, int _len)
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
      memcpy(_buf, info, MIN(_len, (int)strlen(info)+1));
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
  memcpy(_buf, info, MIN(_len, (int)strlen(info)+1));
}