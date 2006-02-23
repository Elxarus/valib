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
// ^                                                          ^
// +--------- sync_buffer_size + max_spdif_frame_size --------+
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
// sync_buffer_size = max_frame_size * 2 + header_size = 32784
// total_buffer_size = sync_buffer_size + spdif_max_frame_size = 40976
//
// DTS stream with 4096 samples per frame and DTS stream that does not satisfy
// the equation (2) cannot be transmitted over spdif. Such DTS streams are
// passed throu unchanged and output format is indicated as DTS so we still 
// have to detect such streams.

#define SPDIF_MAX_NSAMPLES   2048   // max 2048 sampels per spdif frame
#define SPDIF_MAX_FRAME_SIZE 8192   // 2048 samples max * 4 (16bit stereo PCM)
#define SPDIF_HEADER_SIZE    16     // SPDIF header size
#define HEADER_SIZE          16     // header size required to catch syncpoint
#define SYNC_BUFFER_SIZE     32784  // sync buffer size

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
  frame_buf.allocate(SYNC_BUFFER_SIZE + SPDIF_MAX_FRAME_SIZE);
  frames = 0;
  scanner.set_standard(SYNCMASK_MAD);
  reset();
}

Speakers
Spdifer::get_sync() const 
{
  return sync_spk;
}

int
Spdifer::get_frames() const
{
  return frames;
}

