/*
  Simple buffer helper class
*/

#ifndef VALIB_AUTO_BUF_H
#define VALIB_AUTO_BUF_H

#include <string.h>
#include "defs.h"

template <class T> class AutoBuf
{
private:
  T *f_buf;
  size_t f_size;

  AutoBuf(const AutoBuf &);
  AutoBuf &operator =(const AutoBuf &);

public:
  AutoBuf(): f_buf(0), f_size(0)
  {}

  AutoBuf(size_t size): f_buf(0), f_size(0)
  {
    allocate(size);
  }

  ~AutoBuf()
  { 
    drop();
  }

  inline T *allocate(size_t size)
  {
    drop();
    f_buf = new T[size];
    if (f_buf)
      f_size = size;
    return f_buf;
  }

  inline void drop()
  {
    safe_delete(f_buf);
    f_size = 0;
  }

  inline void zero()
  {
    if (f_buf)
      memset(f_buf, 0, sizeof(T) * f_size);
  }

  inline size_t size() { return f_size; }
  inline T *data()     { return f_buf;  }
  inline operator T*() { return f_buf;  }
};

typedef AutoBuf<uint8_t> Rawdata;
typedef AutoBuf<sample_t> Samples;

#endif
