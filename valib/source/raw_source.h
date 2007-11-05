#ifndef VALIB_RAW_SOURCE
#define VALIB_RAW_SOURCE

#include "../data.h"
#include "../filter.h"
#include "../auto_file.h"

class RAWSource: public Source
{
protected:
  AutoFile f;
  Speakers spk;
  DataBuf  buf;
  size_t   block_size;

public:
  RAWSource()
  {}
 
  RAWSource(Speakers _spk, const char *_filename, size_t _block_size = 65536)
  { open (_spk, _filename, _block_size); }

  RAWSource(Speakers _spk, FILE *_f, size_t _block_size = 65536)
  { open (_spk, _f, _block_size); }

  /////////////////////////////////////////////////////////
  // FileSource interface

  bool open(Speakers _spk, const char *_filename, size_t _block_size = 65536);
  bool open(Speakers _spk, FILE *_f, size_t _block_size = 65536);
  void close();

  inline void seek(size_t _pos) { f.seek(_pos);       }

  inline bool is_open() const   { return f.is_open(); }
  inline bool eof()     const   { return f.eof();     }
  inline int  size()    const   { return f.size();    }
  inline int  pos()     const   { return f.pos();     }
  inline FILE *fh()     const   { return f.fh();      }

  /////////////////////////////////////////////////////////
  // Source interface

  virtual Speakers get_output() const  { return spk;               }
  virtual bool is_empty() const        { return !f.is_open() || f.eof(); }
  virtual bool get_chunk(Chunk *chunk);

};

#endif
