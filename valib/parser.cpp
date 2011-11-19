#include <sstream>
#include "parser.h"

using std::stringstream;
using std::endl;

///////////////////////////////////////////////////////////////////////////////
// BasicFrameParser
///////////////////////////////////////////////////////////////////////////////

bool
BasicFrameParser::first_frame(const uint8_t *frame, size_t size)
{
  FrameInfo new_finfo;

  reset();
  if (!parse_header(frame, &new_finfo))
    return false;

  if (size != new_finfo.frame_size)
    return false;

  header.allocate(header_size());
  memcpy(header, frame, header_size());

  finfo = new_finfo;
  sinfo = build_syncinfo(frame, size, finfo);
  return true;
}

bool
BasicFrameParser::next_frame(const uint8_t *frame, size_t size)
{
  FrameInfo new_finfo;

  if (!compare_headers(header, frame))
    return false;

  if (!parse_header(frame, &new_finfo))
    return false;

  if (size != new_finfo.frame_size)
    return false;

  finfo = new_finfo;
  return true;
}

void
BasicFrameParser::reset()
{
  header.zero();
  finfo = FrameInfo();
  sinfo = sync_info();
}

string
BasicFrameParser::stream_info() const
{
  if (!in_sync())
    return string("No sync\n");

  FrameInfo i = frame_info();
  std::stringstream result;

  result << "Stream format: " << i.spk.print() << endl;

  switch (i.bs_type)
  {
    case BITSTREAM_8:    result << "Bitstream type: byte stream" << endl; break;
    case BITSTREAM_16BE: result << "Bitstream type: 16bit big endian" << endl; break;
    case BITSTREAM_16LE: result << "Bitstream type: 16bit low endian" << endl; break;
    case BITSTREAM_14BE: result << "Bitstream type: 14bit big endian" << endl; break;
    case BITSTREAM_14LE: result << "Bitstream type: 14bit low endian" << endl; break;
    default:             result << "Bitstream type: unknown" << endl; break;
  }

  result << "Frame size: " << i.frame_size << endl;
  result << "Samples: " << i.nsamples << endl;
  result << "Bitrate: " << int(i.bitrate() / 1000) << "kbps" << endl;

  if (i.spdif_type)
    result << "SPDIF stream type: 0x" << std::hex << i.spdif_type << endl;

  return result.str();
};

///////////////////////////////////////////////////////////////////////////////
// HeaderParser
///////////////////////////////////////////////////////////////////////////////

string
HeaderParser::header_info(const uint8_t *hdr) const
{
  HeaderInfo h;
  if (!parse_header(hdr, &h))
    return string("No header found\n");

  std::stringstream result;

  result << "Stream format: " << h.spk.print() << endl;

  switch (h.bs_type)
  {
    case BITSTREAM_8:    result << "Bitstream type: byte stream" << endl; break;
    case BITSTREAM_16BE: result << "Bitstream type: 16bit big endian" << endl; break;
    case BITSTREAM_16LE: result << "Bitstream type: 16bit low endian" << endl; break;
    case BITSTREAM_14BE: result << "Bitstream type: 14bit big endian" << endl; break;
    case BITSTREAM_14LE: result << "Bitstream type: 14bit low endian" << endl; break;
    default:             result << "Bitstream type: unknown" << endl; break;
  }

  if (h.frame_size)
    result << "Frame size: " << h.frame_size << endl;
  else
    result << "Frame size: free format" << endl;

  result << "Samples: " << h.nsamples << endl;

  if (h.frame_size > 0 && h.nsamples > 0)
    result << "Bitrate: " << int(h.frame_size * h.spk.sample_rate * 8 / h.nsamples / 1000) << "kbps" << endl;
  else
    result << "Bitrate: unknown" << endl;

  if (h.spdif_type)
    result << "SPDIF stream type: 0x" << std::hex << h.spdif_type << endl;

  return result.str();
};

///////////////////////////////////////////////////////////////////////////////
// StreamBuffer
///////////////////////////////////////////////////////////////////////////////

StreamBuffer::StreamBuffer()
{
  parser = 0;

  sync_buf = 0;
  sync_size = 0;
  sync_data = 0;

  debris = 0;
  debris_size = 0;

  frame = 0;
  frame_size = 0;

  in_sync = false;
  new_stream = false;

  frames = 0;
}

StreamBuffer::StreamBuffer(FrameParser *parser_)
{
  parser = 0;

  sync_buf = 0;
  sync_size = 0;
  sync_data = 0;

  debris = 0;
  debris_size = 0;

  frame = 0;
  frame_size = 0;

  in_sync = false;
  new_stream = false;

  frames = 0;

  set_parser(parser_);
}

StreamBuffer::~StreamBuffer()
{}

