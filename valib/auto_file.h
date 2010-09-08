/**************************************************************************//**
  \file auto_file.h
  \brief File helper classes
******************************************************************************/

#ifndef VALIB_AUTO_FILE_H
#define VALIB_AUTO_FILE_H

#include "defs.h"
#include <stdio.h>

/**************************************************************************//**
  \class AutoFile
  \brief Simple file helper class. Supports large files >2G.

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

  \typedef AutoFile::fsize_t
    Type to represent the file size. It is unsafe to store the size using
    other types (int, size_t, etc).

  \var static const fsize_t AutoFile::bad_size
    Constant to represent an incorrect file size.

  \fn static bool AutoFile::is_large(fsize_t value)
    \param value File size to check

    Returns false when file size cannot be cast to size_t and true otherwise.

  \fn static size_t AutoFile::size_cast(fsize_t value)
    \param value File size to cast

    Cast fsize_t to size_t. You should use this function only when it is safe
    to cast (is_large() returns false). Otherwise, it returns size_t max value.

  \fn AutoFile::AutoFile()
    Create file object without opening a file.

  \fn AutoFile::AutoFile(const char *filename, const char *mode = "rb")
    \param filename File name to open
    \param mode Open mode (equivalent to the mode at fopen()).

    Create and open a file. In case of failure, is_open() reports false.

  \fn AutoFile::AutoFile(FILE *f, bool take_ownership = false)
    \param f File to connect to
    \param take_ownership Take ownership of the file

    Connect to an already open file. When take_ownership is true, file will
    be closed on destruction of an object or close() call.

  \fn AutoFile::~AutoFile()
    Automatically closes the file.

  \fn bool AutoFile::open(const char *filename, const char *mode = "rb")
    \param filename File name to open
    \param mode Open mode (equivalent to the mode at fopen()).
    \return Returns true on success and false otherwise.

    Open the file.

  \fn bool AutoFile::open(FILE *f, bool take_ownership = false)
    \param f File to connect to
    \param take_ownership Take ownership of the file
    \return Returns true on success and false otherwise.

    Connect to an already open file. When take_ownership is true, file will
    be closed on destruction of an object or close() call.

  \fn void AutoFile::close()
    Close the file.

  \fn size_t AutoFile::read(void *buf, size_t size)
    \param buf  Buffer to read to.
    \param size Size fo data to read.
    \return     Number of bytes  actually read.

    Read 'size' bytes into the buffer 'buf'.

    Returns number of bytes it actually read that may be less than 'size' at
    the end of the file.

  \fn size_t AutoFile::write(const void *buf, size_t size)
    \param buf  Buffer to write data from.
    \param size Size of the data at the buffer.
    \return     Number of actually written bytes.

    Write 'size' bytes from buffer 'buf'.

    Returns number of bytes it actually wrote.

  \fn bool AutoFile::is_open() const
    \return Returns true when file is open and false otherwise.

  \fn bool AutoFile::eof() const
    \return Returns true when we reach the end of the file and false otherwise.

  \fn fsize_t AutoFile::size() const
    \return Returns size of the file.

    When the size cannot be determined, the result is AutoFile::bad_size value.

  \fn bool AutoFile::is_large() const
    \return Returns true when file is large and false otherwise.

    File is large when it does not fit an address space and file size cannot
    be safely cast to size_t.

  \fn FILE *AutoFile::fh() const
    \return File handle.

    Returns file handle value explicitly.

    Returns 0 when file was not open.

  \fn AutoFile::operator FILE *() const
    \return File handle.

    Automatic cast to FILE *.

    Returns 0 when file was not open.

  \fn int AutoFile::seek(fsize_t pos);
    \param pos File position to move to.
    \return Returns 0 on success and non-zero otherwise (same as fseek command).

    Move to the file position 'pos'.

  \fn fsize_t AutoFile::pos() const;
    \return Returns current file position.
******************************************************************************/

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


/**************************************************************************//**
  \class MemFile
  \brief Load the whole file into memory.

  \fn MemFile::MemFile(const char *filename)
    \param filename File name to load.
    Allocate a buffer and load the whole file into memory.

    Can throw std::bad_alloc.

  \fn MemFile::~MemFile()
    Free the memory allocated.

  \fn size_t MemFile::size() const
    \return The size of the buffer (size of the file loaded).
    Returns 0 when file was not loaded.

  \fn MemFile::operator uint8_t *() const
    \return Pointer to the start of the buffer.
    Returns 0 when file was not loaded.

******************************************************************************/


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
