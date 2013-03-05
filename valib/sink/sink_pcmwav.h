/**************************************************************************//**
  \file sink_pcmwav.h
  \brief PcmWavSink class
******************************************************************************/

#ifndef SINK_PCMWAV_H
#define SINK_PCMWAV_H

#include "sink_wav.h"
#include "sink_filter.h"
#include "../filters/convert.h"

/**************************************************************************//**
  \class PcmWavSink
  \brief Sink for PCM wave files, accepts FORMAT_LINEAR (no need to convert
    to pcm explicitly) and PCM format. Does not convert the level. I.e. when
    writing a PCM16 file, input signal level must be 32767.5.

  \fn PcmWavSink::PcmWavSink();
    Creates a sink with no file open. Call open_file() to create a wav file.

  \fn PcmWavSink::PcmWavSink(const char *filename, int format);
    \param filename File to create.
    \param format PCM sample format.

    Creates a sink with a file open. Does not throw on error, you have to
    check is_file_open() manually.

  \fn bool PcmWavSink::open_file(const char *filename, int format);
    \param filename File to open.
    \param format PCM sample format.

    Creates a wav file with the format given. Returns true on success and
    false otherwise. 

  \fn void PcmWavSink::close_file();
    Closes the sink and the currently open file.

  \fn bool PcmWavSink::is_file_open() const;
    Returns true when a wav file is open.
******************************************************************************/
    
class PcmWavSink : public SinkWrapper
{
public:
  PcmWavSink();
  PcmWavSink(const char *filename, int format);

  bool open_file(const char *filename, int format);
  void close_file();
  bool is_file_open();

  // Sink interface
  virtual bool can_open(Speakers spk) const;
  virtual bool open(Speakers spk);

protected:
  SinkFilter linear_sink;
  WAVSink wav;
  Converter conv;
};

#endif
