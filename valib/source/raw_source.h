/*
  Raw file source
*/

#ifndef VALIB_RAW_SOURCE
#define VALIB_RAW_SOURCE

#include "../buffer.h"
#include "../auto_file.h"
#include "../source.h"

class RAWSource: public Source
{
protected:
  AutoFile f;
  Speakers spk;
  Rawdata  buf;
  size_t   block_size;

public:
  typedef AutoFile::fsize_t fsize_t;

  RAWSource()
  {}
 
  RAWSource(Speakers spk_, const char *filename_, size_t block_size_ = 65536)
  { open(spk_, filename_, block_size_); }

  RAWSource(Speakers spk_, FILE *_f, size_t block_size_ = 65536)
  { open(spk_, _f, block_size_); }

  /////////////////////////////////////////////////////////
  // FileSource interface

  bool open(Speakers spk_, const char *filename_, size_t block_size_ = 65536);
  bool open(Speakers spk_, FILE *_f, size_t block_size_ = 65536);
  void close();

  inline bool    is_open() const { return f.is_open(); }
  inline bool    eof()     const { return f.eof();     }
  inline fsize_t size()    const { return f.size();    }
  inline FILE   *fh()      const { return f.fh();      }

  inline int seek(fsize_t _pos) { return f.seek(_pos); }
  inline fsize_t pos() const    { return f.pos();      }

  /////////////////////////////////////////////////////////
  // Source interface

  virtual bool get_chunk(Chunk &out);

  virtual void reset()
  { seek(0); }

  virtual bool new_stream() const
  { return false; }

  virtual Speakers get_output() const
  { return spk; }
};

#endif
