/*
  WAV file source
*/

#ifndef WAV_SOURCE_H
#define WAV_SOURCE_H

#include "../buffer.h"
#include "../auto_file.h"
#include "../source.h"
#include "../win32/winspk.h"

class WAVSource : public Source2
{
protected:
  AutoFile f;
  Rawdata  buf;
  Speakers spk;
  Rawdata format;
  
  size_t block_size;

  AutoFile::fsize_t data_start;
  uint64_t          data_size;
  uint64_t          data_remains;

  bool open_riff();

public:
  WAVSource();
  WAVSource(const char *filename, size_t block_size);

  bool open(const char *filename, size_t block_size);
  void close();
  bool is_open() const;

  AutoFile::fsize_t size() const;
  AutoFile::fsize_t pos() const;
  int seek(AutoFile::fsize_t pos);
  const WAVEFORMATEX *wave_format() const;

  /////////////////////////////////////////////////////////
  // Source interface

  virtual bool get_chunk(Chunk2 &out);

  virtual bool new_stream() const
  { return false; }

  virtual Speakers get_output() const
  { return spk; }
};

#endif
