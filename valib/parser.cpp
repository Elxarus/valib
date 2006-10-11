#include <stdio.h>
#include "parser.h"

///////////////////////////////////////////////////////////////////////////////
// HeaderParser
///////////////////////////////////////////////////////////////////////////////

size_t
HeaderParser::header_info(const uint8_t *hdr, char *buf, size_t size) const
{
  char info[1024];
  HeaderInfo h;
  size_t info_size = 0;

  if (parse_header(hdr, &h))
  {
    info_size += sprintf(info + info_size, "Stream format: %s %s %iHz\n", h.spk.format_text(), h.spk.mode_text(), h.spk.sample_rate);

    switch (h.bs_type)
    {
      case BITSTREAM_8:    info_size += sprintf(info + info_size, "Bitstream type: byte stream\n"); break;
      case BITSTREAM_16BE: info_size += sprintf(info + info_size, "Bitstream type: 16bit big endian\n"); break;
      case BITSTREAM_16LE: info_size += sprintf(info + info_size, "Bitstream type: 16bit low endian\n"); break;
      case BITSTREAM_14BE: info_size += sprintf(info + info_size, "Bitstream type: 14bit big endian\n"); break;
      case BITSTREAM_14LE: info_size += sprintf(info + info_size, "Bitstream type: 14bit low endian\n"); break;
      default:        info_size += sprintf(info + info_size, "Bitstream type: unknown\n"); break;
    }

    if (h.frame_size)
      info_size += sprintf(info + info_size, "Frame size: %i\n", h.frame_size);
    else
      info_size += sprintf(info + info_size, "Frame size: free format\n");

    info_size += sprintf(info + info_size, "Samples: %i\n", h.nsamples);

    if (h.frame_size > 0 && h.nsamples > 0)
      info_size += sprintf(info + info_size, "Bitrate: %ikbps\n", h.frame_size * h.spk.sample_rate * 8 / h.nsamples / 1000);
    else
      info_size += sprintf(info + info_size, "Bitrate: unknown");

    if (h.spdif_type)
      info_size += sprintf(info + info_size, "SPDIF stream type: 0x%x\n", h.spdif_type);
  }
  else
  {
    info_size += sprintf(info + info_size, "No header found\n");
  }

  if (info_size > size) info_size = size;
  memcpy(buf, info, info_size);
  buf[info_size] = 0;
  return info_size;
};

///////////////////////////////////////////////////////////////////////////////
// StreamBuffer
///////////////////////////////////////////////////////////////////////////////

StreamBuffer::StreamBuffer()
{
  hparser = 0;
  frame = 0;
  reset();
}

StreamBuffer::StreamBuffer(const HeaderParser *_hparser)
{
  hparser = 0;
  frame = 0;
  reset();

  set_hparser(_hparser);
}

StreamBuffer::~StreamBuffer()
{}

bool 
StreamBuffer::set_hparser(const HeaderParser *_hparser)
{
  if (!_hparser) return false;
  if  (!buf.allocate(_hparser->max_frame_size() * 2 + _hparser->header_size()))
    return false;

  hparser        = _hparser;
  header_size    = hparser->header_size();
  min_frame_size = hparser->min_frame_size();
  max_frame_size = hparser->max_frame_size();
  hdr.drop();

  frame = buf.get_data();

  in_sync = false;
  new_stream = false;
  frame_loaded = false;

  frame_data = 0;
  frame_size = 0;
  frame_interval = 0;

  return true;
}

void 
StreamBuffer::reset()
{
  hdr.drop();

  in_sync = false;
  new_stream = false;
  frame_loaded = false;

  frame_data = 0;
  frame_size = 0;
  frame_interval = 0;
}

#define LOAD(required_size)                           \
if (frame_data < required_size)                       \
{                                                     \
  size_t load_size = required_size - frame_data;      \
  if (*data + load_size > end)                        \
  {                                                   \
    load_size = end - *data;                          \
    memcpy(frame + frame_data, *data, load_size);     \
    frame_data += load_size;                          \
    *data += load_size;                               \
    return false;                                     \
  }                                                   \
  else                                                \
  {                                                   \
    memcpy(frame + frame_data, *data, load_size);     \
    frame_data += load_size;                          \
    *data += load_size;                               \
  }                                                   \
}

