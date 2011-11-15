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
  if (!parse_header(frame, &finfo))
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
  header_size = 0;
  min_frame_size = 0;
  max_frame_size = 0;

  header_buf = 0;
  hinfo.clear();

  sync_buf = 0;
  sync_size = 0;
  sync_data = 0;

  debris = 0;
  debris_size = 0;

  frame = 0;
  frame_size = 0;
  frame_interval = 0;

  in_sync = false;
  new_stream = false;

  frames = 0;
}

StreamBuffer::StreamBuffer(const HeaderParser *_parser)
{
  parser = 0;
  header_size = 0;
  min_frame_size = 0;
  max_frame_size = 0;

  header_buf = 0;
  hinfo.clear();

  sync_buf = 0;
  sync_size = 0;
  sync_data = 0;

  debris = 0;
  debris_size = 0;

  frame = 0;
  frame_size = 0;
  frame_interval = 0;

  in_sync = false;
  new_stream = false;

  frames = 0;

  set_parser(_parser);
}

StreamBuffer::~StreamBuffer()
{}

void
StreamBuffer::set_parser(const HeaderParser *new_parser)
{
  release_parser();

  // Check parser correctness
  if (!new_parser ||
      new_parser->max_frame_size() == 0 ||
      new_parser->header_size() == 0)
    return;

  scan.set_trie(new_parser->sync_trie());
  buf.allocate(new_parser->max_frame_size() * 3 + new_parser->header_size() * 2);

  parser         = new_parser;
  header_size    = parser->header_size();
  min_frame_size = parser->min_frame_size();
  max_frame_size = parser->max_frame_size();

  header_buf = buf.begin();
  sync_buf   = buf.begin() + header_size;
  sync_size  = max_frame_size * 3 + header_size;

  reset();
}

void 
StreamBuffer::release_parser()
{
  scan.set_trie(SyncTrie());
  parser = 0;
  header_size = 0;
  min_frame_size = 0;
  max_frame_size = 0;

  header_buf = 0;
  sync_buf = 0;
  sync_size = 0;

  reset();
}


