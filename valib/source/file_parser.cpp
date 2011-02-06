#include <vector>
#include <sstream>
#include "file_parser.h"

static const size_t buf_size = 65536;

int compact_size(AutoFile::fsize_t size)
{
  int iter = 0;
  while (size >= 10000 && iter < 5)
  {
    size /= 1024;
    iter++;
  }
  return (int)size;
}

const char *compact_suffix(AutoFile::fsize_t size)
{
  static const char *suffixes[] = { "", "K", "M", "G", "T", "P" };

  int iter = 0;
  while (size >= 10000 && iter < 5)
  {
    size /= 1024;
    iter++;
  }
  return suffixes[iter];
}

FileParser::FileParser()
{
  has_probe = false;
  is_new_stream = false;

  buf.allocate(buf_size);
  buf_pos = buf.begin();
  buf_end = buf.begin();

  stat_size = 0;
  avg_frame_interval = 0;
  avg_bitrate = 0;

  max_scan = 0;
}

FileParser::~FileParser()
{}

///////////////////////////////////////////////////////////////////////////////
// File operations

bool 
FileParser::open(const string &new_filename, const HeaderParser *new_parser, size_t new_max_scan)
{
  if (is_open()) 
    close();

  if (!new_parser)
    return false;

  if (!f.open(new_filename.c_str()))
    return false;

  stream.set_parser(new_parser);
  max_scan = new_max_scan;
  filename = new_filename;

  stream_reset();
  return true;
}

bool 
FileParser::open_probe(const string &new_filename, const HeaderParser *new_parser, size_t new_max_scan)
{
  if (!open(new_filename, new_parser, new_max_scan))
    return false;

  if (probe())
    return true;

  close();
  return false;
}

void 
FileParser::close()
{
  stream.release_parser();
  f.close();

  has_probe = false;
  is_new_stream = false;

  stat_size = 0;
  avg_frame_interval = 0;
  avg_bitrate = 0;

  max_scan = 0;
}

bool 
FileParser::probe()
{
  if (!f) return false;

  if (has_probe)
    return true;

  stream_reset();
  bool result = load_frame();
  has_probe = true;
  return result;
}

bool
FileParser::stats(vtime_t precision, unsigned min_measurements, unsigned max_measurements)
{
  if (!f) return false;

  fsize_t old_pos = f.pos();

  // Do not measure if we cannot load a frame.
  // (If file format is unknown measurments may take much of time)
  if (!load_frame())
  {
    seek(old_pos);
    return false;
  }

  double precision_squared = precision * precision / 4;
  stat_size = 0;
  avg_frame_interval = 0;
  avg_bitrate = 0;

  std::vector<double> bitrate_stat;
  for (unsigned i = 0; i < max_measurements; i++)
  {
    fsize_t file_pos = fsize_t((double)rand() * f.size() / RAND_MAX);
    seek(file_pos);
    if (!load_frame())
      continue;

    ///////////////////////////////////////////////////////
    // Update stats

    HeaderInfo hinfo = stream.header_info();

    stat_size++;
    double bitrate = double(stream.get_frame_interval() * 8 * hinfo.spk.sample_rate) / hinfo.nsamples;
    avg_frame_interval += stream.get_frame_interval();
    avg_bitrate        += bitrate;
    bitrate_stat.push_back(bitrate);

    ///////////////////////////////////////////////////////
    // Stop measure when we have enough accuracy
    //
    // We measure bitrate and derive the duration as follow:
    // D(b) = s/b
    // where
    //   d - duration
    //   s - file size in bits
    //   b - bitrate
    //
    // Therefore, duration measurement error is derived from
    // bitrate measurement error as follow:
    // S_d = \frac{dD(b)}{db} S_b = \frac{s}{b^2} S_b
    //
    // Where S_b is bitrate measurement error:
    // S_b = \sqrt{\frac{\sum_{i=1}^N{(\bar{b}-b_i)}}{N(N-1)}}
    //
    // To avoid calculation of the square root, squared error is used:
    // S_d^2 = \frac{s^2}{b^4} \cdot \frac{\sum_{i=1}^N{(\bar{b}-b_i)}}{N(N-1)}

    if (precision == 0.0 && stat_size > min_measurements)
      // Break when min_measurements are done
      break;

    if (precision > 0 && stat_size > min_measurements)
    {
      // Break when we reach enough accuracy
      double error_squared = 0;
      double bitrate = avg_bitrate / stat_size;
      for (i = 0; i < stat_size; i++)
        error_squared += (bitrate - bitrate_stat[i])*(bitrate - bitrate_stat[i]);
      error_squared /= stat_size * (stat_size - 1);
      error_squared *= f.size()*f.size()*64 / (bitrate*bitrate*bitrate*bitrate);
      if (error_squared < precision_squared)
        break;
    }
  }

  if (stat_size)
  {
    avg_frame_interval /= stat_size;
    avg_bitrate        /= stat_size;
  }

  seek(old_pos);
  return stat_size > 0;
}

