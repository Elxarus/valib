/**************************************************************************//**
  \file auto_buf.h
  \brief AutoBuf class
******************************************************************************/


#ifndef VALIB_AUTO_BUF_H
#define VALIB_AUTO_BUF_H

#include <string.h>
#include "defs.h"

/**************************************************************************//**
  \class AutoBuf
  \brief Simple buffer helper class

  Implements a "rubber" buffer. We can increase and decrease the size of
  the buffer without excessive memory allocations. Buffer grows only when
  nessesary and does not actually shrink.

  Does not allow assignment and ownership transfers.
  Strictly exception-safe.

  \fn AutoBuf::AutoBuf()
    Creates an empty buffer with no data.
    Does not throw.

  \fn AutoBuf::AutoBuf(size_t size)
    \param size The size of the buffer to allocate

    Creates and allocates a buffer.
    Can throw std::bad_alloc.

  \fn AutoBuf::~AutoBuf()
    Frees the memory allocated.

  \fn T *AutoBuf::allocate(size_t size)
    \param size The size of the buffer to allocate

    Allocate at least 'size' elements of type T. Previous content of the buffer
    may be lost.
    Can throw std::bad_alloc.

  \fn T *AutoBuf::reallocate(size_t size)
    \param size The new size of the buffer

    Allocate at least 'size' elements of type T. Previous content of the buffer
    is preserved.

    When buffer grows, newly allocated data (after the end of the previous
    content) is not initialized and may be junk.

    When buffer shrinks, the data that does not fit the new size is not
    actually lost, because actual memory reallocation does not occur. But we
    never should rely on this! Only the data in range of the buffer size should
    be considered as actually allocated.

    Can throw std::bad_alloc.

  \fn void AutoBuf::free()
    Release the memory allocated.
    Does not throw.

  \fn void AutoBuf::zero()
    Fill the memory allocated with zeros.

  \fn size_t AutoBuf::size() const
    Returns the size of the buffer (number of elements of type T).

  \fn size_t AutoBuf::allocated() const
    Returns the size of actually allocated memory (number of elements of
    type T). May be more than size().

  \fn bool AutoBuf::is_allocated() const
    Returns true when buffer is allocated.

  \fn T *AutoBuf::begin() const
    Pointer to the beginning of the buffer

  \fn T *AutoBuf::end() const
    Pointer to an element just after the end of the buffer.

  \fn AutoBuf::operator T*() const
    Cast to pointer to the first element of the buffer. So we can
    use the buffer everywhere instead of T* pointer.
******************************************************************************/

template <class T> class AutoBuf
{
private:
  T *f_buf;
  size_t f_size;
  size_t f_allocated;

  // Non-copyable
  AutoBuf(const AutoBuf &);
  AutoBuf &operator =(const AutoBuf &);

public:
  AutoBuf(): f_buf(0), f_size(0), f_allocated(0)
  {}

  AutoBuf(size_t size): f_buf(0), f_size(0), f_allocated(0)
  {
    allocate(size);
  }

  ~AutoBuf()
  { 
    free();
  }

  inline T *allocate(size_t size)
  {
    if (f_allocated < size)
    {
      T *new_buf = new T[size];
      // no exceptions after this point
      safe_delete(f_buf);
      f_buf = new_buf;
      f_allocated = size;
    }
    f_size = size;
    return f_buf;
  }

  inline T *reallocate(size_t size)
  {
    if (f_allocated < size)
    {
      T *new_buf = new T[size];
      memcpy(new_buf, f_buf, f_size * sizeof(T));
      // no exceptions after this point
      safe_delete(f_buf);
      f_buf = new_buf;
      f_allocated = size;
    }
    f_size = size;
    return f_buf;
  }

  inline void free() throw()
  {
    safe_delete(f_buf);
    f_size = 0;
    f_allocated = 0;
  }

  inline void zero()
  {
    if (f_buf)
      memset(f_buf, 0, f_size * sizeof(T));
  }

  inline size_t size()       const { return f_size; }
  inline size_t allocated()  const { return f_allocated; }
  inline bool is_allocated() const { return f_buf != 0; }

  inline T *begin()    const { return f_buf; }
  inline T *end()      const { return f_buf + f_size; }
  inline operator T*() const { return f_buf;  }
};

#endif