void 
StreamBuffer::reset()
{
  hinfo.clear();
  sync_data = 0;
  pre_frame = max_frame_size;

  debris = 0;
  debris_size = 0;

  frame = 0;
  frame_size = 0;
  frame_interval = 0;

  in_sync = false;
  new_stream = false;
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

bool
StreamBuffer::sync(uint8_t **data, uint8_t *end)
{
  assert(*data <= end);
  assert(!in_sync && !new_stream);
  assert(frame == 0 && frame_size == 0 && frame_interval == 0);

  /////////////////////////////////////////////////////////////////////////////
  // Drop debris

  DROP(debris_size);
  debris = 0;
  debris_size = 0;

  /////////////////////////////////////////////////////////////////////////////
  // Cache data

  if (sync_data < sync_size)
  {
    size_t load_size = MIN(size_t(end - *data), (sync_size - sync_data));
    memcpy(sync_buf + sync_data, *data, load_size);
    sync_data += load_size;
    *data += load_size;
  }

  /////////////////////////////////////////////////////////////////////////////
  // Search 1st syncpoint

  HeaderInfo hinfo1, hinfo2;
  uint8_t *frame1, *frame2, *frame3;
  uint8_t *frame1_max, *frame2_max, *frame3_max;

  frame1 = sync_buf;
  frame1_max = sync_buf + pre_frame;

  while (frame1 <= frame1_max)
  {
    LOAD(frame1 - sync_buf + header_size);
    if (parser->parse_header(frame1, &hinfo1))
    {
      ///////////////////////////////////////////////////////////////////////////
      // Search 2nd syncpoint
      // * Known frame size: search next syncpoint after the end of the frame but
      //   not further than scan size (if defined)
      // * Unknown frame size: search next syncpoint after the minimum frame size
      //   up to scan size or maximum frame size if scan size is unspecified.

      if (hinfo1.frame_size)
      {
        frame2 = frame1 + hinfo1.frame_size;
        frame2_max = frame1 + MAX(hinfo1.scan_size, hinfo1.frame_size);
      }
      else
      {
        frame2 = frame1 + min_frame_size;
        frame2_max = frame1 + (hinfo1.scan_size? hinfo1.scan_size: max_frame_size);
      }

      while (frame2 <= frame2_max)
      {
        LOAD(frame2 - sync_buf + header_size);
        if (!parser->compare_headers(frame1, frame2) || !parser->parse_header(frame2, &hinfo2))
        {
          frame2++;
          continue;
        }

        /////////////////////////////////////////////////////////////////////////
        // Search 3rd syncpoint
        // * Known frame size: search next syncpoint after the end of the frame
        //   but not further than scan size (if defined)
        // * Unknown frame size: expect next syncpoint exactly after frame
        //   interval.

        if (hinfo2.frame_size)
        {
          frame3     = frame2 + hinfo2.frame_size;
          frame3_max = frame2 + MAX(hinfo2.scan_size, hinfo2.frame_size);
        }
        else
        {
          frame3     = frame2 + (frame2 - frame1);
          frame3_max = frame2 + (frame2 - frame1);
        }

        while (frame3 <= frame3_max)
        {
          LOAD(frame3 - sync_buf + header_size);
          if (!parser->compare_headers(frame2, frame3) || !parser->parse_header(frame3))
          {
            frame3++;
            continue;
          }

          ///////////////////////////////////////////////////////////////////////
          // Does the data defore the first frame belong to the stream?
          // If not, send it separately from the first frame

          debris = sync_buf;
          debris_size = frame1 - sync_buf;

          size_t temp_frame_size = hinfo.frame_size? hinfo.frame_size: frame2 - frame1;
          if (debris_size > temp_frame_size)
            return true;

          ///////////////////////////////////////////////////////////////////////
          // DONE! Prepare first frame output.

          memcpy(header_buf, frame1, header_size);
          parser->parse_header(header_buf, &hinfo);

          frame = frame1;
          frame_interval = frame2 - frame1;
          frame_size = hinfo.frame_size? hinfo.frame_size: frame_interval;

          in_sync = true;
          new_stream = true;

          frames++;
          return true;
        }

        /////////////////////////////////////////////////////////////////////////
        // No correct 3rd syncpoint found.
        // Continue 2nd syncpoint scanning.

        frame2++;

      } // while (frame2 <= frame2_max)

      /////////////////////////////////////////////////////////////////////////
      // No correct sync sequence found.
      /////////////////////////////////////////////////////////////////////////

    } // if (parser->parse_header(frame1, &hdr))

    frame1++;

  } // while (frame1 <= frame1_max)

  /////////////////////////////////////////////////////////////////////////////
  // Return debris
  // * Sync buffer does not start with a syncpoint
  // * No correct sync sequence found (false sync)
  //
  // Try to locate next syncpoint and return data up to the poistion found
  // as debris.

  size_t pos = frame1 - sync_buf;
  while (scan.scan_pos(sync_buf, sync_data - header_size, pos))
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
  // Load next frame

  frame = sync_buf;
  uint8_t *frame_max = sync_buf;

  if (hinfo.frame_size)
    if (hinfo.frame_size < hinfo.scan_size)
      frame_max += hinfo.scan_size - hinfo.frame_size;

  while (frame <= frame_max)
  {
    LOAD(frame - sync_buf + header_size);

    HeaderInfo new_hinfo;
    if (!parser->compare_headers(header_buf, frame) || !parser->parse_header(frame, &new_hinfo))
    {
      frame++;
      continue;
    }

    ///////////////////////////////////////////////////////////////////////////
    // Load the rest of the frame

    LOAD(frame - sync_buf + (new_hinfo.frame_size? new_hinfo.frame_size: frame_interval));
      
    ///////////////////////////////////////////////////////////////////////////
    // DONE! Prepare new frame output.

    if (hinfo.frame_size)
      frame_interval = hinfo.frame_size + frame - sync_buf;

    memcpy(header_buf, frame, header_size);
    parser->parse_header(header_buf, &hinfo);

    debris = sync_buf;
    debris_size = frame - sync_buf;

    if (hinfo.frame_size)
      frame_size = hinfo.frame_size;
    else
      frame_size = frame_interval;

    frames++;
    return true;
  }

  /////////////////////////////////////////////////////////////////////////////
  // No correct syncpoint found. Resync.

  in_sync = false;
  pre_frame = max_frame_size;

  frame = 0;
  frame_size = 0;
  frame_interval = 0;
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
  DROP(debris_size + frame_size);

  in_sync = false;
  new_stream = false;
  pre_frame = max_frame_size;

  frame = 0;
  frame_size = 0;
  frame_interval = 0;

  debris = sync_buf;
  debris_size = sync_data;

  return debris_size > 0;
}


string
StreamBuffer::stream_info() const
{
  if (!parser)
    return "No parser set";

  if (!in_sync)
    return "No sync";

  stringstream result;
  result << parser->header_info(frame);
  result << "Frame interval: " << frame_interval << endl;
  if (frame_interval > 0 && hinfo.nsamples > 0)
    result << "Actual bitrate: " << int(frame_interval * hinfo.spk.sample_rate * 8 / hinfo.nsamples / 1000) << "kbps" << endl;

  return result.str();
};


///////////////////////////////////////////////////////////////////////////////
// StreamBuffer2
///////////////////////////////////////////////////////////////////////////////

StreamBuffer2::StreamBuffer2()
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

StreamBuffer2::StreamBuffer2(FrameParser *parser_)
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

StreamBuffer2::~StreamBuffer2()
{}

void
StreamBuffer2::set_parser(FrameParser *new_parser)
{
  release_parser();
  if (!new_parser)
    return;

  parser = new_parser;
  sinfo = parser->sync_info();

  buf.allocate(sinfo.max_frame_size * 3 + parser->header_size());
  sync_buf  = buf.begin();
  sync_size = buf.size();

  reset();
}

void 
StreamBuffer2::release_parser()
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
StreamBuffer2::reset()
{
  resync();
  sync_data = 0;
}

bool
StreamBuffer2::load_buffer(uint8_t **data, uint8_t *end, size_t required_size)
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
StreamBuffer2::drop_buffer(size_t size)
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
StreamBuffer2::resync()
{
  finfo = FrameInfo();
  sinfo = parser->sync_info();
  scan.set_trie(sinfo.sync_trie);

  pre_frame = sinfo.max_frame_size;

  frame = 0;
  frame_size = 0;

  debris = 0;
  debris_size = 0;

  in_sync = false;
  new_stream = false;
}

bool
StreamBuffer2::sync(uint8_t **data, uint8_t *end)
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

      if (!parser->parse_header(pos2) ||
          !parser->compare_headers(pos1, pos2) ||
          !parser->first_frame(pos1, pos2 - pos1))
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
        sinfo = parser->sync_info2();
        scan.set_trie(sinfo.sync_trie);

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
StreamBuffer2::load(uint8_t **data, uint8_t *end)
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
  if (sync_data < sinfo.min_frame_size + header_size ||
      sync_data < sinfo.min_frame_size + sync_size)
    // need at least min_frame_size bytes
    return false;

  size_t pos = sinfo.min_frame_size;
  size_t scan_size = sync_data;
  if (header_size > sync_size)
    scan_size -= header_size - sync_size;

  while (scan.scan_pos(sync_buf, scan_size, pos))
  {
    if (parser->parse_header(sync_buf + pos) &&
        parser->next_frame(sync_buf, pos))
    {
      finfo = parser->frame_info();
      frame = sync_buf;
      frame_size = pos;
      return true;
    }
    pos++;
  }

  /////////////////////////////////////////////////////////////////////////////
  // No correct syncpoint found. Resync.

  resync();
  return sync(data, end);
}

bool 
StreamBuffer2::load_frame(uint8_t **data, uint8_t *end)
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
StreamBuffer2::flush()
{
  DROP(debris_size + frame_size);

  debris = 0;
  debris_size = 0;

  frame = 0;
  frame_size = 0;

  if (sync_data)
    if (load(0, 0))
      return true;

  if (sync_data)
    // Last frame?
    if (parser->next_frame(sync_buf, sync_data))
    {
      frame = sync_buf;
      frame_size = sync_data;
      return true;
    }
    else
    {
      debris = sync_buf;
      debris_size = sync_data;
      return true;
    }

  resync();
  return false;
}

string
StreamBuffer2::stream_info() const
{
  if (!parser)
    return "No parser set";

  if (!in_sync)
    return "No sync";

  return parser->stream_info();
}
