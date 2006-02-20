/*
  SyncScan
  Syncronization scanner.

  Searches for 32bit syncronization words.
  May search for multiple words at the same time.
  Requires a buffer to store intermediate results.
*/

#ifndef SYNCSCAN_H
#define SYNCSCAN_H

#include <defs.h>


class SyncScan
{
protected:
  typedef uint32_t synctbl_t;
  synctbl_t *synctable;

public:
  union
  {
    uint8_t  syncbuf[4];
    uint32_t syncword;
  };
  size_t count;

public:
  SyncScan(uint32_t syncword = 0, uint32_t syncmask = 0);
  ~SyncScan();

  bool   set(int index, uint32_t syncword, uint32_t syncmask);
  bool   set_list(const uint32_t *list, size_t size);
  bool   set_mad();

  bool   clear(int index);
  void   clear_all();

  void   reset();
  uint32_t get_sync() const;
  uint32_t get_sync(uint8_t *buf) const;

  size_t scan(uint8_t *buf, size_t size);
  size_t scan(uint8_t *syncword, uint8_t *buf, size_t size) const;
};

#endif
