#include <memory.h>
#include "syncscan.h"

///////////////////////////////////////////////////////////////////////////////
// Syncronization table for MPA/AC3/DTS
// 0x01  MPA syncword 1: 0xff 0xfn
// 0x02  MPA syncword 2: 0xfn 0xff
// 0x04  AC3 syncword 1: 0x0b 0x77
// 0x08  AC3 syncword 1: 0x77 0x0b
// 0x10  DTS syncword 1: 0xff 0x1f 0x00 0xe8
// 0x20  DTS syncword 2: 0x1f 0xff 0xe8 0x00
// 0x40  DTS syncword 3: 0xfe 0x7f 0x01 0x80
// 0x80  DTS syncword 4: 0x7f 0xfe 0x80 0x01

static const int mad_sync[] = 
{
  0xfff00000, 0xfff00000,
  0xf0ff0000, 0xf0ff0000, 
  0x0b770000, 0xffff0000,
  0x770b0000, 0xffff0000,
  0xff1f00e8, 0xffffffff,
  0x1fffe800, 0xffffffff,
  0xfe7f0180, 0xffffffff,
  0x7ffe8001, 0xffffffff,
};

MADSyncScan::MADSyncScan()
{
  for (int i = 0; i < array_size(mad_sync); i+=2)
    set(i, mad_sync[i], mad_sync[i+1]);
}


SyncScan::SyncScan(uint32_t _syncword, uint32_t _syncmask)
{
  synctable = new synctbl_t[1024];
  memset(synctable, 0, sizeof(synctbl_t) * 1024);
  count = 0;

  if (_syncword)
    set(1, _syncword, _syncmask);
}

SyncScan::~SyncScan()
{
  if (synctable) 
    delete synctable;
}

bool
SyncScan::set(int _index, uint32_t _syncword, uint32_t _syncmask)
{
  if (_index < 0 || _index > sizeof(synctbl_t) * 8)
    return false;

  synctbl_t table_mask = (1 << _index);

  int i;
  uint8_t sync_byte;
  uint8_t mask_byte;

  sync_byte = (_syncword >> 24);
  mask_byte = (_syncmask >> 24);

  for (i = 0; i < 256; i++)
    if ((sync_byte & mask_byte) == (i & mask_byte))
      synctable[i] |= table_mask;

  sync_byte = (_syncword >> 16) & 0xff;
  mask_byte = (_syncmask >> 16) & 0xff;

  for (i = 0; i < 256; i++)
    if ((sync_byte & mask_byte) == (i & mask_byte))
      synctable[i + 256] |= table_mask;
  
  sync_byte = (_syncword >> 8) & 0xff;
  mask_byte = (_syncmask >> 8) & 0xff;

  for (i = 0; i < 256; i++)
    if ((sync_byte & mask_byte) == (i & mask_byte))
      synctable[i + 512] |= table_mask;
  
  sync_byte = _syncword & 0xff;
  mask_byte = _syncmask & 0xff;

  for (i = 0; i < 256; i++)
    if ((sync_byte & mask_byte) == (i & mask_byte))
      synctable[i + 768] |= table_mask;

  return true;
}

bool
SyncScan::clear(int _index)
{
  if (_index < 0 || _index > sizeof(synctbl_t) * 8)
    return false;

  synctbl_t table_mask = ~(1 << _index);
  for (int i = 0; i < 1024; i++)
    synctable[i] &= table_mask;

  return true;
}

void
SyncScan::clear_all()
{
  memset(synctable, 0, sizeof(synctbl_t) * 1024);
}

void 
SyncScan::reset()
{
  count = 0;
}

