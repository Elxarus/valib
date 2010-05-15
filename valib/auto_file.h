/*
  AutoFile - Simple file helper class. Supports large files >2G.
  MemFile - Load the whole file into memory.

  AutoFile may support large files >2G (currently only in Visual C++). If you
  open a large file without large file support, file size is set to
  bad_size constant. In this case you can use the file, but cannot seek
  and get the file position over the limit. Note, that limit is only
  2Gb because position in standard library is signed and may not work
  correctly with negative positions.

  VC6 does not support uint64_t to double cast, therefoere we have to use
  the signed type int64_t.

  Note, that file size type, fsize_t may be larger than size_t (true on 32bit
  systems). It's because the file may be larger than memory address space.
  Therefore we cannot safely cast fsize_t to size_t and have to check the
  range before casting.

///////////////////////////////////////////////////////////////////////////////

  AutoFile::fsize_t
    Type to represent the file size. It is unsafe to store the size using
    other types (int, size_t, etc).

  static const fsize_t AutoFile::bad_size
    Constant to represent an incorrect file size.

  static bool is_large(fsize_t value)
    Returns false when file size cannot be cast to size_t and true otherwise.

  static size_t size_cast(fsize_t value)
    Cast fsize_t to size_t. You should use this function only when it is safe
    to cast (is_large() returns false). Otherwise, it returns size_t max value.

  AutoFile()
    Create file object without opening a file.

  AutoFile(const char *filename, const char *mode = "rb")
    Create and open a file. In case of failure, is_open() reports false.

  AutoFile(FILE *_f, bool take_ownership = false)
    Connect to an already open file. When take_ownership is true, file will
    be closed on destruction of an object or close() call.

  ~AutoFile()
    Automatically closes the file.

  bool open(const char *filename, const char *mode = "rb")
    Open the file. Returns false on failure.

  bool open(FILE *_f, bool _take_ownership = false)
    Connect to an already open file. When take_ownership is true, file will
    be closed on destruction of an object or close() call.

  void close()
    Close the file.

  size_t read(void *buf, size_t size)
    Read 'size' bytes to the buffer 'buf'.
    Returns number of bytes it actually read.

  size_t write(const void *buf, size_t size)
    Write 'size' bytes from buffer 'buf'.
    Returns number of bytes it actually wrote.

  bool is_open() const
    Returns true when file is open.

  bool eof() const
    Returns true when we reach the end of the file.

  fsize_t size() const
    Returns size of the file. When the size cannot be determined,
    the result is AutoFile::bad_size value.

  bool is_large() const
    File is large, i.e. it does not fit an address space and file size
    cannot be safely cast to size_t.

  FILE *fh() const
    Returns file handle value explicitly.

  operator FILE *() const
    Automatic cast to FILE *.

  int seek(fsize_t pos);
    Move to the file position 'pos'.
    Returns 0 on success and non-zero otherwise (same as fseek command).

  fsize_t pos() const;
    Returns current file position.
*/

#ifndef VALIB_AUTO_FILE_H
#define VALIB_AUTO_FILE_H

#include "defs.h"
#include <stdio.h>

class AutoFile
{
public:
  typedef int64_t fsize_t;
  static const fsize_t bad_size;
  static bool is_large(fsize_t value) { return value > (fsize_t)(size_t)-1; }
  static size_t size_cast(fsize_t value) { return is_large(value)? -1: (size_t)value; }

protected:
  FILE *f;
  bool own_file;
  fsize_t filesize;

  // Non-copyable
  AutoFile(const AutoFile &);
  AutoFile &operator =(const AutoFile &);


public:
  AutoFile(): f(0), filesize(0)
  {}

  AutoFile(const char *filename, const char *mode = "rb"): f(0), filesize(0)
  { open(filename, mode); }

  AutoFile(FILE *_f, bool _take_ownership = false): f(0), filesize(0)
  { open(_f, _take_ownership); }

  ~AutoFile()
  { close(); }

  bool open(const char *filename, const char *mode = "rb");
  bool open(FILE *_f, bool take_ownership = false);
  void close();

  inline size_t  read(void *buf, size_t size)        { return fread(buf, 1, size, f);  }
  inline size_t  write(const void *buf, size_t size) { return fwrite(buf, 1, size, f); }

  inline bool    is_open()  const { return f != 0;                }
  inline bool    eof()      const { return f? feof(f) != 0: true; }
  inline fsize_t size()     const { return filesize;              }
  inline bool    is_large() const { return is_large(filesize);    }

  inline FILE    *fh()      const { return f; }
  inline operator FILE *()  const { return f; }

  int seek(fsize_t pos);
  fsize_t pos() const;
};


class MemFile
{
protected:
  void *data;
  size_t file_size;

public:
  MemFile(const char *filename);
  ~MemFile();

  inline size_t size() const { return file_size; }
  inline operator uint8_t *() const { return (uint8_t *)data; }
};

#endif
