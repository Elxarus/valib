/*
  Abstract filter interface
  NullFilter implementation
*/

#ifndef VALIB_FILTER_H
#define VALIB_FILTER_H

#include "data.h"

class Sink;
class Source;
class Filter;
class NullFilter;


///////////////////////////////////////////////////////////////////////////////
// Sink class
//
// Abstract audio sink.
// 
// query_input()
//   Check if we can change format now. Returns true if we can and false if we
//   can't. Sink may have some internal state that may prevent format change, 
//   so this function should return true only if it can switch format right 
//   now. It is assumed that query result may be changed only as a result of 
//   some state-change calls and does not change all other time. So we can 
//   query sink from one thread and then call set_input() from other and be 
//   sure that set_input() call succeeds. set_input() and process() calls 
//   should not change query result (?). Should work as fast as possible, 
//   because it may be used to try numerous formats to find acceptable 
//   conversion. Also, this function may be called asynchronously from other 
//   thread, so it should be thread-safe.
//
// set_input()
//   Change format. Returns true on success and false otherwise. It is 
//   expected that it returns true if query_input() returned true just
//   before set_input() call (no additional verification required).
//   It may require some time to change format (initialize output, device, 
//   allocate buffers, etc). Sound output is not required to be continious 
//   and clickless in case of format changes, but it is highly recommended 
//   to produce minimum artifacts. This function should be called only from 
//   worker thread. 
//
// process()
//   Receive data chunk. Returns true on success and false otherwise. Data 
//   chunk may have different format than specified by set_input() call so 
//   this function is required to handle format changes correctly (in this
//   case it is the same assumptions as for set_input() call). Empty chunks
//   shuold be inored without errors. Empty chunks may contain invalid format
//   (sink should not change format for empty chunks) and invalid timestamp.
//   For audio renderers this call may block working thread during audio 
//   playback. After receiving end-of-stream chunk it may block until last 
//   audio sample played.
//

class Sink
{
public:
  virtual bool query_input(Speakers spk) const = 0;
  virtual bool set_input(Speakers spk) = 0;
  virtual bool process(const Chunk *chunk) = 0;
};


///////////////////////////////////////////////////////////////////////////////
// Source class
//
// Abstract audio source.
//
// get_output()
//   Query current output format. Primary purpose of this call is to setup
//   downstream audio sink before processing (because state change may be 
//   time-consuming operation). But it is no general rules when source may 
//   change output format (depends completely on implementation). May be 
//   called asynchronously.
//
// is_empty()
//   Check if we can get some data from this source. Depending on source type
//   it may require some explicit actions to be filled (like filters) or it 
//   may be filled asyncronously (like audio capture). May be called 
//   asynchronously.
//
// get_chunk()
//   Receive data from the source. Should be called only from working thread.
//   If empty chunk is returned it does not mean that source is empty. Always
//   check it with is_empty() call. This call may block working thread.
//

class Source
{
public:
  virtual Speakers get_output() const = 0;
  virtual bool is_empty() const = 0;
  virtual bool get_chunk(Chunk *chunk) = 0;
};


///////////////////////////////////////////////////////////////////////////////
// Filter class
//
// Abstract data processing class. It is sink and source at the same time so 
// it should follow all rules for Sink and Source classes.
//
// reset()
//   Reset filter state to empty and drop all internal buffers. But after 
//   reset() call filter should be able to report its state as it was before 
//   for most of its parameters.
//
// set_input()
//   Acts like reset().
//
// process()
//   May be used for data processing. Do main processing here if filter 
//   produces exactly one output chunk for one input chunk.
//
// get_chunk()
//   May be used for data processing. Do main processing here if filter 
//   may produce many output chunks for one input chunk.
//
// Filter have 3 states:
//
// * empty
//   Filter have no data to output and waits for more data to input. In this 
//   state filter may have some data locked at internal buffes.
//
//   is_empty() returns true.
//   process() with non-eof chunk may switch filter to full state.
//   process() with eof-chunk must switch filter to flushing state.
//   get_chunk() returns empty chunk with no state change.
//
// * full
//   Filter have some data to output. In this state it cannot receive data.
//
//   is_empty() returns false.
//   get_chunk() may return empty and non-empty chunks.
//   get_chunk() may switch filter to empty state.
//   process() call result is undefined (maybe it should fail?)
//
// * flushing
//   In this state filter releases data blocked in its internal buffers.
//
//   is_empty() returns false.
//   get_chunk() may return empty and non-empty chunks without state change.
//   get_chunk() may return eof-chunk and switch to empty state.
//   process() call result is undefined.
//           
//
// Data processing model (working thread):
//
//                 |
//   +------------>|<----------------------------+
//   |             v                  ^          |
//   |      +-------------+           |          |
//   |      |   reset()   |           |          |
//   |      | set_input() |           |    ...any state
//   |      +-------------+           |
//   |             |                  |
//   |             |<-------------+   |
//   |        ----------          |   |
//   |      (  get data  )        |   |
//   |      ( to process )        |   |
//   |        ----------          |   |
//   |             |              |   |
//   |             v              |   |
//   | error +-----------+        |   |
//   +-------| process() |        |   |
//           +-----------+        |   |
//                 |              |   |
//   +------------>|              |   |
//   |             v              |   |
//   |       /-----------\  true  |   |
//   |      < is_empty()? >-------+   |
//   |       \-----------/            |                    
//   |       false |                  |
//   |             |                  |
//   |             v                  |
//   |      +-------------+  error    |
//   |      | get_chunk() |-----------+ 
//   |      +-------------+  
//   |             |         
//   |             v         
//   |      ---------------  
//   |    (  do something   )
//   |    ( with chunk data )
//   |      ---------------  
//   |             |         
//   +-------------+
//
// Other threads may call:
//   query_input()
//   get_output()
//   is_empty()
        