void
StreamBuffer::set_parser(FrameParser *new_parser)
{
  release_parser();
  if (!new_parser)
    return;

  if (!new_parser->sync_info().is_good())
    return;

  parser = new_parser;
  sinfo = parser->sync_info();
  scan.set_trie(sinfo.sync_trie);

  buf.allocate(sinfo.max_frame_size * 3 + parser->header_size());
  sync_buf  = buf.begin();
  sync_size = buf.size();

  reset();
}

void 
StreamBuffer::release_parser()
{
  parser = 0;
  finfo = FrameInfo();
  sinfo = SyncInfo();
  scan.set_trie(SyncTrie());

  sync_buf = 0;
  sync_size = 0;
  sync_data = 0;
  pre_frame = 0;

  debris = 0;
  debris_size = 0;

  frame = 0;
  frame_size = 0;

  in_sync = false;
  new_stream = false;
}


void 
StreamBuffer::reset()
{
  resync();
  sync_data = 0;
}

bool
StreamBuffer::load_buffer(uint8_t **data, uint8_t *end, size_t required_size)
{
  if (required_size <= sync_data)
    return true;

  if (sync_data + (end - *data) < required_size)
  {
    size_t load_size = end - *data;
    memcpy(sync_buf + sync_data, *data, load_size);
    sync_data += load_size;
    *data += load_size;
    return false;
  }

  size_t load_size = required_size - sync_data;
  memcpy(sync_buf + sync_data, *data, load_size);
  sync_data += load_size;
  *data += load_size;
  return true;
}

void
StreamBuffer::drop_buffer(size_t size)
{
  assert(sync_data >= size);
  sync_data -= size;
  memmove(sync_buf, sync_buf + size, sync_data);
}

#define LOAD(required_size) if (!load_buffer(data, end, required_size)) return false;
#define DROP(size) drop_buffer(size);

///////////////////////////////////////////////////////////////////////////////
// When we sync on a new stream we may start syncing in between of two
// syncpoints. The data up to the first syncpoint may be wrongly interpreted
// as PCM data and poping sound may occur before the first frame. To prevent
// this we must consider this part as debris before the first syncpoint.
//
// But amount of this data may not be larger than the maximum frame size:
//
// Case 1: The data before the first frame is a part of the stream
// |--------------+------------------------+---
// | < frame_size |          frame1        |  ....
// |--------------+------------------------+---
//
// Case 2: The data before the first frame is not a part of the stream
// |----------------------------+------------------------+---
// |       > frame_size         |          frame1        |  ....
// |----------------------------+------------------------+---
//
// Therefore if we have not found the syncpoint at first frame_size bytes,
// all this data does not belong to a stream.

void
StreamBuffer::resync()
{
  finfo = FrameInfo();
  
  if (parser)
    parser->reset();

  pre_frame = sinfo.max_frame_size;

  frame = 0;
  frame_size = 0;

  debris = 0;
  debris_size = 0;

  in_sync = false;
  new_stream = false;
}

bool
StreamBuffer::sync(uint8_t **data, uint8_t *end)
{
  assert(*data <= end);
  assert(!in_sync && !new_stream);
  assert(frame == 0 && frame_size == 0);

  /////////////////////////////////////////////////////////////////////////////
  // Drop debris

  DROP(debris_size);
  debris = 0;
  debris_size = 0;

  /////////////////////////////////////////////////////////////////////////////
  // Cache data (as much as possible)

  if (sync_data < sync_size)
  {
    size_t load_size = MIN(size_t(end - *data), (sync_size - sync_data));
    memcpy(sync_buf + sync_data, *data, load_size);
    sync_data += load_size;
    *data += load_size;
  }

  /////////////////////////////////////////////////////////////////////////////
  // Search 1st syncpoint

  uint8_t *pos1, *pos2, *pos3;
  uint8_t *pos1_max, *pos2_max, *pos3_max;

  pos1 = sync_buf;
  pos1_max = sync_buf + pre_frame;

  size_t header_size = parser->header_size();
  size_t sync_size = sinfo.sync_trie.sync_size();
  while (pos1 <= pos1_max)
  {
    if (sync_buf + sync_data < pos1 + header_size)
      return false;

    if (!parser->parse_header(pos1))
    {
      pos1++;
      continue;
    }

    ///////////////////////////////////////////////////////////////////////////
    // Search 2nd syncpoint

    pos2 = pos1 + sinfo.min_frame_size;
    pos2_max = pos1 + sinfo.max_frame_size;

    while (pos2 <= pos2_max)
    {
      if (sync_buf + sync_data < pos2 + header_size)
        return false;

      if (!parser->parse_header(pos2))
      {
        pos2++;
        continue;
      }
      if (!parser->compare_headers(pos1, pos2))
      {
        pos2++;
        continue;
      }
      if (!parser->first_frame(pos1, pos2 - pos1))
      {
        pos2++;
        continue;
      }

      /////////////////////////////////////////////////////////////////////////
      // Search 3rd syncpoint

      pos3 = pos2 + sinfo.min_frame_size;
      pos3_max = pos2 + sinfo.max_frame_size;

      while (pos3 <= pos3_max)
      {
        if (sync_buf + sync_data < pos3 + header_size)
          return false;

        if (!parser->parse_header(pos3) ||
            !parser->compare_headers(pos2, pos3) ||
            !parser->next_frame(pos2, pos3 - pos2))
        {
          pos3++;
          continue;
        }

        ///////////////////////////////////////////////////////////////////////
        // DONE! Prepare first frame output.

        parser->first_frame(pos1, pos2 - pos1);
        finfo = parser->frame_info();

        debris = sync_buf;
        debris_size = pos1 - sync_buf;

        frame = pos1;
        frame_size = pos2 - pos1;

        in_sync = true;
        new_stream = true;

        frames++;
        return true;
      }

      /////////////////////////////////////////////////////////////////////////
      // No correct 3rd syncpoint found.
      // Continue 2nd syncpoint scanning.

      pos2++;

    } // while (pos2 <= pos2_max)

    /////////////////////////////////////////////////////////////////////////
    // No correct sync sequence found.
    /////////////////////////////////////////////////////////////////////////

    pos1++;

  } // while (pos1 <= pos1_max)

  /////////////////////////////////////////////////////////////////////////////
  // Return debris
  // * Sync buffer does not start with a syncpoint
  // * No correct sync sequence found (false sync)
  //
  // Try to locate next syncpoint and return data up to the poistion found
  // as debris.

  size_t pos = pos1 - sync_buf;
  size_t scan_size = sync_data;
  if (header_size > sync_size)
    scan_size -= header_size - sync_size;

  while (scan.scan_pos(sync_buf, scan_size, pos))
  {
    if (parser->parse_header(sync_buf + pos))
      break;
    pos++;
  }

  pre_frame = 0;
  debris = sync_buf;
  debris_size = pos;
  return true;
}


