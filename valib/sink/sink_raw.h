/*
  RAW file output audio renderer
*/

#ifndef VALIB_SINK_RAW_H
#define VALIB_SINK_RAW_H

#include "../sink.h"
#include "../auto_file.h"

class RAWSink : public SimpleSink
{
protected:
  AutoFile f;

public:
  RAWSink() 
  {}

  RAWSink(const char *_filename): 
  f(_filename, "wb") 
  {}

  RAWSink(FILE *_f): 
  f(_f) 
  {}

  /////////////////////////////////////////////////////////
  // RAWSink interface

  bool open_file(const char *_filename)
  {
    return f.open(_filename, "wb");
  }

  bool open_file(FILE *_f)
  {
    return f.open(_f);
  }

  void close_file()
  {
    f.close();
    close();
  }

  bool is_file_open() const
  {
    return f.is_open();
  }

  /////////////////////////////////////////////////////////
  // Sink interface

  virtual bool can_open(Speakers new_spk) const  
  { 
    // cannot write linear format
    return f.is_open() && new_spk.format != FORMAT_LINEAR;
  }

  virtual void process(const Chunk2 &chunk)               
  {
    f.write(chunk.rawdata, chunk.size);
  }
};

#endif
