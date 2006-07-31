#include <stdlib.h>
#include <string.h>
#include "file_parser.h"

#define swab16(x) (((x) >> 8) & 0xff | (((x) & 0xff) << 8))

const int max_buf_size = 65536;

FileParser::FileParser()
{
  parser = 0;
  max_scan = 0;

  f = 0;
  filename = 0;
  filesize = 0;

  frame_size = 0;
  frame_samples = 0;

  frames_overhead = 0;
  errors_overhead = 0;

  buf_data = 0;
  buf_pos = 0;

  is_pes = false;

  buf = new uint8_t[max_buf_size];
  if (buf)
    buf_size = max_buf_size;
  else
    buf_size = 0;
}

void 
FileParser::reset()
{
  parser->reset();
  demux.reset();
  buf_data = 0;
  buf_pos = 0;
}

FileParser::~FileParser()
{
  close();
}


bool 
FileParser::open(Parser *_parser, const char *_filename, unsigned _max_scan)
{
  if (!buf || !_parser || !_filename) return false;

  if (f) close();
  f = fopen(_filename, "rb");
  if (!f) return false;

  parser = _parser;
  max_scan = _max_scan;

  filename = strdup(_filename);

  fseek(f, 0, SEEK_END);
  filesize = ftell(f);
  fseek(f, 0, SEEK_SET);

  frames_overhead = parser->get_frames();
  errors_overhead = parser->get_errors() + demux.parser.errors;

  detect_pes();
  reset();
  return true;
}

void 
FileParser::close()
{
  if (f) 
  {
    fclose(f);
    f = 0;
  }

  if (filename)
  {
    delete filename;
    filename = 0;
  }

  filesize = 0;

  frame_size = 0;
  frame_samples = 0;

  buf_data = 0;
  buf_pos = 0;

  is_pes = false;
}

void
FileParser::detect_pes(size_t scan_size, int npackets)
{
  // This function scans the file and search for a sequence
  // of packets of the same stream.

  PSParser ps;

  size_t old_pos = get_pos();

  uint8_t *ps_pos;
  uint8_t *ps_end;

  int ps_stream = 0;
  int ps_substream = 0;

  int packet_size = 0;
  int packets = 0;

  is_pes = false;
  while (1)
  { 
    // Fill buffer

    if (!scan_size)
    {
      seek(old_pos);
      return;
    }

    ps_pos = buf;
    ps_end = buf;
    ps_end += fread(ps_pos, 1, buf_size, f);
    scan_size -= (ps_end - ps_pos);
    if (ps_pos == ps_end)
    {
      seek(old_pos);
      return;
    }

    // Parse buffer

    while (ps_pos < ps_end)
    {
      if (packet_size)
      {
        // drop packet
        if (packet_size > ps_end - ps_pos)
        {
          packet_size -= ps_end - ps_pos;
          ps_pos = ps_end;
        }
        else
        {
          ps_pos += packet_size;
          packet_size = 0;
        }
      }
      else if (packet_size = ps.parse(&ps_pos, ps_end))
      {
        // count consecutive packets of the same stream
        if ((ps_stream && ps_stream != ps.stream) ||
            (ps_substream && ps.substream != ps.substream))
        {
          ps_stream = ps.stream;
          ps_substream = ps.substream;
          packets = 0;
        }
        else
        {
          ps_stream = ps.stream;
          ps_substream = ps.substream;

          packets++;
          if (packets >= npackets)
          {
            is_pes = true;
            seek(old_pos);
            return;
          }
        }
      } // if (packet_size = ps.parse(&ps_pos, ps_end))
    }
  } // while (1)

  // never be here
}

bool 
FileParser::probe(size_t scan_size, int nframes)
{
  if (!f) return false;

  size_t old_pos = get_pos();
  int old_parser_frames = parser->get_frames();
  int old_parser_errors = parser->get_errors();
  int old_demux_errors  = demux.parser.errors;

  int frames = 0;

  while (1)
  { 
    if (frame())
    {
      frames++;
      if (frames > nframes)
      {
        // probe succeeded
        seek(old_pos);
        frames_overhead += parser->get_frames() - old_parser_frames;
        errors_overhead += parser->get_errors() - old_parser_errors;
        errors_overhead += demux.parser.errors - old_demux_errors;
        return true;
      }
    }
    else
      frames = 0;

    if (eof() || (get_pos() - old_pos > scan_size))
    {
      // probe failed
      seek(old_pos);
      frames_overhead += parser->get_frames() - old_parser_frames;
      errors_overhead += parser->get_errors() - old_parser_frames;
      errors_overhead += demux.parser.errors - old_demux_errors;
      return false;
    }
  } // while (1)

  // never be here
}

