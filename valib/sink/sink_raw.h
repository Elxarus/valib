/*
  RAW file output audio sink
*/

#ifndef SINK_RAW_H
#define SINK_RAW_H

#include "sink.h"
#include "auto_file.h"

class RAWSink : public NullSink
{
protected:
  AutoFile f;

public:
  RAWSink() 
  {}

  RAWSink(const char *_filename): 
  f(_filename, "wb") 
  {}

  /////////////////////////////////////////////////////////
  // RAWSink interface

  bool open_file(const char *_filename)
  {
    return f.open(_filename, "wb");
  }

  void close_file()
  {
    close();
    f.close();
  }

  bool is_file_open() const
  {
    return f.is_open();
  }

  /////////////////////////////////////////////////////////
  // AudioSink interface

  // Device open/close
  virtual bool query(Speakers _spk) const  
  { 
    // cannot write linear format
    return _spk.format != FORMAT_LINEAR;
  }

  // data write
  virtual bool write(const Chunk *_chunk)               
  { 
    if (!receive_chunk(_chunk))
      return false;

    return f.write(_chunk->get_rawdata(), _chunk->get_size()) != 0;
  }
};



#endif