#include <memory.h>
#include "wav_source.h"
#include "../win32/winspk.h"

static const uint32_t fcc_riff = be2int32('RIFF');
static const uint32_t fcc_rf64 = be2int32('RF64');
static const uint32_t fcc_wave = be2int32('WAVE');
static const uint32_t fcc_ds64 = be2int32('ds64');
static const uint32_t fcc_fmt  = be2int32('fmt ');
static const uint32_t fcc_data = be2int32('data');

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

struct DS64Chunk
{
  uint32_t fcc;
  uint32_t size;
  uint64_t riff_size;
  uint64_t data_size;
  uint64_t sample_count;
  uint32_t table_length;
};

struct FMTChunk
{
  uint32_t fcc;
  uint32_t size;
  WAVEFORMATEX wfx;
};

WAVSource::WAVSource()
{
  chunk_size = 0;
  data_start = 0;
  data_size  = 0;
  data_remains = 0;
}

WAVSource::WAVSource(const char *filename_, size_t chunk_size_)
{
  chunk_size = 0;
  data_start = 0;
  data_size  = 0;
  data_remains = 0;

  format.allocate(sizeof(WAVEFORMATEX));
  format.zero();

  open(filename_, chunk_size_);
}

bool 
WAVSource::open(const char *filename_, size_t chunk_size_)
{
  close();
  f.open(filename_);
  if (!f.is_open())
    return false;

  if (!open_riff())
  {
    close();
    return false;
  }

  chunk_size = chunk_size_;
  buf.allocate(chunk_size);

  f.seek(data_start);
  data_remains = data_size;

  return true;
}

bool 
WAVSource::open_riff()
{
  /////////////////////////////////////////////////////////
  // Initializes spk, data_start and data_size

  const size_t buf_size = 256;
  uint8_t buf[buf_size];
  size_t buf_data;

  ChunkHeader *header = (ChunkHeader *)buf;
  RIFFChunk   *riff   = (RIFFChunk *)buf;
  DS64Chunk   *ds64   = (DS64Chunk *)buf;

  /////////////////////////////////////////////////////////
  // Check RIFF header

  f.seek(0);
  buf_data = f.read(buf, sizeof(RIFFChunk));
  if (buf_data < sizeof(RIFFChunk) ||
      (riff->fcc != fcc_riff && riff->fcc != fcc_rf64) || 
      riff->type != fcc_wave)
    return false;

  /////////////////////////////////////////////////////////
  // Chunk walk

  bool have_fmt = false;
  bool have_ds64 = false;
  uint64_t data_size64 = 0;
  AutoFile::fsize_t next = f.pos();
  while (1)
  {
    f.seek(next);
    buf_data = f.read(buf, sizeof(ChunkHeader));
    if (buf_data < sizeof(ChunkHeader))
      return false;

    ///////////////////////////////////////////////////////
    // Format chunk

    if (header->fcc == fcc_fmt)
    {
      if (!format.allocate(header->size))
        return false;

      format.zero();
      size_t data_read = f.read(format, header->size);
      if (data_read < header->size)
        return false;

      // Determine output format
      spk = wf2spk((WAVEFORMAT *)format.begin(), format.size());
      if (spk.is_unknown())
        spk = Speakers(FORMAT_RAWDATA, 0, 0);

      have_fmt = true;
    }

    ///////////////////////////////////////////////////////
    // ds64 chunk

    if (header->fcc == fcc_ds64)
    {
      buf_data += f.read(buf + buf_data, sizeof(DS64Chunk) - buf_data);
      if (buf_data < sizeof(DS64Chunk))
        return false;

      data_size64 = ds64->data_size;
      have_ds64 = true;
    }

    ///////////////////////////////////////////////////////
    // Data chunk

    if (header->fcc == fcc_data)
    {
      if (!have_fmt)
        return false;

      data_start = next + sizeof(ChunkHeader);
      data_size = header->size;
      if (have_ds64 && header->size >= 0xffffff00)
        data_size = data_size64;

      f.seek(data_start);
      data_remains = data_size;
      return true;
    }

    next += header->size + sizeof(ChunkHeader);
  }

  // never be here
  return false;
}

void
WAVSource::close()
{
  spk = spk_unknown;
  chunk_size = 0;
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

void
WAVSource::set_chunk_size(size_t chunk_size_)
{
  chunk_size = chunk_size_;
  if (is_open())
    buf.allocate(chunk_size);
}

size_t
WAVSource::get_chunk_size() const
{
  return chunk_size;
}

AutoFile::fsize_t
WAVSource::size() const
{
  if (data_size > (uint64_t)AutoFile::bad_size)
    return AutoFile::bad_size;
  return (AutoFile::fsize_t) data_size;
}

AutoFile::fsize_t
WAVSource::pos() const
{
  return f.pos() - data_start;
}

int
WAVSource::seek(AutoFile::fsize_t _pos)
{
  if (_pos < 0) _pos = 0;
  if ((uint64_t)_pos > data_size) _pos = data_size;

  int result = f.seek(_pos + data_start);
  data_remains = data_size - _pos;
  return result;
}

const WAVEFORMATEX *
WAVSource::wave_format() const
{ return (WAVEFORMATEX *)format.begin(); }

///////////////////////////////////////////////////////////
// Source interface

void
WAVSource::reset()
{
  if (f.is_open())
  {
    f.seek(data_start);
    data_remains = data_size;
  }
}

bool
WAVSource::get_chunk(Chunk &chunk)
{
  if (!data_remains || f.eof() || !f.is_open() )
    return false;

  size_t len = chunk_size;
  if (data_remains < chunk_size)
    len = f.size_cast(data_remains);

  size_t data_read = f.read(buf, len);

  if (data_read < len) // eof
    data_remains = 0;
  else
    data_remains -= len;

  chunk.set_rawdata(buf, data_read);
  return true;
}
