/*
  RAW file output audio renderer
*/

#ifndef SINK_RAW_H
#define SINK_RAW_H

#include "sink.h"
#include "auto_file.h"

class RAWRenderer : public NullRenderer
{
protected:
  AutoFile f;

public:
  RAWRenderer() 
  {}

  RAWRenderer(const char *_filename): 
  f(_filename, "wb") 
  {}

  /////////////////////////////////////////////////////////
  // RAWRenderer interface

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
  // AudioRenderer interface

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

    return f.write(_chunk->get_rawdata(), _chunk->get_size()) == _chunk->get_size();
  }
};



#endif