#include <string.h>
#include <spk.h>
#include <log.h>

#include <helpers\syncscan.h>
#include <source\noise.h>
#include <win32\cpu.h>

SyncScan s;

static const int syncwords[] = 
{
  0x0b770000,
  0x770b0000,
  0xff1f00e8,
  0x1fffe800,
  0xfe7f0180,
  0x7ffe8001,

  0xfff00000, 0xfff10000, 0xfff20000, 0xfff30000,
  0xfff40000, 0xfff50000, 0xfff60000, 0xfff70000,
  0xfff80000, 0xfff90000, 0xfffa0000, 0xfffb0000,
  0xfffc0000, 0xfffd0000, 0xfffe0000, 0xffff0000,

  0xf0ff0000, 0xf1ff0000, 0xf2ff0000, 0xf3ff0000,
  0xf4ff0000, 0xf5ff0000, 0xf6ff0000, 0xf7ff0000,
  0xf8ff0000, 0xf9ff0000, 0xfaff0000, 0xfbff0000,
  0xfcff0000, 0xfdff0000, 0xfeff0000, 0xffff0000,
};

static const int syncindex[] = 
{
  0, 
  1,
  2,
  3,
  4,
  5,

  6, 6, 6, 6, 
  6, 6, 6, 6, 
  6, 6, 6, 6, 
  6, 6, 6, 6, 

  7, 7, 7, 7, 
  7, 7, 7, 7, 
  7, 7, 7, 7, 
  7, 7, 7, 7, 
};

const max_syncwords = array_size(syncwords);

