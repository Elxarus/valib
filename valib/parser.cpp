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
      info_size += sprintf(info + info_size, "Bitrate: unknown\n");

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
  frames = 0;

  parser = 0;
  header_size = 0;
  min_frame_size = 0;
  max_frame_size = 0;

  header = 0;
  frame = 0;

  reset();
}

StreamBuffer::StreamBuffer(const HeaderParser *_parser)
{
  parser = 0;
  header_size = 0;
  min_frame_size = 0;
  max_frame_size = 0;
  frames = 0;

  header = 0;
  frame = 0;

  reset();
  set_parser(_parser);
}

StreamBuffer::~StreamBuffer()
{}

bool 
StreamBuffer::set_parser(const HeaderParser *_parser)
{
  release_parser();

  if (!_parser) 
    return false;

  if  (!buf.allocate(_parser->max_frame_size() * 2 + _parser->header_size() * 2))
    return false;

  parser         = _parser;
  header_size    = parser->header_size();
  min_frame_size = parser->min_frame_size();
  max_frame_size = parser->max_frame_size();
  hdr.drop();

  header = buf.get_data();
  frame = buf.get_data() + header_size;

  in_sync = false;
  new_stream = false;
  frame_loaded = false;

  frame_data = 0;
  frame_size = 0;
  frame_interval = 0;

  return true;
}

void 
StreamBuffer::release_parser()
{
  reset();

  parser = 0;
  header_size = 0;
  min_frame_size = 0;
  max_frame_size = 0;

  header = 0;
  frame = 0;
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
  frames = 0;
}

#define LOAD(required_size)                           \
if (frame_data < required_size)                       \
{                                                     \
  int load_size = required_size - frame_data;         \
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
    ///////////////////////////////////////////////////////////////////////////
    // Search 1st syncpoint

    if (frame_data + (end - *data) < header_size)
    {
      size_t load_size = end - *data;
      memcpy(frame + frame_data, *data, load_size);
      frame_data += load_size;
      *data += load_size;
      return false;
    }
    else if (frame_data < max_frame_size * 2)
    {
      size_t load_size = MIN(max_frame_size * 2 - frame_data, size_t(end - *data));
      memcpy(frame + frame_data, *data, load_size);
      frame_data += load_size;
      *data += load_size;
    }

    for (size_t i = 0; i <= frame_data - header_size; i++)
      if (parser->parse_header(frame + i, &hdr))
        break;

    DROP(i);
    if (frame_data < header_size)
      continue;
 
    ///////////////////////////////////////////////////////////////////////////
    // Search 2nd syncpoint
    // * If frame size can be determined from the header we must search next
    //   syncpoint after the end of the frame but not further than scan size
    //   (only if scan size if specified, do not scan at all otherwise).
    // * If frame size is unknown from the header we must search next syncpoint
    //   after the minimum frame size and the scan size (or maximum frame size
    //   if scan interval is not specified).

    uint8_t *frame2;
    uint8_t *frame2_max;

    if (hdr.frame_size)
    {
      frame2 = frame + hdr.frame_size;
      frame2_max = frame + MAX(hdr.scan_size, hdr.frame_size);
    }
    else
    {
      frame2 = frame + min_frame_size;
      frame2_max = frame + (hdr.scan_size? hdr.scan_size: max_frame_size);
    }

    // Search second synpoint. If we have found correct header then try to
    // locate 3rd syncpoint. If we cannot find 3rd syncpoint we must continue
    // 2nd syncpoint scanning because frame data may contain something that
    // looks like correct frame header but it is not.

    while (frame2 <= frame2_max)
    {
      LOAD(frame2 - frame + header_size);
      if (!parser->compare_headers(frame, frame2) || !parser->parse_header(frame2, &hdr))
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
        frame3_max = frame2 + MAX(hdr.scan_size, hdr.frame_size);
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
        if (!parser->compare_headers(frame2, frame3) || !parser->parse_header(frame3, &hdr))
        {
          frame3++;
          continue;
        }

        ///////////////////////////////////////////////////////////////////////
        // DONE! Prepare first frame output.

        in_sync = true;
        new_stream = true;
        frame_loaded = true;

        memcpy(header, frame, header_size);
        parser->parse_header(header, &hdr);
        frame_interval = frame2 - frame;
        frame_size = hdr.frame_size? hdr.frame_size: frame_interval;

        frames++;
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
  if (!parser)
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

  uint8_t *frame2;
  uint8_t *frame2_max;

  // Skip first frame data and set maximum scan range

  if (hdr.frame_size)
  {
    frame2 = frame + hdr.frame_size;
    frame2_max = frame + MAX(hdr.scan_size, hdr.frame_size);
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
    if (!parser->compare_headers(header, frame2) || !parser->parse_header(frame2, &hdr))
    {
      frame2++;
      continue;
    }

    ///////////////////////////////////////////////////////////////////////////
    // Load the rest of the frame

    frame_interval = frame2 - frame;
    frame_size = hdr.frame_size? hdr.frame_size: frame_interval;
    LOAD(frame2 - frame + frame_size);

    ///////////////////////////////////////////////////////////////////////////
    // DONE! Drop old frame and prepare new frame output.

    DROP(frame_interval);
    memcpy(header, frame, header_size);
    frame_loaded = true;

    frames++;
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

  info_size += parser->header_info(frame, info, sizeof(info));
  info_size += sprintf(info + info_size, "Frame interval: %i\n", frame_interval);
  if (frame_interval > 0 && hdr.nsamples > 0)
    info_size += sprintf(info + info_size, "Actual bitrate: %ikbps\n", frame_interval * hdr.spk.sample_rate * 8 / hdr.nsamples / 1000);

  if (info_size > size) info_size = size;
  memcpy(buf, info, info_size);
  buf[info_size] = 0;
  return info_size;
};
