/*
  Sync helper class.
  Received timestamp is for the first syncpoint in the chunk.
*/

#ifndef VALIB_SYNC_H
#define VALIB_SYNC_H

#include "filter.h"

///////////////////////////////////////////////////////////////////////////////
// SyncHelper
// Helper class that holds sync information and makes correct timestamps for
// output chunks.
//
// When a filter buffers some input data, it has to track timestamps correctly.
// To track timestamps filter has to know how much data is buffered at output
// buffer and when data is removed from there.
//
// There're 2 methods of tracking timestamps:
// 1) Timestamp is applied to the first syncpoint in the chunk. But when we
//    have a compressed stream, syncpoint (beginning of a frame) may be in the
//    middle of the chunk. So if we want to form output chunks with one frame
//    in a chunk, we have to move timestamps like following:
//
//   t1           t2           t3
//   v            v            v
//   +------------+------------+------------+
//   |   chunk1   |   chunk2   |   chunk3   |
//   +------------+------------+------------+
//
//   +--------+--------+--------+--------+--
//   | frame1 | frame2 | frame3 | frame4 |
//   +--------+--------+--------+--------+--
//   ^        ^        ^        ^
//   t1    no time     t2       t3
//
// 2) When a filter buffers some data, it may need to update timestamps:
//
//   t1           t2           t3
//   v            v            v
//   +------------+------------+------------+
//   |   chunk1   |   chunk2   |   chunk3   |
//   +------------+------------+------------+
//
//   +--------+--------+--------+--------+--
//   | chunk1 | chunk2 | chunk3 | chunk4 |
//   +--------+--------+--------+--------+--
//   ^        ^   ^    ^        ^
//   t1    no time|    t2+5     t1+1
//                |    |
//                <---->
//             buffered data
//
// For linear format it is possible to track buffered data automatically, just
// by counting input and output samples. (But this method cannot be applied to
// sample rate conversion).
//
// receive_sync(Chunk *chunk, size_t pos)
//   Receive sync info from the input chunk.
//   This syncpoint will be applied to the buffer position pos.
//
// send_frame_sync(Chunk *chunk)
//   Timestamp the output chunk if nessesary.
//   Chunk will be stamped with the first timestamp in the queue.
//   This is applicable when chunks are frames of compressed data and we must
//   apply the timestamp to the first frame after the timestamp received.
//
// send_sync(Chunk *chunk, double size_to_time)
//   Timestamp the output chunk if nessesary.
//   Chunk will be stamped with the first timestamp in the queue, shifted by
//   the amount of data between the timestamp received and the chunk's start.
//   Applicable for linear and PCM data:
//
// drop(int size)
//   Drop buffered data. Required to track suncpoint positions.
//
// reset()
//   Drop all timing information.


class SyncHelper
{
protected:
  bool    sync[2]; // timestamp exists
  vtime_t time[2]; // timestamp
  pos_t   pos[2];  // buffer position for timestamp
  pos_t   nsamples;// number of buffered samples (for linear format only)

  inline void shift();

public:
  SyncHelper()
  { reset(); }

  // Track timestamps automatically by counting samples
  inline void receive_linear(Chunk &chunk);
  inline void send_linear(Chunk &chunk, int sample_rate);

  // Track timestamps manually
  inline void receive_sync(const Chunk &chunk, pos_t pos);
  inline void send_frame_sync(Chunk &chunk);
  inline void send_sync(Chunk &chunk, double size_to_time);

  inline void drop(size_t size);
  inline void reset();
};

///////////////////////////////////////////////////////////////////////////////

inline void
SyncHelper::receive_linear(Chunk &chunk)
{
  if (chunk.sync)
  {
    if (sync[0] && pos[0] != nsamples)
    {
      assert(pos[0] < nsamples);
      sync[1] = true;
      time[1] = chunk.time;
      pos[1]  = nsamples;
    }
    else
    {
      sync[0] = true;
      time[0] = chunk.time;
      pos[0]  = nsamples;
    }
  }
  nsamples += chunk.size;
}

inline void
SyncHelper::send_linear(Chunk &chunk, int sample_rate)
{
  assert((size_t)nsamples >= chunk.size);
  nsamples -= chunk.size;

  if (pos[0] <= 0)
  {
    chunk.sync = sync[0];
    chunk.time = time[0] - vtime_t(pos[0]) / sample_rate;
    shift();
  }
  drop(chunk.size);
}

inline void
SyncHelper::shift()
{
  sync[0] = sync[1];
  time[0] = time[1];
  pos[0]  = pos[1];
  sync[1] = false;
  time[1] = 0;
  pos[1]  = 0;
}

inline void
SyncHelper::receive_sync(const Chunk &chunk, pos_t _pos)
{
  if (chunk.sync)
  {
    if (sync[0] && pos[0] != _pos)
    {
      assert(pos[0] < _pos);
      sync[1] = true;
      time[1] = chunk.time;
      pos[1]  = _pos;
    }
    else
    {
      sync[0] = true;
      time[0] = chunk.time;
      pos[0]  = _pos;
    }
  }
}

inline void
SyncHelper::send_frame_sync(Chunk &chunk)
{
  if (pos[0] <= 0)
  {
    chunk.sync = sync[0];
    chunk.time = time[0];
    shift();
  }
}

inline void
SyncHelper::send_sync(Chunk &chunk, double size_to_time)
{
  if (pos[0] <= 0)
  {
    chunk.sync = sync[0];
    chunk.time = time[0] - pos[0] * size_to_time;
    shift();
  }
}

inline void
SyncHelper::drop(size_t size)
{
  if (sync[0])
    pos[0] -= size;

  if (sync[1])
  {
    pos[1] -= size;
    if (pos[1] <= 0)
      shift();
  }
}

inline void
SyncHelper::reset()
{
  sync[0] = false;
  time[0] = 0;
  pos[0]  = 0;
  sync[1] = false;
  time[1] = 0;
  pos[1]  = 0;
  nsamples = 0;
}

#endif