int 
test_syncer(Log *log)
{
  log->open_group("Testing Syncer");

  int i;
  for (i = 0; i < array_size(syncwords); i++)
    s.set(syncindex[i], syncwords[i], 0xffffffff);

  const int max_ptr_offset = 32;
  const int max_block_size = 32;
  const int max_offset = 128;
  const int buf_size = max_ptr_offset + max_block_size + max_offset + 16;

  uint8_t *buf = new uint8_t[buf_size];
  uint8_t scanbuf[4];

  //////////////////////////////////////////////////////////
  // Syncpoint find test

  for (int isyncword = 0; isyncword < max_syncwords; isyncword++)
  {
    log->status("syncword: %x", syncwords[isyncword]);
    for (int ptr_offset = 0; ptr_offset < max_ptr_offset; ptr_offset++)
      for (int block_size = 1; block_size < max_block_size; block_size++)
        for (int offset = 0; offset < max_offset; offset++)
        {
          memset(buf, 0, buf_size);
          *(uint32_t *)(buf + ptr_offset + offset) = swab_u32(syncwords[isyncword]);
          s.reset();

          for (int i = 0; i < offset + 16; i += block_size)
          {
            int gone = s.scan(buf + i + ptr_offset, block_size);

            if (s.get_sync())
            {
              if (gone > block_size)
              {
                log->msg("syncword: %x ptr.offset: %i block size: %i offset: %i", syncwords[isyncword], ptr_offset, block_size, offset);
                log->err("Too much bytes gone: %i", gone);
                break;
              }

              if (i + gone - 4 != offset)
              {
                log->msg("syncword: %x ptr.offset: %i block size: %i offset: %i", syncwords[isyncword], ptr_offset, block_size, offset);
                log->err("Sync found at %i", i+gone-4);
              }

              if (s.count != 4)
              {
                log->msg("syncword: %x ptr.offset: %i block size: %i offset: %i", syncwords[isyncword], ptr_offset, block_size, offset);
                log->err("Count = %i", s.count);
              }

              if ((s.get_sync() & (1 << syncindex[isyncword])) == 0)
              {
                log->msg("syncword: %x ptr.offset: %i block size: %i offset: %i", syncwords[isyncword], ptr_offset, block_size, offset);
                log->err("Wrong sync = %i", s.count);
              }
              break;
            }
            else
              if (gone != block_size)
              {
                log->msg("syncword: %x ptr.offset: %i block size: %i offset: %i", syncwords[isyncword], ptr_offset, block_size, offset);
                log->err("Wrong number of bytes gone: %i", gone);
                break;
              }

          }
          if (!s.get_sync())
          {
            log->msg("syncword: %x ptr.offset: %i block size: %i offset: %i", syncwords[isyncword], ptr_offset, block_size, offset);
            log->err("Sync was not found");
          }
        }
  }

  //////////////////////////////////////////////////////////
  // Syncpoint find test

  for (isyncword = 0; isyncword < max_syncwords; isyncword++)
  {
    log->status("syncword: %x", syncwords[isyncword]);
    for (int ptr_offset = 0; ptr_offset < max_ptr_offset; ptr_offset++)
      for (int block_size = 1; block_size < max_block_size; block_size++)
        for (int offset = 0; offset < max_offset; offset++)
        {
          memset(buf, 0, buf_size);
          *(uint32_t *)(buf + ptr_offset + offset) = swab_u32(syncwords[isyncword]);
          s.reset();

          for (int i = 0; i < offset + 16; i += block_size)
          {
            int gone = s.scan(scanbuf, buf + i + ptr_offset, block_size);

            if (s.get_sync(scanbuf))
            {
              if (gone > block_size)
              {
                log->msg("syncword: %x ptr.offset: %i block size: %i offset: %i", syncwords[isyncword], ptr_offset, block_size, offset);
                log->err("Too much bytes gone: %i", gone);
                break;
              }

              if (i + gone - 4 != offset)
              {
                log->msg("syncword: %x ptr.offset: %i block size: %i offset: %i", syncwords[isyncword], ptr_offset, block_size, offset);
                log->err("Sync found at %i", i+gone-4);
              }

              if ((s.get_sync(scanbuf) & (1 << syncindex[isyncword])) == 0)
              {
                log->msg("syncword: %x ptr.offset: %i block size: %i offset: %i", syncwords[isyncword], ptr_offset, block_size, offset);
                log->err("Wrong sync = %i", s.count);
              }
              break;
            }
            else
              if (gone != block_size)
              {
                log->msg("syncword: %x ptr.offset: %i block size: %i offset: %i", syncwords[isyncword], ptr_offset, block_size, offset);
                log->err("Wrong number of bytes gone: %i", gone);
                break;
              }

          }
          if (!s.get_sync(scanbuf))
          {
            log->msg("syncword: %x ptr.offset: %i block size: %i offset: %i", syncwords[isyncword], ptr_offset, block_size, offset);
            log->err("Sync was not found");
          }
        }
  }

  delete buf;

  //////////////////////////////////////////////////////////
  // Speed test

  s.clear_all();
  s.set_mad();

  const int runs = 50;
  const int size = 10000000;

  Chunk chunk;
  Noise noise(spk_unknown, size, size);
  noise.get_chunk(&chunk);

  CPUMeter cpu;
  int sync_count;

  sync_count = 0;
  cpu.reset();
  cpu.start();
  for (i = 0; i < runs; i++)
  {
    s.reset();
    size_t gone = 0;
    while (gone < chunk.size)
    {
      gone += s.scan(chunk.rawdata + gone, chunk.size - gone);
      if (s.get_sync())
        sync_count++;
    }
  }
  cpu.stop();

  log->msg("Sync scan speed: %iMB/s, Syncpoints found: %i", 
    int(double(chunk.size) * runs / cpu.get_thread_time() / 1000000), 
    sync_count / runs);

  sync_count = 0;
  cpu.reset();
  cpu.start();
  for (i = 0; i < runs; i++)
  {
    s.reset();
    size_t gone = 0;
    while (gone < chunk.size)
    {
      gone += s.scan(scanbuf, chunk.rawdata + gone, chunk.size - gone);
      if (s.get_sync(scanbuf))
        sync_count++;
    }
  }
  cpu.stop();

  log->msg("Sync scan speed: %iMB/s, Syncpoints found: %i", 
    int(double(chunk.size) * runs / cpu.get_thread_time() / 1000000), 
    sync_count / runs);


  return log->close_group();
};