#define DROP(drop_size)                               \
{                                                     \
  frame_data -= drop_size;                            \
  memmove(frame, frame + drop_size, frame_data);      \
}



bool
StreamBuffer::sync(uint8_t **data, uint8_t *end)
{
  while (1)
  {
    LOAD(header_size);
    if (!hparser->parse_header(frame, &hdr))
    {
      DROP(1);
      continue;
    }
 
    ///////////////////////////////////////////////////////////////////////////
    // Find 2nd syncpoint
    // * If frame size can be determined from header we must search next
    //   syncpoint after the end of the frame but not further than maximum
    //   frame size.
    // * If frame size is unknown from the header info we must search next
    //   syncpoint in between of minimum and maximum frame sizes.

    uint8_t *frame2     = frame;
    uint8_t *frame2_max = frame + max_frame_size;

    // Skip first frame data (if we know the frame size) or minimum frame size

    if (hdr.frame_size)
      frame2 += hdr.frame_size;
    else
      frame2 += min_frame_size;

    // Scan second synpoint position. If we have found correct header then
    // try to locate 3rd syncpoint. If we cannot find 3rd syncpoint we must
    // continue 2nd syncpoint scanning because frame data may contain something
    // that looks like correct frame header but it is not.

    while (frame2 <= frame2_max)
    {
      LOAD(frame2 - frame + header_size);
      if (!hparser->compare_headers(frame, frame2) || !hparser->parse_header(frame2, &hdr))
      {
        frame2++;
        continue;
      }

      // Now we can set frame interval. It is constant for entire stream for
      // unknown frame size and will be used for for frame loading. For known
      // frame size it will change with each frame load.

      frame_interval = frame2 - frame;

      /////////////////////////////////////////////////////////////////////////
      // Find 3rd syncpoint
      // * For unknown frame size we must locate next syncpoint exactly at next
      //   frame interval.
      // * For known frame size we must search next syncpoint in between of
      //   minimum and maximum frame sizes.

      uint8_t *frame3;
      uint8_t *frame3_max;

      // Skip 2nd frame data and set maximum scan range

      if (hdr.frame_size)
      {
        frame3     = frame2 + hdr.frame_size;
        frame3_max = frame2 + max_frame_size;
      }
      else
      {
        frame3     = frame2 + frame_interval;
        frame3_max = frame2 + frame_interval;
      }

      // Scan third synpoint position. If we have found correct header then
      // task is done. If we cannot find 3rd syncpoint we must continue 2nd
      // syncpoint scanning.

      while (frame3 <= frame3_max)
      {
        LOAD(frame3 - frame + header_size);
        if (!hparser->compare_headers(frame2, frame3) || !hparser->parse_header(frame3, &hdr))
        {
          frame3++;
          continue;
        }

        ///////////////////////////////////////////////////////////////////////
        // DONE! Prepare first frame output.

        in_sync = true;
        new_stream = true;
        frame_loaded = true;

        hparser->parse_header(frame, &hdr);
        frame_interval = frame2 - frame;
        frame_size = hdr.frame_size? hdr.frame_size: frame_interval;

        return true;
      }

      // No correct 3rd syncpoint found.
      // Continue 2nd syncpoint scanning.

      frame2++;
    }

    // No correct 2nd syncpoint found.
    // Resync first frame.

    DROP(1);
  }
}


