/**************************************************************************//**
  \file sync.h
  \brief SyncHelper: Synchronization helper class
******************************************************************************/

#ifndef VALIB_SYNC_H
#define VALIB_SYNC_H

#include "filter.h"

/**************************************************************************//**
  \class SyncHelper
  \brief Synchronization helper class

  Helper class that holds sync information and makes correct timestamps for
  output chunks.

  When a filter buffers some input data, it has to track timestamps correctly.
  To track timestamps filter has to know how much data is buffered at output
  buffer and when data is removed from there.

  There're 2 methods of tracking timestamps:
  1) Timestamp is applied to the first syncpoint in the chunk. But when we
     have a compressed stream, syncpoint (beginning of a frame) may be in the
     middle of the chunk. So if we want to form output chunks with one frame
     in a chunk, we have to move timestamps like following:

  \verbatim
    t1           t2           t3
    v            v            v
    +------------+------------+------------+
    |   chunk1   |   chunk2   |   chunk3   |
    +------------+------------+------------+

    +--------+--------+--------+--------+--
    | frame1 | frame2 | frame3 | frame4 |
    +--------+--------+--------+--------+--
    ^        ^        ^        ^
    t1    no time     t2       t3
  \endverbatim


  2) When a filter buffers some data, it may need to update timestamps:

  \verbatim
   t1           t2           t3
   v            v            v
   +------------+------------+------------+
   |   chunk1   |   chunk2   |   chunk3   |
   +------------+------------+------------+

   +--------+--------+--------+--------+--
   | chunk1 | chunk2 | chunk3 | chunk4 |
   +--------+--------+--------+--------+--
   ^        ^   ^    ^       ^^
   t1    no time|    t2+A    |t3+B
                |    |       ||
                <---->       <>
          buffered data A  buffered data B
  \endverbatim

  For linear format it is possible to track buffered data automatically, just
  by counting input and output samples. (But this method cannot be applied to
  sample rate conversion).

  <b>Example 1: Linear format filter</b>

  In this example filter processes data of the linear format in blocks. It have
  to remember incoming time stamps and apply correct shit at output.

  \code
  bool SomeFilter::process(Chunk &in, Chunk &out)
  {
    // SyncHelper remembers the time stamp and counts number of samples that
    // arrive the filter.

    sync_helper.receive_linear(in);

    // Fill an internal buffer and exit when it have no enogh data

    if (!fill_block(in))
      return false;

    // Process data at the buffer and fill the output chunk

    process_block();
    out.set_linear(buf, buf_size);

    // Apply correct timestamp to the output chunk. SyncHelper counts the
    // number of samples that leaves the filter and calculates the correct time
    // shift when nessesary.
    //
    // Note, that it must be done after filling the output chunk.

    sync_helper.send_linear(out, spk.sample_rate);
    return true;
  }

  void SomeFilter::reset()
  {
    ....
    // We must always reset the helper with the filter.
    sync_helper.reset();
  }
  \endcode

  <b>Example 2: Frames</b>

  In this example filter receives raw stream and splits it into frames.
  Timestamp is always applied to the beginning of the frame.

  \code
  bool FrameFilter::process(Chunk &in, Chunk &out)
  {
    // SyncHelper remembers the time stamp. You must provide the helper with
    // the amount of currently buffered data.

    sync_helper.receive_sync(in, buf_pos);

    // Fill an internal buffer and exit when it have no enogh data

    if (!fill_block(in))
      return false;

    // Process data at the buffer

    data_gone = process_block();

    // Now buffer points to the beginning of the frame, but to correctly track
    // buffer position helper must know how much data gone from the buffer.
    // So *before* applying the timestamp we must notify the helper about
    // changes at the buffer.

    out.set_rawdata(buf, frame_size);
    sync_helper.drop(data_gone);
    sync_helper.send_frame(out);
    return true;
  }
  \endcode

  \fn void SyncHelper::receive_linear(Chunk &chunk)
    \param chunk Input chunk

    Receive sync info and count nuimber of samples that arrive the filter.

  \fn void SyncHelper::send_linear(Chunk &chunk, int sample_rate)
    \param chunk Output chunk
    \param sample_rate Sample rate

    Timestamp the output chunk if nessesary and count number of samples that
    leaves the filter. Output chunk must be filled before.

  \fn void SyncHelper::receive_sync(const Chunk &chunk, pos_t pos)
    \param chunk Input chunk
    \param pos Buffer position

    Receive sync info from the input chunk. Syncpoint will be applied to
    the buffer position pos.

  \fn void SyncHelper::send_frame_sync(Chunk &chunk)
    Timestamp the output chunk if nessesary.
    Chunk will be stamped with the first timestamp in the queue.
    This is applicable when chunks are frames of compressed data and we must
    apply the timestamp to the first frame after the timestamp received.

  \fn void SyncHelper::send_sync(Chunk &chunk, double size_to_time)
    Timestamp the output chunk if nessesary.
    Chunk will be stamped with the first timestamp in the queue, shifted by
    the amount of data between the timestamp received and the chunk's start.
    Applicable for linear and PCM data.

  \fn void SyncHelper::drop(int size)
    Drop buffered data. Required to track suncpoint positions.

  \fn void SyncHelper::reset()
    Drop all timing information.

******************************************************************************/

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
SyncHelper::receive_sync(const Chunk &chunk, pos_t buf_pos)
{
  if (chunk.sync)
  {
    if (sync[0] && pos[0] != buf_pos)
    {
      assert(pos[0] < buf_pos);
      sync[1] = true;
      time[1] = chunk.time;
      pos[1]  = buf_pos;
    }
    else
    {
      sync[0] = true;
      time[0] = chunk.time;
      pos[0]  = buf_pos;
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
