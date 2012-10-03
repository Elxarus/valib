#include <memory>
#include <memory.h>
#include "../win32/winspk.h"
#include "sink_wav.h"

// Positions of header fields
static const int riff_size32_pos = 4;

WAVSink::WAVSink()
{
  spk = spk_unknown;
  header_size = 0;
  data_size = 0;
  file_format = 0;
}

WAVSink::WAVSink(const char *_file_name)
{
  spk = spk_unknown;
  header_size = 0;
  data_size = 0;
  file_format = 0;

  open_file(_file_name);
}

WAVSink::~WAVSink()
{
  close_file();
  safe_delete(file_format);
}

void
WAVSink::init_riff()
{
  WAVEFORMATEX *wfe = (WAVEFORMATEX *)file_format;
  uint32_t format_size = sizeof(WAVEFORMATEX) + wfe->cbSize;
  f.seek(0);

  // RIFF header
  f.write("RIFF\0\0\0\0WAVE", 12);

  // RF64 junk chunk
  // (to be replaced with ds64 chunk if nessesary)
  f.write("JUNK\x1c\0\0\0", 8);    // header
  f.write("\0\0\0\0\0\0\0\0", 8); // RIFF size
  f.write("\0\0\0\0\0\0\0\0", 8); // data size
  f.write("\0\0\0\0\0\0\0\0", 8); // sample count
  f.write("\0\0\0\0", 4);         // table length (unused)

  // Format chunk
  f.write("fmt ", 4);
  f.write(&format_size, 4);
  f.write(file_format, format_size);

  // Data chunk
  f.write("data\0\0\0\0", 8);

  header_size = (uint32_t) f.pos();
}

void
WAVSink::close_riff()
{
  uint64_t riff_size = header_size + data_size - 8;

  if (riff_size <= 0xffffffff)
  {
    // Write usual RIFF sizes
    uint32_t riff_size32 = (uint32_t) riff_size;
    uint32_t data_size32 = (uint32_t) data_size;
    // RIFF size
    f.seek(riff_size32_pos);
    f.write(&riff_size32, 4);
    // data size
    f.seek(header_size - 4);
    f.write(&data_size32, 4);
  }
  else
  {
    // Make RF64 header
    uint32_t riff_size32 = 0xffffffff;
    uint32_t data_size32 = 0xffffffff;

    WAVEFORMATEX *wfe = (WAVEFORMATEX *)file_format;
    uint64_t sample_count = data_size;
    if (wfe->nBlockAlign > 0)
      sample_count = data_size / wfe->nBlockAlign;

    // RF64 header
    f.seek(0);
    f.write("RF64", 4);
    f.write(&riff_size32, 4);
    f.write("WAVE", 4);

    // ds64 chunk
    f.write("ds64\x1c\0\0\0", 8); // header
    f.write(&riff_size, 8);
    f.write(&data_size, 8);
    f.write(&sample_count, 8);

    // data size
    f.seek(header_size - 4);
    f.write(&data_size32, 4);
  }
}

bool
WAVSink::open_file(const char *new_file_name)
{
  close_file();
  if (!f.open(new_file_name, "wb"))
    return false;

  fname = new_file_name;
  data_size = 0;
  safe_delete(file_format);
  return true;
}

void
WAVSink::close_file()
{
  if (!f.is_open())
    return;

  if (is_open())
  {
    close_riff();
    close();
  }
  f.close();
  fname.clear();

  header_size = 0;
  data_size = 0;
  safe_delete(file_format);
}

bool
WAVSink::is_file_open() const
{
  return f.is_open();
}

string
WAVSink::filename() const
{
  return fname;
}


///////////////////////////////////////////////////////////
// Sink interface

bool
WAVSink::can_open(Speakers new_spk) const
{
  if (!f.is_open())
    return false;

  // Use first (main) WAVEFROMATEX only
  std::auto_ptr<WAVEFORMATEX> wfe(spk2wfe(new_spk, 0));
  return wfe.get() != 0;
}

bool
WAVSink::init()
{
  WAVEFORMATEX *wfe = (WAVEFORMATEX *)file_format;
  WAVEFORMATEX *new_wfe = spk2wfe(spk, 0);
  if (!new_wfe)
    return false;

  // Reset the file only in case when formats are not equal
  if (!wfe || wfe->cbSize != new_wfe->cbSize || 
      memcmp(wfe, new_wfe, sizeof(WAVEFORMATEX) + wfe->cbSize) != 0)
  {
    safe_delete(file_format);
    file_format = (uint8_t *)new_wfe;
    init_riff();
  }
  else
    safe_delete(new_wfe);

  return true;
}

void
WAVSink::process(const Chunk &chunk)
{
  if (f.is_open())
  {
    f.write(chunk.rawdata, chunk.size);
    data_size += chunk.size;
  }
}
