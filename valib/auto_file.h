/*
  Simple file helper class
*/

#ifndef AUTO_FILE_H
#define AUTO_FILE_H

#include "defs.h"  // rquired to disable depreciation warning in msvc8 (use compiler-dependent options)
#include <stdio.h>

class AutoFile
{
protected:
  FILE *f;
  bool close_file;
  int filesize;

public:
  AutoFile()
  {
    f = 0;
  }
  AutoFile(const char *filename, const char *mode = "rb")
  {
    f = 0;
    open(filename, mode);
  }
  AutoFile(FILE *_f)
  {
    f = 0;
    open(_f);
  }

  ~AutoFile()
  {
    close();
  };

  bool open(const char *filename, const char *mode = "rb")
  {
    if (f) close();
    filesize = 0;
    if (f = fopen(filename, mode))
    {
      fseek(f, 0, SEEK_END);
      filesize = ftell(f);
      fseek(f, 0, SEEK_SET);
      close_file = true;
    }
    return is_open();
  }

  bool open(FILE *_f)
  {
    if (f) close();
    filesize = 0;
    f = _f;
    if (f)
    {
      long old_pos = ftell(f);
      fseek(f, 0, SEEK_END);
      filesize = ftell(f);
      fseek(f, old_pos, SEEK_SET);
      close_file = false;
    }
    return is_open();
  }

  void close()
  {
    if (f && close_file)
      fclose(f);
    f = 0;
  }

  inline void   seek(int _pos)                { fseek(f, _pos, SEEK_SET);       }
  inline size_t read(void *buf, size_t size)  { return fread(buf, 1, size, f);  }
  inline size_t write(void *buf, size_t size) { return fwrite(buf, 1, size, f); }

  inline bool is_open() const { return f != 0;                }
  inline bool eof()     const { return f? feof(f) != 0: true; }
  inline int  size()    const { return filesize;              }
  inline int  pos()     const { return ftell(f);              }
  inline FILE *fh()     const { return f;                     }

  inline operator FILE *() const { return f; }
};

#endif
