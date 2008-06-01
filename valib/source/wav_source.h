/*
  WAV file source
*/

#ifndef WAV_SOURCE_H
#define WAV_SOURCE_H

#include "../filter.h"
#include "../data.h"
#include "../auto_file.h"

class WAVSource : public Source
{
protected:
  AutoFile f;
  DataBuf  buf;
  Speakers spk;

  size_t block_size;

  int data_start;
  int data_size;
  size_t data_remains;

  bool open_riff();

public:
  WAVSource();
  WAVSource(const char *filename, size_t block_size);

  bool open(const char *filename, size_t block_size);
  void close();
  bool is_open() const;

  int size() const;
  int pos() const;
  void seek(int pos);

  /////////////////////////////////////////////////////////
  // Source interface

  Speakers get_output() const;
  bool is_empty() const;
  bool get_chunk(Chunk *chunk);
};

#endif
