/**************************************************************************//**
  \file sync.h
  \brief SyncHelper: Synchronization helper class
******************************************************************************/

#ifndef VALIB_SYNC_H
#define VALIB_SYNC_H

#include <deque>
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
     per chunk, we have to move timestamps like following:

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

  To track timestamp position SyncHelper has to know when data comes into and
  goes out from the buffer. Timestamp is sent when it goes out from the buffer
  window.

  Let's constider some filter that has some data at the buffer and a timestamp
  at the middle of the data received:

  \verbatim
                   Buffer start    Buffer end
                   V               V
  +--------------------------------------------------------------
  |          |     *****|**********|          |          | Input stream
  +--------------------------------------------------------------
                        ^
                        t1
  \endverbatim

  This filter receives 2 chunks with one timestamp:

  \verbatim
                   Buffer start                          Buffer end
                   V                                     V
  +--------------------------------------------------------------
  |          |     *****|**********|**********|**********| Input stream
  +--------------------------------------------------------------
                        ^                     ^
                        t1                    t2
  \endverbatim

  Then it makes one output chunk. Note, that this chunk is not stamped because
  t1 timestamp is received *after* the data at the buffer start:

  \verbatim
                              Buffer start               Buffer end
                              V                          V
  +--------------------------------------------------------------
  |          |          |     *****|**********|**********| Input stream
  +--------------------------------------------------------------
                        ^                     ^
                        t1                    t2
                   +----------+
                   |          | Output chunk without timestamp
                   +----------+
  \endverbatim

  Now the timestamp lays before the buffer start, and next output chunk will
  be stamped:

  \verbatim
                                         Buffer start    Buffer end
                                         V               V
  +--------------------------------------------------------------
  |          |          |          |     *****|**********| Input stream
  +--------------------------------------------------------------
                        ^                     ^
                        t1                    t2
                              +----------+
                        < dt >|          | Output chunk
                              +----------+
                              ^
                              t1 + dt
  \endverbatim

  To find dt we should know how to convert size to time. Conversion
  coeffitient is used for this. For linear format this coeffitient equals to
  1/sample_rate. For pcm format it is 1/(word_size*nchannels*sample_rate).

  In frame output mode dt equals to zero (timestamp does not shift) and the
  coeffitient is also zero.

  \b Example

  In this example filter buffers and processes data in blocks. It have
  to remember incoming timestamps and produce correct time at output.

  \code
  bool SomeFilter::process(Chunk &in, Chunk &out)
  {
    // SyncHelper remembers the time stamp
    sync_helper.receive_linear(in);

    // Fill an internal buffer and exit when it have no enogh data
    // Note, that we may buffer only a part of the input.

    size_t gone = fill_block(in);
    sync_helper.put(gone);

    if (!buffer_is_full())
      return false;

    // Process data at the buffer and fill the output chunk

    process_block();
    out.set_linear(buf, buf_size);

    // Apply correct timestamp to the output chunk. SyncHelper counts the
    // number of samples that leaves the filter and calculates the correct time
    // shift when nessesary.
    //
    // Note, that it must be called after filling the output chunk, so helper
    // can count the number of output samples sent.

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

  \fn void SyncHelper::receive_sync(Chunk &chunk)
    \param chunk Input chunk

    Receive sync info from the input chunk.

    This function removes timestamp from the chunk to avoid occasional usage
    of this timestamp again.

  \fn void SyncHelper::send_frame_sync(Chunk &chunk)
    \param chunk Output chunk

    Timestamp the output chunk if nessesary.

    Chunk will be stamped with the first timestamp in the queue.
    This is applicable when chunks are frames of compressed data and we must
    apply the timestamp to the first frame after the timestamp received.

    This function if equivalent to the following:
    \code
    sync_helper.send_sync(chunk, 0);
    \endcode

  \fn void SyncHelper::send_sync(Chunk &chunk, double size_to_time)
    \param chunk Output chunk
    \param size_to_time Buffer size to time conversion coeffitient

    Timestamp the output chunk if nessesary.

    Chunk will be stamped with the first timestamp in the queue, shifted by
    the amount of data between the timestamp received and the chunk's start.
    Applicable for linear and PCM data.

  \fn void SyncHelper::send_sync_linear(Chunk &chunk, int sample_rate)
    \param chunk Output chunk
    \param sample_rate Sampling rate

    Timestamp the output chunk if nessesary.

    Chunk will be stamped with the first timestamp in the queue, shifted by
    the number of samples between the timestamp received and the chunk's start.

    This function assumes that output chunk is dropped from the buffer, so
    drop() is called automatically. Also, this means that this function must
    be called *after* chunk was filled.

    This function if equivalent to the following:
    \code
    sync_helper.send_sync(chunk, 1.0/spk.sample_rate);
    sync_helper.drop(chunk.size);
    \endcode

  \fn void SyncHelper::put(int size)
    \param size Amount of data arrives the buffer

    Advance the end-of-the-buffer pointer.

  \fn void SyncHelper::drop(int size)
    \param size Amount of data leaves the buffer

    Move start-of-the-buffer pointer.

  \fn void SyncHelper::reset()
    Drop all timing information.

******************************************************************************/

class SyncHelper
{
public:
  SyncHelper()
  { reset(); }

  inline void receive_sync(Chunk &chunk);
  inline void send_frame_sync(Chunk &chunk);
  inline void send_sync(Chunk &chunk, double size_to_time);
  inline void send_sync_linear(Chunk &chunk, int sample_rate);

  inline void put(size_t size);
  inline void drop(size_t size);
  inline void reset();

protected:
  struct timestamp {
    vtime_t time;
    pos_t pos;

    timestamp(vtime_t time_, pos_t pos_):
    time(time_), pos(pos_) {}
  };

  std::deque<timestamp> t; // timestamp queque
  pos_t buf_size; // current buffer fullness
};

///////////////////////////////////////////////////////////////////////////////

inline void
SyncHelper::receive_sync(Chunk &chunk)
{
  if (chunk.sync)
  {
    t.push_back(timestamp(chunk.time, buf_size));
    chunk.set_sync(false, 0);
  }
}

inline void
SyncHelper::send_frame_sync(Chunk &chunk)
{
  send_sync(chunk, 0);
}

inline void
SyncHelper::send_sync(Chunk &chunk, double size_to_time)
{
  if (t.size() > 0 && t[0].pos <= 0)
  {
    chunk.sync = true;
    chunk.time = t[0].time - t[0].pos * size_to_time;
    t.pop_front();
  }
}

inline void
SyncHelper::send_sync_linear(Chunk &chunk, int sample_rate)
{
  send_sync(chunk, 1.0 / sample_rate);
  drop(chunk.size);
}

inline void
SyncHelper::put(size_t size)
{
  buf_size += size;
}

inline void
SyncHelper::drop(size_t size)
{
  for (size_t i = 0; i < t.size(); i++)
    t[i].pos -= size;
  while (t.size() > 1 && t[1].pos <= 0)
    t.pop_front();
  buf_size -= size;
}

inline void
SyncHelper::reset()
{
  t.clear();
  buf_size = 0;
}

#endif
