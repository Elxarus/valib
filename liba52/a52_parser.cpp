#include <string.h>
#include "a52_parser.h"
#include "crc.h"

Speakers a52_to_spk(int _mode, int _sample_rate)
{
  const int acmod2mask_tbl[] = 
  {
    MODE_2_0,
    MODE_1_0, 
    MODE_2_0,
    MODE_3_0,
    MODE_2_1,
    MODE_3_1,
    MODE_2_2,
    MODE_3_2,
    MODE_1_0,
    MODE_1_0,
    MODE_2_0,
  };

  int mask = (_mode & A52_LFE)? CH_MASK_LFE: 0;
  int mode = _mode & A52_CHANNEL_MASK;
  int dolby = NO_RELATION;
  switch (mode)
  {
  case A52_DOLBY:
    mask |= MODE_STEREO;
    dolby = RELATION_DOLBY;
    break;

  default:
    mask |= acmod2mask_tbl[mode];
    break;
  }
  return Speakers(FORMAT_LINEAR, mask, _sample_rate, 1.0, dolby);
}


A52Parser::A52Parser()
{
  frames = 0;
  errors = 0;

  samples.allocate(6, 1536);
  a52_state = a52_init(0);
  reset();
}

A52Parser::~A52Parser()
{
  a52_free(a52_state);
}


void 
A52Parser::reset()
{
  frame_size = 0;
  frame_data = 0;
  state = state_sync;
}

unsigned
A52Parser::load_frame(uint8_t **_buf, uint8_t *_end)
{
  int bitrate;
  sample_t level;

  int l;
  int required;
  uint8_t *pos;
  uint8_t *end;

  if (state == state_decode)
  {
    // drop previous frame
    frame_data -= frame_size;
    memmove(frame_buf, frame_buf + frame_size, frame_data);
    state = state_sync;
  }
  
  // state_sync:
  // frame_data - data in the frame buffer
  // frame_size - previous frame size
  //
  // state_load_frame:
  // frame_data - data in the frame_buffer
  // frame_size - new frame size
  // frame_size > frame_data
  //
  // state_decode:
  // frame_data - data in the frame_buffer
  // frame_size - new frame size
  // frame_size <=  frame_data

  // note: state == state_sync || state == state_load_frame

  if (state == state_sync)
    required = 128;
  else
    required = frame_size;

  while (1)
  {
    ///////////////////////////////////////////////////////
    // Load frame buffer

    l = required - frame_data;
    if (l > 0)
    {
      if (*_buf + l > _end)
      {
        l = _end - *_buf;
        // copy all data from input and return
        memcpy(frame_buf + frame_data, *_buf, l);
        frame_data += l;
        *_buf = _end;
        return 0;
      }
      // copy requested data and continue
      memcpy(frame_buf + frame_data, *_buf, l);
      frame_data += l;
      *_buf += l;
    }

    // note: frame_data >= required >= AC3_MIN_FRAME_SIZE

    // note: state == state_sync || state == state_load_frame
    switch (state)
    {
      case state_sync:
      {
        pos = frame_buf;
        end = frame_buf + frame_data - 6;
        while (pos < end && !(required = a52_syncinfo(pos, &mode, &sample_rate, &bitrate)))
          pos++;

        frame_data = end - pos + 6;

        if (pos > frame_buf)
          memmove(frame_buf, pos, frame_data);

        if (required)
        {
          frame_size = required;
          state = state_load_frame;
        }
        else
          required = 128;

        continue;
      } // case state_sync:

      case state_load_frame:
      {
        // note: frame_data >= required == frame size

        ///////////////////////////////////////////////////////
        // CRC check

        frame_size = required;
        int frame_size1 = ((frame_size >> 1) + (frame_size >> 3)) & ~1;
        if (!calc_crc(0, frame_buf,  frame_size1))
        {
          // error/false sync
          errors++;
          frame_data--;
          memmove(frame_buf, frame_buf + 1, frame_data);
          required = 128;
          state = state_sync;
          continue;
        }

        ///////////////////////////////////////////////////////
        // Start decoding

        level = 1.0;
        if (a52_frame(a52_state, frame_buf, &mode, &level, 0.0))
        {
          // error (drop frame)
          errors++;
          frame_data -= frame_size;
          memmove(frame_buf, frame_buf + frame_size, frame_data);
          required = 128;
          state = state_sync;
          continue;
        }

        frames++;
        state = state_decode;
        spk = a52_to_spk(mode, sample_rate);

        return frame_size;
      } // case state_data:
    } // switch(state)
  } // while (1)
}


bool
A52Parser::decode_frame()
{
  int ch, b;

  for (b = 0; b < 6; b++)
  {
    if (a52_block(a52_state))
      return false;

    int nch = spk.nch();

    if (spk.lfe())
    {
      memcpy(samples[nch - 1] + b * 256, a52_samples(a52_state), sizeof(sample_t) * 256);
      for (ch = 0; ch < nch - 1; ch++)
        memcpy(samples[ch] + b * 256, a52_samples(a52_state) + (ch + 1) * 256, sizeof(sample_t) * 256);
    }
    else
      for (ch = 0; ch < nch; ch++)
        memcpy(samples[ch] + b * 256, a52_samples(a52_state) + ch * 256, sizeof(sample_t) * 256);
  }

  return true;
}