class Filter: public Sink, public Source
{
public:
  virtual void reset() = 0;

  virtual bool query_input(Speakers spk) const = 0;
  virtual bool set_input(Speakers spk) = 0;
  virtual bool process(const Chunk *chunk) = 0;

  virtual Speakers get_output() const = 0;
  virtual bool is_empty() const = 0;
  virtual bool get_chunk(Chunk *chunk) = 0;
};




///////////////////////////////////////////////////////////////////////////////
// NullFilter class
//
// Simple filter implementation that does nothing (just passthrough all data)
// and fulfills all requirements.
//
// NullFilter remembers input data and makes output chunk equal to input data.
// Following funcitons may be used in descendant classes:
//
// receive_chunk()      - remember input chunk data
// send_empty_chunk()   - fill empty output chunk
// send_chunk_buffer()  - fill output chunk
// send_chunk_inplace() - fill output chunk and drop data from input buffer
// drop()               - drop data from input buffer and track time changes
//
// Time tracking:
// ==============
// todo...

class NullFilter : public Filter
{
protected:
  Speakers  spk;

  bool      sync;
  time_t    time;
  bool      flushing;

  uint8_t  *rawdata;
  samples_t samples;
  size_t    size;

  inline bool receive_chunk(const Chunk *_chunk)
  {
    // format change
    if (spk != _chunk->get_spk() && !set_input(_chunk->get_spk())) 
      return false;

    // remember input chunk info
    if (_chunk->is_sync()) // ignore non-sync chunks
    {
      sync     = true;
      time     = _chunk->get_time();
    }
    flushing = _chunk->is_eos();
    rawdata  = _chunk->get_rawdata();
    samples  = _chunk->get_samples();
    size     = _chunk->get_size();

    return true;
  }

  inline void send_chunk_inplace(Chunk *_chunk, size_t _size)
  {
    // fill output chunk & drop data

    // speakers
    _chunk->set_spk(spk);

    // sync
    _chunk->set_sync(sync, time);
    sync = false;

    // data send & drop
    if (_size > size)
      _size = size;

    if (spk.format == FORMAT_LINEAR)
    {
      _chunk->set_samples(samples, _size);
      samples += _size;
    }
    else
    {
      _chunk->set_rawdata(rawdata, _size);
      rawdata += _size;
    }

    size -= _size;

    // end-of-stream
    _chunk->set_eos(flushing && !size);
    flushing = flushing && size;
  }

  inline void drop(size_t _size)
  {
    if (_size > size)
      _size = size;

    if (spk.format == FORMAT_LINEAR)
      samples += _size;
    else
      rawdata += _size;

    size -= _size;
  }

  inline void drop_rawdata(size_t _size)
  {
    if (_size > size)
      _size = size;

    rawdata += _size;
    size -= _size;
  }

  inline void drop_samples(size_t _size)
  {
    if (_size > size)
      _size = size;

    samples += _size;
    size    -= _size;
  }

public:
  NullFilter() 
  {
    spk  = unk_spk;
    size = 0;
    time = 0;
    sync = false;
    flushing = false;
  }

  virtual void reset()
  {
    size = 0;
    time = 0;
    sync = false;
    flushing = false;
  }                            

  virtual bool query_input(Speakers _spk) const
  { 
    // general audio processing filter works only with linear format
    return _spk.format == FORMAT_LINEAR;
  }

  virtual bool set_input(Speakers _spk)
  {
    if (spk != _spk)
    {
      if (!query_input(_spk)) // may be overwritten
        return false;
      spk = _spk;
      reset(); // may be overwritten
    }
    return true;
  }

  virtual bool process(const Chunk *_chunk)
  {
    return receive_chunk(_chunk);
  }

  virtual Speakers get_output() const
  {
    return spk;
  }

  virtual bool is_empty() const
  {
    // must report false in flushing state
    return size && !flushing;
  };

  virtual bool get_chunk(Chunk *_chunk)
  {
    send_chunk_inplace(_chunk, size);
    return true;
  };
};

#endif
