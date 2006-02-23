#include "parser.h"

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
    // Load frame data and prepare for decoding
    // todo: crc check here

    LOAD(frame_size);
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