void
FileParser::stats(int nframes)
{
  if (!f) return;

  frame_size    = 0;
  frame_samples = 0;
  sample_rate   = 0;
  int cnt = 0;

  int old_pos = get_pos();
  int old_parser_frames = parser->get_frames();
  int old_parser_errors = parser->get_errors();
  int old_demux_errors  = demux.parser.errors;

  for (int i = 0; i < nframes; i++)
  {
    int file_pos = int((double)rand() * filesize / RAND_MAX);
    seek(file_pos);

    if (!load_frame() || !decode_frame())
      continue;

    file_pos = get_pos();

    if (!load_frame() || !decode_frame())
      continue;

    frame_size     += get_pos() - file_pos;;
    frame_samples  += parser->get_nsamples();
    sample_rate    += parser->get_spk().sample_rate;
    cnt++;
  }

  if (cnt)
  {
    frame_size    /= cnt;
    frame_samples /= cnt;
    sample_rate   /= cnt;
  }

  seek(old_pos);
  frames_overhead += parser->get_frames() - old_parser_frames;
  errors_overhead += parser->get_errors() - old_parser_errors;
  errors_overhead += demux.parser.errors - old_demux_errors;
}

void 
FileParser::set_mpeg_stream(int stream, int substream)
{
  demux.stream = stream;
  demux.substream = substream;
  reset();
}


void
FileParser::seek(int pos)
{ 
  fseek(f, pos, SEEK_SET);
  reset();
}

void
FileParser::seek(double pos, units_t units)
{ 
  int abs_pos = 0;

  switch (units)
  {
    case bytes:    seek(int(pos)); return;
    case relative: seek(int(pos * filesize)); return;
  }
  
  if (frame_size && frame_samples)
    switch (units)
    {
      case frames:  seek(int(pos * frame_size)); return;
      case samples: seek(int(pos * frame_size / frame_samples)); return;
      case ms:      seek(int(pos * frame_size / frame_samples * sample_rate / 1000)); return;
    }
}

int
FileParser::get_pos() const
{
  return f? ftell(f) - buf_data + buf_pos: 0;
}

double 
FileParser::get_pos(units_t units) const
{
  double pos = get_pos();

  switch (units)
  {
    case bytes:    return pos;
    case relative: return pos / filesize;
  }

  if (frame_size && frame_samples)
    switch (units)
    {
      case frames:  return pos / frame_size;
      case samples: return pos / frame_size * frame_samples;
      case ms:      return pos / frame_size * frame_samples / sample_rate * 1000;
    }

  return 0;
}

int
FileParser::get_size() const
{
  return filesize;
}

double 
FileParser::get_size(units_t units) const
{
  double pos = filesize;

  switch (units)
  {
    case bytes:    return pos;
    case relative: return pos / filesize;
  }

  if (frame_size && frame_samples)
    switch (units)
    {
      case frames:  return pos / frame_size;
      case samples: return pos / frame_size * frame_samples;
      case ms:      return pos / frame_size * frame_samples / sample_rate * 1000;
    }

  return 0;
}

double 
FileParser::get_bitrate() const
{
  double length = get_size(ms) / 1000;
  if (length > 0.001)
    return get_size() * 8 / length;
  else
    return 0;
}



unsigned 
FileParser::load_frame()
{
  int scan = 0;
  uint8_t *pos;

  while (scan < max_scan)
  {
    if (!buf_data || buf_pos >= buf_data)
      if (!fill_buf())
        return false;

    pos = buf + buf_pos;
    if (parser->load_frame(&pos, buf + buf_data))
    {
      buf_pos = pos - buf;
      return true;
    }

    scan += (pos - buf) - buf_pos;
    buf_pos = pos - buf;
  }
  return false;
}

bool 
FileParser::fill_buf()
{
  if (!f) return false;

  if (buf_data && buf_pos)
    memmove(buf, buf + buf_pos, buf_data - buf_pos);

  buf_pos = 0;
  int sync = 0;

  do
  {
    buf_data = fread(buf, 1, max_buf_size, f);
    if (!buf_data) return false;

    if (is_pes)
    {
      sync += buf_data;
      buf_data = demux.demux(buf, buf_data);
    }
  }
  while (!buf_data && sync < max_scan);

  return true;
}


void 
FileParser::get_info(char *buf, size_t len) const 
{
  char info[1024];
  size_t info_len;

  int size_bytes = get_size();
  int size_sec = int(get_size(ms) / 1000);
  int size_frames = int(get_size(frames));

  sprintf(info,
    "File: %s\n"
    "size: %i bytes\n"
    "frames: %i\n"
    "length: %i:%02i\n"
    "%s"
    "avg. frame size: %i bytes%s\n"
    "avg. samples/frame: %i\n"
    "\n",

    get_filename(), 
    size_bytes, 
    size_frames,
    int(size_sec / 60), int(size_sec % 60),
    is_pes? "PES-wrapped stream\n": "",
    frame_size, is_pes? " (including PES overhead)": "",
    frame_samples);

  info_len = MIN(len, strlen(info)+1);
  memcpy(buf, info, info_len);

  buf += info_len - 1;
  len -= info_len + 1;

  parser->get_info(buf, len); 
}