/*
  Sync helper class.
  Received timestamp is for the first syncpoint in the chunk.
*/

#ifndef SYNC_H
#define SYNC_H

#include "data.h"


class Sync
{
protected:
  bool    syncing; // syncing state
  bool    sync[2]; // timestamp exists
  vtime_t time[2]; // timestamp

public:
  Sync()
  {
    reset();
  }

  inline void receive_sync(bool _sync, vtime_t _time)
  {
    if (_sync)
      if (syncing)
      {
        sync[0] = true;
        time[0] = _time;
        sync[1] = false;
        time[1] = _time;
      }
      else
      {
        sync[1] = true;
        time[1] = _time;
      }
  }
  inline void receive_sync(const Chunk *chunk)
  {
    receive_sync(chunk->is_sync(), chunk->get_time());
  }

  inline void send_sync(Chunk *_chunk)
  {
    _chunk->set_sync(sync[0], time[0]);
    sync[0] = sync[1];
    time[0] = time[1];
    sync[1] = false;
  }

  inline bool is_syncing()
  {
    return syncing;
  }

  inline void set_syncing(bool _syncing)
  {
    syncing = _syncing;
  }

  inline void reset()
  {
    syncing = true;
    sync[0] = false;
    sync[1] = false;
  }
};

#endif