bool
Spdifer::load_frame()
{
  bool use_spdif_header = true;
  size_t spdif_payload_size;

  uint8_t *spdif_ptr = frame_buf.get_data();
  uint8_t *frame_ptr = frame_buf.get_data() + SPDIF_HEADER_SIZE;

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

  #define SYNC                          \
  {                                     \
    LOAD(HEADER_SIZE);                  \
    if (!frame_sync(frame_ptr) || !frame_syncinfo(frame_ptr)) \
    {                                   \
      frame_data--;                     \
      memmove(frame_ptr, frame_ptr + 1, frame_data); \
      continue;                         \
    }                                   \
  }

  #define FLUSH                         \
  {                                     \
    out_flushing = true;                \
    state = state_sync;                 \
    return true;                        \
  }

  while (1) switch (state)
  {
    ///////////////////////////////////////////////////////
    // Syncronization
    // Catch at least 3 syncpoints
    // input:
    //   frame_data - data size at sync buffer
    // output:
    //   frame_data - data size at sync buffer
    //   sync_spk - format we're synced on
    //   out_spk - output format

    case state_sync:
    {
      // find first syncpoint
      LOAD(4);
      if (!scanner.get_sync(frame_ptr))
      {
        size_t gone = scanner.scan(frame_ptr, frame_ptr + 4, frame_data - 4);
        frame_data -= gone;
        memmove(frame_ptr + 4, frame_ptr + 4 + gone, frame_data);

        if (!scanner.get_sync(frame_ptr))
        {
          assert(frame_data == 4);
          gone = scanner.scan(frame_ptr, rawdata, size);
          rawdata += gone;
          size -= gone;
          if (!scanner.get_sync(frame_ptr))
            return false;
        }
      }

      // validate syncpoint
      LOAD(HEADER_SIZE);
      if (!frame_sync(frame_ptr) || !frame_syncinfo(frame_ptr))
      {
        frame_data--;
        memmove(frame_ptr, frame_ptr + 1, frame_data);
        continue;
      }
      sync_spk = stream_spk;

      // load buffer until buffer fills or we have no more data
      {
        size_t len = MIN(size, SYNC_BUFFER_SIZE - frame_data);
        memcpy(frame_ptr + frame_data, rawdata, len);
        frame_data += len;
        rawdata += len;
        size -= len;
      }

      // count syncpoints
      // find max(frame_size) and max(nsamples) along syncpoints found
      size_t max_nsamples = 0;
      size_t max_frame_size = 0;
      int sync_count = 0;

      uint8_t *pos = frame_ptr;
      while (pos < frame_ptr + frame_data - HEADER_SIZE)
      {
        if (frame_sync(pos) && frame_syncinfo(pos))
        {
          if (stream_spk != sync_spk) 
            break;

          sync_count++;

          if (frame_size > max_frame_size)
            max_frame_size = frame_size;

          if (nsamples > max_nsamples)
            max_nsamples = nsamples;

          pos += frame_size;
        }
        else
          pos++;
      }

      // if we did not catch 3 syncpoints
      if (sync_count < 3)
      {
        if (size)
        {
          // resync
          frame_data--;
          memmove(frame_ptr, frame_ptr + 1, frame_data);
          continue;
        }
        else 
        {
          // we have no enough data
          sync_spk = spk_unknown;
          out_spk = spk_unknown;
          return false;
        }
      }

      // find spdif payload size and decide type of output
      // load_frame() always generates at least 3 frames 
      // after successful syncronization

      spdif_payload_size = nsamples * 4;
      if (use_spdif_header)
        spdif_payload_size -= SPDIF_HEADER_SIZE;

      if (max_frame_size > spdif_payload_size || max_nsamples > SPDIF_MAX_NSAMPLES)
      {
        out_spk = sync_spk;
        state = state_passthrough;
      }
      else
      {
        out_spk = sync_spk;
        out_spk.format = FORMAT_SPDIF;
        state = state_spdif;
      }
      continue;
    }

    ///////////////////////////////////////////////////////
    // Spdif mode
    // Load and format spdif frame
    // input:
    //   frame_data - data size at sync buffer
    //   sync_spk - format we're synced on
    //   out_spk - output format
    // output:
    //   frame_data - data size at sync buffer
    //   out_rawdata - pointer to output buffer
    //   out_size - data size at output buffer

    case state_drop_spdif:
    {
      // find spdif payload size
      spdif_payload_size = nsamples * 4;
      if (use_spdif_header)
        spdif_payload_size -= SPDIF_HEADER_SIZE;

      // drop frame sent
      memmove(frame_ptr, frame_ptr + spdif_payload_size, frame_data - spdif_payload_size);
      frame_data -= spdif_payload_size;
      state = state_spdif;
      // no break: now we go to state_spdif...
    }

    case state_spdif:
    {
      // find syncpoint
      LOAD(4);
      if (!scanner.get_sync(frame_ptr))
      {
        size_t gone = scanner.scan(frame_ptr, frame_ptr + 4, frame_data - 4);
        frame_data -= gone;
        memmove(frame_ptr + 4, frame_ptr + 4 + gone, frame_data);

        if (!scanner.get_sync(frame_ptr))
        {
          assert(frame_data == 4);
          gone = scanner.scan(frame_ptr, rawdata, size);
          rawdata += gone;
          size -= gone;
          if (!scanner.get_sync(frame_ptr))
            return false;
        }
      }

      // validate syncpoint
      LOAD(HEADER_SIZE);
      if (!frame_sync(frame_ptr) || !frame_syncinfo(frame_ptr))
      {
        frame_data--;
        memmove(frame_ptr, frame_ptr + 1, frame_data);
        continue;
      }

      // switch streams
      if (stream_spk != sync_spk)
        FLUSH;

      // load frame
      LOAD(frame_size);

      // find spdif payload size
      spdif_payload_size = nsamples * 4;
      if (use_spdif_header)
        spdif_payload_size -= SPDIF_HEADER_SIZE;

      // switch to passthrough mode
      // state_spdif -> state_flush -> state_sync -> state_passthrough
      if (frame_size > spdif_payload_size || nsamples > SPDIF_MAX_NSAMPLES)
        FLUSH;

      // fill spdif header
      spdif_header_t *hdr = (spdif_header_t *)frame_buf.get_data();
      hdr->set(magic, frame_size * 8);

      // move data and fill padding
      if (frame_data > frame_size)
        memmove(frame_ptr + spdif_payload_size, frame_ptr + frame_size, frame_data - frame_size);
      memset(frame_ptr + frame_size, 0, spdif_payload_size - frame_size);
      frame_data += spdif_payload_size - frame_size;

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
      if (use_spdif_header)
        out_rawdata = spdif_ptr;
      else
        out_rawdata = frame_ptr;
      out_size = nsamples * 4;
      state = state_drop_spdif;
      return true;
    }

    ///////////////////////////////////////////////////////
    // Passthrough mode
    // Load a frame and send it unchanged
    // input:
    //   frame_data - data size at sync buffer
    //   sync_spk - format we're synced on
    //   out_spk - output format
    // note that out_spk == sync_spk because it is 
    // passthrough mode!
    // output:
    //   frame_data - data size at sync buffer
    //   out_rawdata - pointer to output buffer
    //   out_size - data size at output buffer

    case state_drop_passthrough:
    {
      // drop frame sent
      memmove(frame_ptr, frame_ptr + frame_size, frame_data - frame_size);
      frame_data -= frame_size;
      state = state_passthrough;
      // no break: now we go to state_passthrough...
    }

    case state_passthrough:
    {
      // find syncpoint
      LOAD(4);
      if (!scanner.get_sync(frame_ptr))
      {
        size_t gone = scanner.scan(frame_ptr, frame_ptr + 4, frame_data - 4);
        frame_data -= gone;
        memmove(frame_ptr + 4, frame_ptr + 4 + gone, frame_data);

        if (!scanner.get_sync(frame_ptr))
        {
          assert(frame_data == 4);
          gone = scanner.scan(frame_ptr, rawdata, size);
          rawdata += gone;
          size -= gone;
          if (!scanner.get_sync(frame_ptr))
            return false;
        }
      }

      // validate syncpoint
      LOAD(HEADER_SIZE);
      if (!frame_sync(frame_ptr) || !frame_syncinfo(frame_ptr))
      {
        frame_data--;
        memmove(frame_ptr, frame_ptr + 1, frame_data);
        continue;
      }

      // switch streams
      if (stream_spk != sync_spk)
        FLUSH;

      // load frame
      LOAD(frame_size);

      // return data
      frames++;
      out_rawdata = frame_ptr;
      out_size = frame_size;
      state = state_drop_passthrough;
      return true;
    }
  }

  // never be here
  assert(false);
  return false;
}

