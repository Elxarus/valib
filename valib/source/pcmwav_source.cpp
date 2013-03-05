#include "pcmwav_source.h"

PcmWavSource::PcmWavSource():
SourceWrapper(&source), conv(2048)
{
  source.set(&wav, &conv);
  conv.set_format(FORMAT_LINEAR);
  conv.set_order(win_order);
}

PcmWavSource::PcmWavSource(const char *filename, size_t chunk_size):
SourceWrapper(&source), conv(2048)
{
  source.set(&wav, &conv);
  conv.set_format(FORMAT_LINEAR);
  conv.set_order(win_order);
  open_file(filename, chunk_size);
}

bool
PcmWavSource::open_file(const char *filename, size_t chunk_size)
{
  close_file();
  if (!wav.open(filename, chunk_size))
    return false;

  Speakers spk = wav.get_output();
  conv.set_buffer(chunk_size);
  if (spk.is_unknown() || !conv.open(spk))
  {
    wav.close();
    return false;
  }

  wav.set_chunk_size(chunk_size * spk.sample_size() * spk.nch());
  return true;
}

void
PcmWavSource::close_file()
{
  wav.close();
  conv.close();
}

bool
PcmWavSource::is_file_open() const
{
  return wav.is_open();
}
