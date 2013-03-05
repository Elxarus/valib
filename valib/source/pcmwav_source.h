/**************************************************************************//**
  \file pcmwav_source.h
  \brief PcmWavSource class
******************************************************************************/

#ifndef VALIB_PCMWAVSOURCE_H
#define VALIB_PCMWAVSOURCE_H

#include "wav_source.h"
#include "source_filter.h"
#include "../filters/convert.h"

/**************************************************************************//**
  \class PcmWavSource
  \brief Source for PCM wave files, returns FORMAT_LINEAR (no need to convert
    explicitly). Does not convert the level. I.e. when reading PCM16 file
    max signal amplitude will be 32767.5.

  \fn PcmWavSource::PcmWavSource();
    Creates uninitialied source. Call open_file() to initialize.

    Post conditions:
    - file_is_open() returns false

  \fn PcmWavSource::PcmWavSource(const char *filename, size_t chunk_size);
    \param filename File to open.
    \param chunk_size Size of output chunks.

    Constructs an initialized source (opens a file). Note, you must call
    is_file_open() explicitly to check 

  \fn bool PcmWavSource::open_file(const char *filename, size_t chunk_size);
    \param filename File to open.
    \param chunk_size Size of output chunks.

    Opens a wav file and initializes the source. Returns true on success and
    false otherwise.

  \fn void PcmWavSource::close_file();
    Closes the currently open file.

  \fn bool PcmWavSource::is_file_open() const;
    Returns true when a wav file is open.

******************************************************************************/

class PcmWavSource : public SourceWrapper
{
public:
  PcmWavSource();
  PcmWavSource(const char *filename, size_t chunk_size = 2048);

  bool open_file(const char *filename, size_t chunk_size = 2048);
  void close_file();
  bool is_file_open() const;

protected:
  SourceFilter source;
  WAVSource wav;
  Converter conv;
};

#endif
