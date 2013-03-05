#include "sink_pcmwav.h"

PcmWavSink::PcmWavSink():
SinkWrapper(&linear_sink), conv(2048)
{
  linear_sink.set(&wav, &conv);
  conv.set_format(FORMAT_PCM16);
  conv.set_order(win_order);
}

PcmWavSink::PcmWavSink(const char *filename, int format):
SinkWrapper(&linear_sink), conv(2048)
{
  linear_sink.set(&wav, &conv);
  conv.set_format(FORMAT_PCM16);
  conv.set_order(win_order);
  open_file(filename, format);
}

bool
PcmWavSink::open_file(const char *filename, int format)
{
  close();
  conv.set_format(format);
  return wav.open_file(filename);
}

void
PcmWavSink::close_file()
{
  wav.close_file();
  conv.close();
}

bool
PcmWavSink::is_file_open()
{
  return wav.is_file_open();
}

bool
PcmWavSink::can_open(Speakers spk) const
{
  if (!wav.is_file_open()) return false;
  return SinkWrapper::can_open(spk);
}

bool
PcmWavSink::open(Speakers spk)
{
  if (!wav.is_file_open()) return false;
  return SinkWrapper::open(spk);
}
