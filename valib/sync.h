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
  bool   is_syncpoint;  // is it a syncpoint
  bool   timestamp[2];  // should we stamp next output chunk
  time_t time[2];       // time next output chunk should be stamped

public:
  Sync()
  {
    reset();
  }

  void receive_timestamp(bool _timestamp, time_t _time)
  {
    if (_timestamp)
      if (is_syncpoint)
      {
        timestamp[0] = true;
        time[0] = _time;
        timestamp[1] = false;
        time[1] = 0;
      }
      else
      {
        timestamp[1] = true;
        time[1] = _time;
      }
  }
  void receive_timestamp(const Chunk *chunk)
  {
    receive_timestamp(chunk->timestamp, chunk->time);
  }

  void set_time(Chunk *_chunk)
  {
    _chunk->set_time(timestamp[0], time[0]);
    timestamp[0] = timestamp[1];
    timestamp[1] = false;
    time[0] = time[1];
    time[1] = 0;   
  }

  void syncpoint(bool _is_syncpoint)
  {
    is_syncpoint = _is_syncpoint;
  }

  void reset()
  {
    is_syncpoint = true;
    timestamp[0] = false;
    timestamp[1] = false;
    time[0] = 0;
    time[1] = 1;
  }
};


#endif