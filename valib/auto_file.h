/*
  Simple file interface
*/

#ifndef AUTO_FILE_H
#define AUTO_FILE_H

#include <stdio.h>

class AutoFile
{
protected:
  FILE *f;
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
  ~AutoFile()
  {
    if (f) 
      fclose(f);
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
    }
    return is_open();
  }
  void close()
  {
    if (f)
      fclose(f);
  }
  bool is_open()
  {
    return f != 0;
  }

  int  read(void *buf, unsigned size)  { return fread(buf, 1, size, f);  }
  int  write(void *buf, unsigned size) { return fwrite(buf, 1, size, f); }

  bool eof()   const { return f? feof(f) != 0: true;  }
  int  size()  const { return filesize;               }
  FILE *fh()   const { return f;                      }
  int  pos()   const { return ftell(f);               }
};

#endif