///////////////////////////////////////////////////////////////////////////////
// Filter interface
///////////////////////////////////////////////////////////////////////////////

void
Spdifer::reset()
{
  NullFilter::reset();
  sync_helper.reset();
  scanner.reset();

  frame_data = 0;
  frames = 0;

  sync_spk = spk_unknown;

  out_spk = spk_unknown;
  out_rawdata = 0;
  out_size = 0;
  out_flushing = false;

  stream_spk = spk_unknown;
  frame_size = 0;
  nsamples = 0;

  state = state_sync;
}

bool
Spdifer::query_input(Speakers _spk) const
{
  return (FORMAT_MASK(_spk.format) & format_mask) != 0;
}

bool 
Spdifer::process(const Chunk *_chunk)
{
  FILTER_SAFE(receive_chunk(_chunk));

  // receive sync
  if (sync)
  {
    sync_helper.receive_sync(sync, time);
    sync = false;
  }

  // load next frame
  load_frame();

  // if we did not start a stream we must
  // forget about current stream on flushing
  // and drop data currently buffered
  // (flushing state is also dropped so we 
  // do not pass eos event in this case)
  if (flushing && out_spk == spk_unknown)
    reset();

  return true;
}

Speakers
Spdifer::get_output() const
{
  return out_spk;
}

bool 
Spdifer::is_empty() const
{
  return !size && !out_size && !flushing && !out_flushing;
}

bool 
Spdifer::get_chunk(Chunk *_chunk)
{
  if (!out_size && !out_flushing)
    load_frame();

  if (out_size)
  {
    // send data chunk
    _chunk->set_rawdata(out_spk, out_rawdata, out_size);
    out_size = 0;
  }
  else if (out_flushing)
  {
    // send inter-stream flushing and forget current stream
    _chunk->set_empty(out_spk, false, 0, true);
    out_spk = spk_unknown;
    out_flushing = false;

    // try to start a new stream
    load_frame();

    // if we're flushing and we did not start a new stream
    // we must drop data currently buffered
    // (flushing state is also dropped so we will not send
    // excessive flushing)
    if (flushing && out_spk == spk_unknown)
      reset();
  }
  else if (flushing)
  {
    // send filter flushing, forget current stream
    // and drop data buffered
    _chunk->set_empty(out_spk, false, 0, true);
    flushing = false;
    reset();
  }

  // timing
  // note thet filter flushing will be untimed
  // because of reset() call (is it a problem???)
  sync_helper.send_sync(_chunk);
  sync_helper.set_syncing(true);
  return true;
};

///////////////////////////////////////////////////////////////////////////////
// Syncronization info parsers
///////////////////////////////////////////////////////////////////////////////

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
  union MPAHeader
  {
    MPAHeader() {};
    MPAHeader(uint32_t i) { raw = i; }

    uint32_t raw;
    struct
    {
      unsigned emphasis           : 2;
      unsigned original           : 1;
      unsigned copyright          : 1;
      unsigned mode_ext           : 2;
      unsigned mode               : 2;
      unsigned extension          : 1;
      unsigned padding            : 1;
      unsigned sampling_frequency : 2;
      unsigned bitrate_index      : 4;
      unsigned error_protection   : 1;
      unsigned layer              : 2;
      unsigned version            : 1;
      unsigned sync               : 12;
    };
  };

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

  MPAHeader h;

  // MPA low and big endians have ambigous headers
  // so first we check low endian as most used and only
  // then try big endian

  // 8 bit or 16 bit little endian steram sync
  if ((_buf[0] == 0xff)         && // sync
     ((_buf[1] & 0xf0) == 0xf0) && // sync
     ((_buf[1] & 0x06) != 0x00) && // layer
     ((_buf[2] & 0xf0) != 0xf0) && // bitrate
     ((_buf[2] & 0xf0) != 0x00) && // prohibit free-format
     ((_buf[2] & 0x0c) != 0x0c))   // sample rate
  {
    uint32_t header = *(uint32_t *)_buf;
    h = swab_u32(header);
    bs_type = BITSTREAM_8;
  }
  else
  // 16 bit big endian steram sync
  if ((_buf[1] == 0xff)         && // sync
     ((_buf[0] & 0xf0) == 0xf0) && // sync
     ((_buf[0] & 0x06) != 0x00) && // layer
     ((_buf[3] & 0xf0) != 0xf0) && // bitrate
     ((_buf[3] & 0xf0) != 0x00) && // prohibit free-format
     ((_buf[3] & 0x0c) != 0x0c))   // sample rate
  {
    uint32_t header = *(uint32_t *)_buf;
    h = (header >> 16) | (header << 16);
    bs_type = BITSTREAM_16BE;
  }
  else
    return false;

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

  static const int amode2mask_tbl[] = 
  {
    MODE_MONO,   MODE_STEREO,  MODE_STEREO,  MODE_STEREO,  MODE_STEREO,
    MODE_3_0,    MODE_2_1,     MODE_3_1,     MODE_2_2,     MODE_3_2
  };

  static const int amode2rel_tbl[] = 
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
