/*
  RAW file output audio sink
*/

#ifndef SINK_RAW_H
#define SINK_RAW_H

#include "sink.h"

class RAWSink : public NullSink
{
protected:
  FILE *f;

public:
  RAWSink()
  {
    f = 0;
  }

  RAWSink(const char *filename)
  {
    f = 0;
    open_file(filename);
  }

  ~RAWSink()
  {
    close_file();
  }
   
  bool open_file(const char *filename)
  {
    close_file();
    f = fopen(filename, "wb");
    return f != 0;
  }

  void close_file()
  {
    close();
    if (f) fclose(f);
  }

  bool is_file_open() const
  {
    return f != 0;
  }

  // playback control
  virtual bool query(Speakers _spk) const  
  { 
    return f != 0 && _spk.format != FORMAT_LINEAR;  
  }

  // data write
  virtual bool write(const Chunk *chunk)               
  { 
    if (!f || paused) return false;

    fwrite(chunk->buf, 1, chunk->size, f);

    if (chunk->timestamp)
      time = chunk->time + chunk->size;
    else
      time += chunk->size;

    return true;
  }
};



#endif