string
FileParser::file_info() const
{
  using std::endl;
  std::stringstream result;

  result << "File: " << filename << endl;
  result << "Size: " << f.size()
         << " (" << compact_size(f.size()) << " " << compact_suffix(f.size()) << "B)" << endl;

  if (stat_size)
  {
    result << "Length: "
      << int(get_size(time)) / 3600 << ":"
      << int(get_size(time)) % 3600 / 60 << ":"
      << int(get_size(time)) % 60 << endl;
    result << "Frames: " << int(get_size(frames)) << endl;
    result << "Frame interval: " << int(avg_frame_interval) << endl;
    result << "Bitrate: " << int(avg_bitrate / 1000) << "kbps" << endl;
  }

  return result.str();
}

///////////////////////////////////////////////////////////////////////////////
// Positioning

inline double
FileParser::units_factor(units_t units) const
{
  switch (units)
  {
    case bytes:    return 1.0;
    case relative: return 1.0 / f.size();
  }

  if (stat_size)
    switch (units)
    {
      case frames:  return 1.0 / avg_frame_interval;
      case time:    return 8.0 / avg_bitrate;
    }

  return 0.0;
}

FileParser::fsize_t
FileParser::get_pos() const
{
  return f.is_open()? fsize_t(f.pos() - (buf_end - buf_pos)): 0;
}

double 
FileParser::get_pos(units_t units) const
{
  return get_pos() * units_factor(units);
}

FileParser::fsize_t
FileParser::get_size() const
{
  return f.size();
}

double 
FileParser::get_size(units_t units) const
{
  return f.size() * units_factor(units);
}

int
FileParser::seek(fsize_t pos)
{
  int result = f.seek(pos);
  stream_reset();
  return result;
}

int
FileParser::seek(double pos, units_t units)
{ 
  double factor = units_factor(units);
  if (factor > 0)
    return seek(fsize_t(pos / factor + 0.5));
  return -1;
}

///////////////////////////////////////////////////////////////////////////////
// Frame-level interface (StreamBuffer interface wrapper)

void
FileParser::stream_reset()
{
  buf_pos = buf.begin();
  buf_end = buf.begin();
  stream.reset();
  has_probe = false;
  is_new_stream = false;
}

bool
FileParser::load_frame()
{
  size_t scan_size = 0;

  while (!f.eof() || buf_pos < buf_end)
  {
    if (buf_pos >= buf_end)
    {
      size_t read_size = f.read(buf.begin(), buf.size());
      buf_pos = buf.begin();
      buf_end = buf.begin() + read_size;
    }

    size_t data_size = buf_end - buf_pos;

    if (stream.load_frame(&buf_pos, buf_end))
      return true;

    scan_size += data_size;
    if (max_scan && scan_size > max_scan)
      return false;
  }
  return false;
}

/////////////////////////////////////////////////////////////////////////////
// Source interface

void
FileParser::reset()
{
  seek(0);
}

bool
FileParser::get_chunk(Chunk &out)
{
  if (has_probe)
  {
    out.set_rawdata(stream.get_frame(), stream.get_frame_size());
    has_probe = false;
    return true;
  }

  if (load_frame())
  {
    is_new_stream = stream.is_new_stream();
    out.set_rawdata(stream.get_frame(), stream.get_frame_size());
    return true;
  }
  return false;
}

bool
FileParser::new_stream() const
{
  return is_new_stream;
}

Speakers
FileParser::get_output() const
{
  return stream.get_spk();
}
