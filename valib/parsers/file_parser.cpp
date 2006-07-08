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

bool 
FileParser::probe()
{
  if (!f) return false;

  int old_pos = get_pos();
  int old_parser_frames = parser->get_frames();
  int old_parser_errors = parser->get_errors();
  int old_demux_errors  = demux.parser.errors;

  int i;
  bool failed = false;

  /////////////////////////////////////////////////////////
  // Try as PES

  seek(old_pos);
  is_pes  = true;

  failed = false;
  for (i = 0; i < 50; i++)  // try to decode 50 frames
    if (!frame())
    {
      failed = true;
      break;
    }
  if (!failed) goto probe_ok;

  /////////////////////////////////////////////////////////
  // Try as pure stream

  seek(old_pos);
  is_pes  = false;

  failed = false;
  for (i = 0; i < 50; i++)  // try to decode 50 frames
    if (!frame())
    {
      failed = true;
      break;
    }
  if (!failed) goto probe_ok;

  // probe failed
  seek(old_pos);
  frames_overhead += parser->get_frames() - old_parser_frames;
  errors_overhead += parser->get_errors() - old_parser_frames;
  errors_overhead += demux.parser.errors - old_demux_errors;
  return false;

probe_ok:
  // probe succeeded
  seek(old_pos);
  frames_overhead += parser->get_frames() - old_parser_frames;
  errors_overhead += parser->get_errors() - old_parser_errors;
  errors_overhead += demux.parser.errors - old_demux_errors;
  return true;
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
    "avg. frame size: %i bytes\n"
    "avg. samples/frame: %i\n"
    "\n",

    get_filename(), 
    size_bytes, 
    size_frames, 
    int(size_sec / 60), int(size_sec % 60),
    frame_size, 
    frame_samples);

  info_len = MIN(len, strlen(info)+1);
  memcpy(buf, info, info_len);

  buf += info_len - 1;
  len -= info_len + 1;

  parser->get_info(buf, len); 
}