bool 
StreamBuffer::load_frame(uint8_t **data, uint8_t *end)
{
  if (!hparser)
    return false;

  /////////////////////////////////////////////////////////////////////////////
  // Syncronize with a new stream if we're not in sync

  if (!in_sync)
    return sync(data, end);

  /////////////////////////////////////////////////////////////////////////////
  // Drop current frame and load next one
  //
  // When we're in sync we always have one frame loaded at frame buffer. To
  // load next frame we load next frame after current one, compare headers and
  // drop first frame. (We need to compare headers to detect stream changes).

  new_stream = false;
  frame_loaded = false;

  uint8_t *frame2     = frame;
  uint8_t *frame2_max = frame2 + max_frame_size;

  // Skip first frame data and set maximum scan range

  if (hdr.frame_size)
  {
    frame2     = frame + hdr.frame_size;
    frame2_max = frame + max_frame_size;
  }
  else
  {
    frame2     = frame + frame_interval;
    frame2_max = frame + frame_interval;
  }

  // Scan

  while (frame2 <= frame2_max)
  {
    LOAD(frame2 - frame + header_size);
    if (!hparser->compare_headers(frame, frame2) || !hparser->parse_header(frame2, &hdr))
    {
      frame2++;
      continue;
    }

    ///////////////////////////////////////////////////////////////////////////
    // DONE! Drop old frame and prepare new frame output.

    frame_loaded = true;
    frame_interval = frame2 - frame;
    frame_size = hdr.frame_size? hdr.frame_size: frame_interval;
    DROP(frame_interval);

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////
  // No correct syncpoint found. Resync.

  DROP(frame_size);
  in_sync = false;
  return sync(data, end);
}

size_t
StreamBuffer::stream_info(char *buf, size_t size) const
{
  char info[1024];
  size_t info_size = 0;

  info_size += hparser->header_info(frame, info, sizeof(info));
  info_size += sprintf(info + info_size, "Frame interval: %i\n", frame_interval);
  if (frame_interval > 0 && hdr.nsamples > 0)
    info_size += sprintf(info + info_size, "Actual bitrate: %ikbps\n", frame_interval * hdr.spk.sample_rate * 8 / hdr.nsamples / 1000);

  if (info_size > size) info_size = size;
  memcpy(buf, info, info_size);
  buf[info_size] = 0;
  return info_size;
};






size_t
BaseParser::load_frame(uint8_t **buf, uint8_t *end)
{
  // Drop old frame if loaded
  if (is_frame_loaded())
    drop_frame();

  uint8_t *frame_buf = frame.get_data();
  #define LOAD(amount)                  \
  if (frame_data < (amount))            \
  {                                     \
    size_t len = (amount) - frame_data; \
    if (size_t(end - *buf) < len)       \
    {                                   \
      /* have no enough data */         \
      memcpy(frame_buf + frame_data, *buf, end - *buf); \
      frame_data += end - *buf;         \
      *buf += end - *buf;               \
      return 0;                         \
    }                                   \
    else                                \
    {                                   \
      memcpy(frame_buf + frame_data, *buf, len); \
      frame_data += len;                \
      *buf += len;                      \
    }                                   \
  }

  while (1) 
  {
    ///////////////////////////////////////////////////////
    // Sync 
    // * We may have some data in frame buffer. Therefore 
    //   we must scan frame buffer first.
    // * We may have no data in frame buffer. So we must
    //   load 4 bytes to make scanner to work with frame
    //   buffer as scan buffer.

    LOAD(4);
    if (!scanner.get_sync(frame_buf))
    {
      size_t gone = scanner.scan(frame_buf, frame_buf + 4, frame_data - 4);
      frame_data -= gone;
      memmove(frame_buf + 4, frame_buf + 4 + gone, frame_data);

      if (!scanner.get_sync(frame_buf))
      {
        gone = scanner.scan(frame_buf, *buf, end - *buf);
        *buf += gone;
        if (!scanner.get_sync(frame_buf))
          return 0;
      }
    }

    ///////////////////////////////////////////////////////
    // Load header, parse it, and fill stream info

    LOAD(header_size())
    if (!load_header(frame_buf))
    {
      // resync (possibly false sync)
      frame_data--;
      memmove(frame_buf, frame_buf + 1, frame_data);
      continue;
    }

    ///////////////////////////////////////////////////////
    // Fool protection

    if (frame_size > frame.get_size())
    {
      // resync
      // (frame is too big and we cannot load it...)
      frame_data--;
      memmove(frame_buf, frame_buf + 1, frame_data);
      errors++; // inform about error
      continue;
    }

    ///////////////////////////////////////////////////////
    // Load frame data and do CRC check

    LOAD(frame_size);
    if (do_crc)
      if (!crc_check())
      {
        // resync on crc check fail
        frame_data--;
        memmove(frame_buf, frame_buf + 1, frame_data);
        errors++; // inform about error
        continue;
      }

    ///////////////////////////////////////////////////////
    // Prepare to decode

    if (!prepare())
    {
      // resync (possibly false sync)
      frame_data--;
      memmove(frame_buf, frame_buf + 1, frame_data);
      continue;
    }

    frames++;
    return frame_size;
  } // while (*buf < end)

  return 0;
}
