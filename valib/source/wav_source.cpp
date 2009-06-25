#include <memory.h>
#include "wav_source.h"
#include "../win32/winspk.h"

const uint32_t fcc_riff = be2int32('RIFF');
const uint32_t fcc_wave = be2int32('WAVE');
const uint32_t fcc_fmt  = be2int32('fmt ');
const uint32_t fcc_data = be2int32('data');

struct ChunkHeader
{
  uint32_t fcc;
  uint32_t size;
};

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
  size_t buf_data;

  ChunkHeader *header = (ChunkHeader *)buf;
  RIFFChunk   *riff   = (RIFFChunk *)buf;
  FMTChunk    *fmt    = (FMTChunk *)buf;

  /////////////////////////////////////////////////////////
  // Check RIFF header

  f.seek(0);
  buf_data = f.read(buf, sizeof(RIFFChunk));
  if (buf_data < sizeof(RIFFChunk) ||
      riff->fcc != fcc_riff || 
      riff->type != fcc_wave)
    return false;

  /////////////////////////////////////////////////////////
  // Seek fmt-chunk
  // Init spk

  AutoFile::fsize_t next = f.pos();
  while (1)
  {
    ///////////////////////////////////////////////////////
    // Read a chunk header

    f.seek(next);
    buf_data = f.read(buf, sizeof(ChunkHeader));
    if (buf_data < sizeof(ChunkHeader))
      return false;

    ///////////////////////////////////////////////////////
    // Check FCC and go to the next chunk if it is not fmt

    if (header->fcc != fcc_fmt)
    {
      next += header->size + sizeof(ChunkHeader);
      continue;
    }

    ///////////////////////////////////////////////////////
    // Load format chunk

    memset(&fmt->wfx, 0, sizeof(fmt->wfx));
    buf_data = f.read(buf + buf_data, header->size);

    if (buf_data < header->size)
      return false;

    ///////////////////////////////////////////////////////
    // Convert format

    if (!wfx2spk(&fmt->wfx, spk))
      return false;

    ///////////////////////////////////////////////////////
    // Ok

    next += fmt->size + sizeof(ChunkHeader);
    break;
  }

  /////////////////////////////////////////////////////////
  // Seek data-chunk
  // Init data_start and buf_data

  while (1)
  {
    ///////////////////////////////////////////////////////
    // Read a chunk header

    f.seek(next);
    buf_data = f.read(buf, sizeof(ChunkHeader));
    if (buf_data < sizeof(ChunkHeader))
      return false;

    ///////////////////////////////////////////////////////
    // Check FCC and go to the next chunk if it is not data

    if (header->fcc != fcc_data)
    {
      next += header->size + sizeof(ChunkHeader);
      continue;
    }

    ///////////////////////////////////////////////////////
    // Determine actual data size

    data_start = next + sizeof(ChunkHeader);
    data_size = header->size;

    if (f.size() != f.max_size)
    {
      AutoFile::fsize_t file_tail = f.size() - data_start;
      if (header->size >= 0xffffff00 || header->size > file_tail)
        // * File is a big WAV file >4Gb
        // * File is cut down (incomplete download, etc)
        data_size = file_tail;
    }

    f.seek(data_start);
    data_remains = data_size;
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


AutoFile::fsize_t
WAVSource::size() const
{
  return data_size;
}

AutoFile::fsize_t
WAVSource::pos() const
{
  return f.pos() - data_start;
}

int
WAVSource::seek(AutoFile::fsize_t _pos)
{
  if (_pos > data_size)
    _pos = data_size;

  int result = f.seek(_pos + data_start);
  data_remains = data_size - _pos;
  return result;
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
  size_t len = block_size;
  if (data_remains < block_size)
    len = f.size_cast(data_remains);

  size_t data_read = f.read(buf, len);

  if (data_read < len) // eof
    data_remains = 0;
  else
    data_remains -= len;

  _chunk->set_rawdata(spk, buf, data_read, false, 0, data_remains <= 0);
  return true;
}
