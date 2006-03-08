#include "wav_source.h"
#include "win32\winspk.h"

const uint32_t fcc_riff = 'RIFF';
const uint32_t fcc_wave = 'WAVE';
const uint32_t fcc_fmt  = 'fmt ';
const uint32_t fcc_data = 'data';

struct RIFFChunk
{
  uint32_t fcc;
  uint32_t size;
  uint32_t type;
};

struct FMTChunk
{
  uint32_t fcc;
  uint32_t size;
  WAVEFORMATEX wfx;
};

struct DataChunk
{
  uint32_t fcc;
  uint32_t size;
};

WAVSource::WAVSource()
{
  spk = spk_unknown;
  block_size = 0;
  data_start = 0;
  data_size  = 0;
  data_remains = 0;
}

WAVSource::WAVSource(const char *_filename, size_t _block_size)
{
  spk = spk_unknown;
  block_size = 0;
  data_start = 0;
  data_size  = 0;
  data_remains = 0;

  open(_filename, _block_size);
}

bool 
WAVSource::open(const char *_filename, size_t _block_size)
{
  close();
  f.open(_filename);
  if (!f.is_open())
    return false;

  if (!open_riff())
  {
    close();
    return false;
  }

  block_size = _block_size;
  if (!buf.allocate(block_size))
  {
    close();
    return false;
  }

  f.seek(data_start);
  data_remains = data_size;

  return true;
}

bool 
WAVSource::open_riff()
{
  /////////////////////////////////////////////////////////
  // Initializes spk, data_start and data_size

  uint8_t buf[255];
  size_t data_size;

  RIFFChunk *riff = (RIFFChunk *)buf;
  FMTChunk  *fmt  = (FMTChunk *)buf;
  DataChunk *data = (DataChunk *)buf;

  /////////////////////////////////////////////////////////
  // Check RIFF header

  f.seek(0);
  data_size = f.read(buf, sizeof(RIFFChunk));
  if (data_size < sizeof(RIFFChunk) ||
      riff->fcc != fcc_riff || 
      riff->type != fcc_wave)
    return false;;

  /////////////////////////////////////////////////////////
  // Seek fmt-chunk
  // Init spk

  int next = f.pos();
  while (1)
  {
    ///////////////////////////////////////////////////////
    // Read a chunk header and hope that it is fmt-chunk

    f.seek(next);
    data_size = f.read(buf, sizeof(FMTChunk));
    if (data_size < sizeof(FMTChunk))
      return false;

    ///////////////////////////////////////////////////////
    // Check FCC and point to the next chunk if it fails

    if (fmt->fcc != fcc_fmt)
    {
      next += fmt->size + 8;
      continue;
    }

    ///////////////////////////////////////////////////////
    // Load format extension

    if (fmt->wfx.cbSize)
    {
      data_size += f.read(buf + data_size, fmt->wfx.cbSize);
      if (data_size < sizeof(FMTChunk) + fmt->wfx.cbSize)
        return false;
    }

    ///////////////////////////////////////////////////////
    // Convert format

    if (!wfx2spk(&fmt->wfx, spk))
      return false;

    ///////////////////////////////////////////////////////
    // Ok

    next += fmt->size + 8;
    break;
  }

  /////////////////////////////////////////////////////////
  // Seek data-chunk
  // Init data_start and data_size

  while (1)
  {
    ///////////////////////////////////////////////////////
    // Read a chunk header

    f.seek(next);
    data_size = f.read(buf, sizeof(DataChunk));
    if (data_size < sizeof(DataChunk))
      return false;

    ///////////////////////////////////////////////////////
    // Check FCC and point to the next chunk if it fails

    if (data->fcc != fcc_data)
    {
      next += data->size + 8;
      continue;
    }

    ///////////////////////////////////////////////////////
    // Determine actual data size (cut file possible)

    data_start = next + 8;
    f.seek(data_start + data->size);
    data_size = f.pos() - data_start;
    return true;
  }
}

void
WAVSource::close()
{
  spk = spk_unknown;
  block_size = 0;
  data_start = 0;
  data_size  = 0;
  data_remains = 0;

  f.close();
}

bool
WAVSource::is_open() const
{
  return f.is_open();
}


int
WAVSource::size() const
{
  return data_size;
}

int
WAVSource::pos() const
{
  return f.pos() - data_start;
}

void
WAVSource::seek(int _pos)
{
  if (_pos > data_size)
    _pos = data_size;

  if (_pos < 0)
    _pos = 0;

  f.seek(_pos + data_start);
  data_remains = data_size - _pos;
}

///////////////////////////////////////////////////////////
// Source interface

Speakers 
WAVSource::get_output() const
{
  return spk;
}

bool 
WAVSource::is_empty() const
{
  return data_remains == 0;
}

bool
WAVSource::get_chunk(Chunk *_chunk)
{
  int len = min(data_remains, block_size);
  int data_read = f.read(buf, len);

  if (data_read < len) // eof
    data_remains = 0;
  else
    data_remains -= len;

  _chunk->set_rawdata(spk, buf, data_read, false, 0, data_remains > 0);
  return true;
}
