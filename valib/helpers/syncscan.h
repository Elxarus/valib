#ifndef SYNCSCAN_H
#define SYNCSCAN_H

#include <defs.h>

typedef uint32_t synctbl_t;

class SyncScan
{
protected:
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
  bool   clear(int index);
  void   clear_all();

  void   reset();
  int    get_sync();
  size_t scan(uint8_t *buf, size_t size);
};

// MPA/AC3/DTS sync scanner
class MADSyncScan : public SyncScan
{
public:
  MADSyncScan();
};

#endif