bool 
StreamBuffer::load(uint8_t **data, uint8_t *end)
{
  if (!parser)
  {
    *data = end; // avoid endless loop
    return false;
  }

  /////////////////////////////////////////////////////////////////////////////
  // Syncronize with a new stream if we're not in sync

  if (!in_sync)
    return sync(data, end);

  /////////////////////////////////////////////////////////////////////////////
  // Drop old debris and frame data

  if (frame_size || debris_size)
  {
    DROP(debris_size + frame_size);
    debris_size = 0;
    frame_size = 0;
  }

  new_stream = false;

  /////////////////////////////////////////////////////////////////////////////
  // Cache data (as much as possible)

  if (sync_data < sync_size)
  {
    size_t load_size = MIN(size_t(end - *data), (sync_size - sync_data));
    memcpy(sync_buf + sync_data, *data, load_size);
    sync_data += load_size;
    *data += load_size;
  }

  /////////////////////////////////////////////////////////////////////////////
  // Load next frame

  size_t header_size = parser->header_size();
  size_t sync_size = sinfo.sync_trie.sync_size();

  size_t pos = sinfo.min_frame_size;
  size_t scan_size = sinfo.max_frame_size + sync_size;
  if (sync_data < scan_size)
    scan_size = sync_data;

  while (scan.scan_pos(sync_buf, scan_size, pos))
  {
    if (sync_data < pos + header_size)
      return false;

    if (parser->next_frame(sync_buf, pos))
    {
      finfo = parser->frame_info();
      frame = sync_buf;
      frame_size = pos;
      return true;
    }
    pos++;
  }

  if (pos < sinfo.max_frame_size)
    return false;

  /////////////////////////////////////////////////////////////////////////////
  // No correct syncpoint found. Resync.

  resync();
  return sync(data, end);
}

bool 
StreamBuffer::load_frame(uint8_t **data, uint8_t *end)
{
  while (*data < end || has_frame() || has_debris())
  {
    load(data, end);
    if (has_frame())
      return true;
  }
  return false;
}

bool
StreamBuffer::flush()
{
  if (!sync_data)
    return false;

  DROP(debris_size + frame_size);

  debris = 0;
  debris_size = 0;

  frame = 0;
  frame_size = 0;

  if (sync_data)
  {
    uint8_t *pos = 0;
    if (load(&pos, 0))
      return true;
  }

  // Last frame?
  if (sync_data > parser->header_size())
    if (parser->next_frame(sync_buf, sync_data))
    {
      frame = sync_buf;
      frame_size = sync_data;
      return true;
    }

  if (sync_data)
  {
    debris = sync_buf;
    debris_size = sync_data;
    return true;
  }

  resync();
  return false;
}

string
StreamBuffer::stream_info() const
{
  if (!parser)
    return "No parser set";

  if (!in_sync)
    return "No sync";

  return parser->stream_info();
}
