#include "parser.h"

void 
BaseParser::reset()
{
  // drop data in frame buffer
  frame_data = 0;
  new_frame_size = 0;

  // samples.zero()?
}

size_t
BaseParser::load_frame(uint8_t **buf, uint8_t *end)
{
  // Drop old frame if loaded
  if (frame_size && (frame_data >= frame_size))
  {
    frame_data = 0;
    new_frame_size = 0;
  }

  while (*buf < end) 
  {
    ///////////////////////////////////////////////////////
    // Sync

    if (!new_frame_size)
    {
      new_frame_size = sync(buf, end);
      continue;
    }

    ///////////////////////////////////////////////////////
    // Load frame

    if (end - *buf < signed(new_frame_size - frame_data))
    {
      memcpy(frame + frame_data, *buf, end - *buf);
      frame_data += end - *buf;
      *buf = end;
      return 0;
    }
    else
    {
      memcpy(frame + frame_data, *buf, new_frame_size - frame_data);
      *buf += new_frame_size - frame_data;
      frame_data = new_frame_size;
    }

    ///////////////////////////////////////////////////////
    // Update stream info and prepare to decode frame

    if (!start_decode())
    {
      // error (drop frame)
      errors++;
      frame_data = 0;
      new_frame_size = 0;
      continue;
    }

    frames++;
    return frame_size = new_frame_size;
  } // while (*buf < end)

  return 0;
}
