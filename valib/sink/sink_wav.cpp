#include <windows.h>
#include <ks.h>
#include <ksmedia.h>
#include "sink_wav.h"
#include "win32\winspk.h"

WAVSink::WAVSink()
{
  spk = spk_unknown;
  header_size = 0;
  data_size = 0;
}

WAVSink::WAVSink(const char *_file_name, Speakers _spk)
{
  spk = spk_unknown;
  open(_file_name, _spk);
}

WAVSink::~WAVSink()
{
  close();
}

bool
WAVSink::open(const char *_file_name, Speakers _spk)
{
  close();

  // Determine file format

  WAVEFORMATEXTENSIBLE wfx;
  bool use_wfx = false;

  if (_spk.format == FORMAT_LINEAR)
    return false;

  if (_spk.format & FORMAT_CLASS_PCM)
    if (_spk.mask != MODE_MONO && _spk.mask != MODE_STEREO)
      use_wfx = true;

  if (!spk2wfx(_spk, (WAVEFORMATEX *)&wfx, use_wfx))
    return false;

  // Open file

  if (!f.open(_file_name, "wb"))
    return false;

  // Write WAV headers

  f.write("RIFF\0\0\0\0", 8);
  f.write("WAVEfmt ", 8);
  uint32_t format_size = sizeof(WAVEFORMATEX) + wfx.Format.cbSize;
  f.write(&format_size, 4);
  f.write(&wfx, format_size);
  f.write("data\0\0\0\0", 8);
  header_size = f.pos();

  // Initialize

  spk = _spk;
  data_size = 0;

  return true;
}

void
WAVSink::close()
{
  if (!f.is_open())
    return;

  uint32_t riff_size = header_size + data_size - 8;
  f.seek(4);
  f.write(&riff_size, 4);

  f.seek(header_size - 4);
  f.write(&data_size, 4);

  f.close();

  spk = spk_unknown;
  header_size = 0;
  data_size = 0;
}

bool
WAVSink::is_open() const
{
  return f.is_open();
}


///////////////////////////////////////////////////////////
// Sink interface

bool
WAVSink::query_input(Speakers _spk) const
{
  return spk == _spk;
}

bool
WAVSink::set_input(Speakers _spk)
{
  return spk == _spk;
}

Speakers
WAVSink::get_input() const
{
  return spk;
}

bool
WAVSink::process(const Chunk *_chunk)
{
  if (_chunk->is_dummy())
    return true;

  if (_chunk->spk != spk)
    if (!set_input(_chunk->spk))
      return false;

  f.write(_chunk->rawdata, _chunk->size);
  data_size += _chunk->size;
  return true;
}