size_t 
SyncScan::scan(uint8_t *buf, size_t size)
{
  uint8_t *pos = buf;
  uint8_t *end = buf + size;

  ///////////////////////////////////////////////////////
  // Use local syncword

  uint32_t sync = swab_u32(syncword);                   

  ///////////////////////////////////////////////////////
  // Use local synctable
  // Macroses to access the synctable

  synctbl_t *st = synctable;

  #define st1(i) st[(i)]
  #define st2(i) st[(i) + 256]
  #define st3(i) st[(i) + 512]
  #define st4(i) st[(i) + 768]

  #define is_sync(s)            \
  (                             \
    st1(sync >> 24) &           \
    st2((sync >> 16) & 0xff) &  \
    st3((sync >> 8) & 0xff) &   \
    st4(sync & 0xff)            \
  )

  ///////////////////////////////////////////////////////
  // Resync

  if (count >= 4)
    count = 3;

  ///////////////////////////////////////////////////////
  // Fill the syncword up to 3 bytes

  if (size + count < 4)
  {
    // not enough data to fill the syncword
    switch (size)
    {
      case 3: sync = (sync << 8) | *pos++; count++;
      case 2: sync = (sync << 8) | *pos++; count++;
      case 1: sync = (sync << 8) | *pos++; count++;
    }
    syncword = swab_u32(sync);
    return size;
  }

  switch (count)
  {
    case 0: sync = (sync << 8) | *pos++;
    case 1: sync = (sync << 8) | *pos++;
    case 2: sync = (sync << 8) | *pos++;
  }

  ///////////////////////////////////////////////////////
  // Optimistically suppose that we will find the syncpoint

  count = 4;

  ///////////////////////////////////////////////////////
  // Process unaligned start

  while (((uint32_t)pos & 3) && (pos < end))
  {
    sync = (sync << 8) | *pos++;
    if (is_sync(sync))
    {
      syncword = swab_u32(sync);
      return pos - buf;
    }
  }

  ///////////////////////////////////////////////////////
  // Setup 32bit transfer

  uint32_t *pos32 = (uint32_t *)pos;
  uint32_t *end32 = (uint32_t *)((uint32_t)end & ~3);

  ///////////////////////////////////////////////////////
  // Process main block

  while (pos32 < end32)
  {
    uint32_t next = *pos32++;

    #define S1 ((sync >> 16) & 0xff)
    #define S2 ((sync >> 8) & 0xff)
    #define S3 (sync & 0xff)
    #define N4 (next & 0xff)
    #define N5 ((next >> 8) & 0xff)
    #define N6 ((next >> 16) & 0xff)
    #define N7 ((next >> 24) & 0xff)

    if (st1(S1) & st2(S2))
    if (st1(S1) & st2(S2) & st3(S3) & st4(N4))
    {
      sync = (sync << 8) | N4;
      syncword = swab_u32(sync);
      return (uint8_t *)pos32 - buf + 1 - 4;
    }

    if (st1(S2) & st2(S3))
    if (st1(S2) & st2(S3) & st3(N4) & st4(N5))
    {
      sync = (sync << 16) | (N4 << 8) | N5;
      syncword = swab_u32(sync);
      return (uint8_t *)pos32 - buf + 2 - 4;
    }

    if (st1(S3) & st2(N4))
    if (st1(S3) & st2(N4) & st3(N5) & st4(N6))
    {
      sync = (sync << 24) | (N4 << 16) | (N5 << 8) | N6;
      syncword = swab_u32(sync);
      return (uint8_t *)pos32 - buf + 3 - 4;
    }

    sync = swab_u32(next);

    if (st1(N4) & st2(N5))
    if (st1(N4) & st2(N5) & st3(N6) & st4(N7))
    {
      // sync is right
      syncword = swab_u32(sync);
      return (uint8_t *)pos32 - buf + 4 - 4;
    }
  }

  ///////////////////////////////////////////////////////
  // Process unaligned end

  pos = (uint8_t *)pos32;

  while (pos < end)
  {
    sync = (sync << 8) | *pos++;
    if (is_sync(sync))
    {
      syncword = swab_u32(sync);
      return pos - buf;
    }
  }

  ///////////////////////////////////////////////////////
  // Scan failed

  count = 3;
  syncword = swab_u32(sync);
  return size;
}

int 
SyncScan::get_sync()
{
  if (count == 4)
    return synctable[(syncword & 0xff)] & 
           synctable[((syncword >> 8) & 0xff) + 256] & 
           synctable[((syncword >> 16) & 0xff) + 512] & 
           synctable[(syncword >> 24) + 768];
  else
    return 0;
}
