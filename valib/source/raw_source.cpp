#include "raw_source.h"

bool 
RAWSource::open(Speakers spk_, const char *filename_, size_t block_size_)
{
  if (spk.format == FORMAT_LINEAR)
    return false;

  if (!f.open(filename_))
    return false;

  if (!buf.allocate(block_size_))
    return false;

  spk = spk_;
  block_size = block_size_;
  return true;
}

bool 
RAWSource::open(Speakers spk_, FILE *f_, size_t block_size_)
{
  if (spk.format == FORMAT_LINEAR)
    return false;

  if (!f.open(f_))
    return false;

  if (!buf.allocate(block_size_))
    return false;

  spk = spk_;
  block_size = block_size_;
  return true;
}

void 
RAWSource::close()
{ 
  spk = spk_unknown;
  f.close();
}

bool
RAWSource::get_chunk(Chunk2 &chunk)
{
  if (!f.is_open() || f.eof())
    return false;

  size_t read_size = f.read(buf, block_size);
  chunk.set_rawdata(buf, read_size);
  return true;
};
