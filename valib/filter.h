/*
  Abstract filter interface
  NullFilter implementation
*/

#ifndef VALIB_FILTER_H
#define VALIB_FILTER_H

#include "data.h"

class Chunk;
class Sink;
class Source;
class Filter;

class NullSink;
class NullFilter;

// safe call to filtering functions
#define FILTER_SAFE(call) if (!call) return false;

///////////////////////////////////////////////////////////////////////////////
// Chunk class.
// A part of audio data.
//
// There are several types of chunks:
// 1) Data chunk/empty chunk. A chunk that has data/has no data.
// 2) Syncronization chunk (sync-chunk). A chunk that has syncronization info.
// 3) End-of-steam chunk (eos-chunk). A chunk that ends data transmission.
//
// Empty chunk (chunk that contain no data) may be used to inform downsteram
// about different events: format change, syncronization or end of steram.
//
// Speakers
// ========
// Chunk always has data format. 
//
// get_spk() - get data format
// set_spk() - set data format
//
// Syncronization
// ==============
// Chunk may contain timestamp that indicates position in the stream. In this
// case it is called syncronization chunk. Empty syncronization chunk is used 
// just to indicate position. (see also Sync class at filter.h).
//
// is_sync()  - is this chunk contains timestamp
// get_time() - returns timestamp
// set_sync() - set syncronization parameters
// 
// End-of-stream
// =============
// End-of-stream chunk (eos-chunk) is method to correctly finish the stream. 
// This chunk may contain data and stream is assumed to end after last 
// byte/sample of this chunk. Empty end-of-stream chunk is used just to end 
// the stream correctly. After receiving eos-chunk filter should flush all 
// internal data buffers and mark last chunk sent as eos-chunk.
//
// is_eos()  - chunk is end-of-stream chunk
// set_eos() - set chunk as end-of-stream and empty
//
// Empty chunk
// ===========
// Empty chunk may be used to deliver special events to the processing chain
// (format change, syncronization, end-of-stream). Both data buffers may be
// invalid in this case.
//
// is_empty()  - chunk is empty
// set_empty() - set chunk as empty chunk
//
// Buffers
// =======
// Chunk has 2 types of buffer pointers: raw buffer and linear 
// (splitted-channels) buffer. Most of internal data processing is done on
// linear format, but data from external sources may be in any format. Some 
// filters works with both raw and linear data so for interface universality 
// it is only one type of chunk with both raw and linear buffers.
//
// get_size()    - number of samples in case of linear data farmat and size of
//                 raw buffer in bytes otherwise.       
// get_rawdata() - raw buffer pointer (only for raw data)
// get_samples() - pointers to linear channel buffers (only for linear firmat)
// operator []   - pointer to channel buffer
// set_rawdata() - set raw buffer
// set_samples() - set linear buffers
// drop()        - drop data from raw or linear buffer (bytes or samples).
//


class Chunk
{
protected:
  Speakers  spk;

  bool      sync;
  vtime_t   time;
            
  bool      eos;

  uint8_t  *rawdata;
  samples_t samples;
  size_t    size;

public:
  // Chunk()
  //
  // Speakers
  // inline Speakers get_spk() const
  // inline void set_spk(Speakers _spk)

  // Syncronization
  // inline bool is_sync() const
  // inline vtime_t get_time() const
  // inline void set_sync(bool _sync, vtime_t _time = 0)

  // End-of-stream
  // inline bool is_eos() const;
  // inline void set_eos(bool eos = true);

  // Empty chunk
  // inline bool is_empty() const
  // inline void set_empty()

  // Buffers
  // inline size_t    get_size() const
  // inline uint8_t  *get_rawdata() const
  // inline samples_t get_samples() const

  // inline operator uint8_t *() const
  // inline operator samples_t() const
  // inline sample_t *operator[](int _ch) const

  // inline void set_buf(uint8_t *_buf, size_t _size);
  // inline void set_samples(samples_t _samples, size_t _size) 
  // inline void drop(size_t _size)
  


  Chunk(): spk(unk_spk), sync(false), time(0), eos(false), size(0), rawdata(0)
  {}

  Chunk(Speakers _spk, 
                  samples_t _samples, size_t _size,
                  bool _sync = false, vtime_t _time = 0,
                  bool _eos  = false)
  {
    set(_spk, _samples, _size, _sync, _time, _eos);
  }

  Chunk(Speakers _spk, 
                  uint8_t *_rawdata, size_t _size,
                  bool _sync = false, vtime_t _time = 0,
                  bool _eos  = false)
  {
    set(_spk, _rawdata, _size, _sync, _time, _eos);
  }

  /////////////////////////////////////////////////////////
  // Speakers

  inline void set(Speakers _spk, 
                  samples_t _samples, size_t _size,
                  bool _sync = false, vtime_t _time = 0,
                  bool _eos  = false)
  {
    spk = _spk;
    samples = _samples;
    size = _size;
    sync = _sync;
    time = _time;
    eos = _eos;
  }

  inline void set(Speakers _spk, 
                  uint8_t *_rawdata, size_t _size,
                  bool _sync = false, vtime_t _time = 0,
                  bool _eos  = false)
  {
    spk = _spk;
    rawdata = _rawdata;
    size = _size;
    sync = _sync;
    time = _time;
    eos = _eos;
  }

  /////////////////////////////////////////////////////////
  // Speakers

  inline Speakers get_spk() const
  {
    return spk;
  }

  inline void set_spk(Speakers _spk)
  { 
    spk = _spk; 
  };

  /////////////////////////////////////////////////////////
  // Syncronization

  inline bool is_sync() const
  {
    return sync;
  }

  inline vtime_t get_time() const
  {
    return time;
  }

  inline void set_sync(bool _sync, vtime_t _time = 0)
  { 
    sync = _sync;
    time = _time;
  }

  /////////////////////////////////////////////////////////
  // End-of-stream

  inline bool is_eos() const
  {
    return eos;
  }

  inline void set_eos(bool _eos)
  {
    eos = _eos;
  }

  /////////////////////////////////////////////////////////
  // Empty chunk

  inline bool is_empty() const
  { 
    return size == 0; 
  }

  inline void set_empty()         
  { 
    size = 0; 
  }

  /////////////////////////////////////////////////////////
  // Buffers

  inline size_t    get_size() const              { return size;         }
  inline uint8_t  *get_rawdata() const           { return rawdata;      }
  inline samples_t get_samples() const           { return samples;      }

  inline operator uint8_t *() const              { return rawdata;      }
  inline operator samples_t() const              { return samples;      }
  inline sample_t *operator[](int _ch) const     { return samples[_ch]; }

  inline void set_rawdata(uint8_t *_rawdata, size_t _size)
  { 
    rawdata = _rawdata; 
    size = _size;
  }

  inline void set_samples(const samples_t &_samples, size_t _size) 
  {
    samples = _samples; 
    size = _size; 
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
    sync = false;
  };
};

///////////////////////////////////////////////////////////////////////////////
// Sink class
//
// Abstract audio sink.
// 
// query_input() [thread-safe, fast]
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
// set_input() [working thread, blocking]
//   Change format. Returns true on success and false otherwise. It is 
//   expected that it returns true if query_input() returned true just
//   before set_input() call (no additional verification required).
//   It may require some time to change format (initialize output, device, 
//   allocate buffers, etc). Sound output is not required to be continious 
//   and clickless in case of format changes, but it is highly recommended 
//   to produce minimum artifacts. This function should be called only from 
//   worker thread. 
//
// get_input() [thread-safe, fast]
//   Just report current input format. If function reports FORMAT_UNKNOWN it
//   means that sink is not initialized. In all other cases it is supposed 
//   that query_input(get_input()) returns true.
//
// process() [working thread, critical path]
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
  virtual Speakers get_input() const = 0;
  virtual bool process(const Chunk *chunk) = 0;
};

class NullSink : public Sink
{
protected:
  Speakers spk;

public:
  NullSink() {};

  virtual bool query_input(Speakers _spk) const { return true; }
  virtual bool set_input(Speakers _spk)         { spk = _spk; return true; }
  virtual Speakers get_input() const            { return spk;  }
  virtual bool process(const Chunk *_chunk)     { spk = _chunk->get_spk(); return true; }
};


///////////////////////////////////////////////////////////////////////////////
// Source class
//
// Abstract audio source.
//
// get_output() [thread-safe, fast]
//   Query current output format. Primary purpose of this call is to setup
//   downstream audio sink before processing (because state change may be 
//   time-consuming operation). But it is no general rules when source may 
//   change output format (depends completely on implementation). May be 
//   called asynchronously.
//
// is_empty() [thread-safe, critical path]
//   Check if we can get some data from this source. Depending on source type
//   it may require some explicit actions to be filled (like filters) or it 
//   may be filled asyncronously (like audio capture). May be called 
//   asynchronously.
//
// get_chunk() [working thread, critical path]
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
// reset() [working thread]
//   Reset filter state to empty, drop all internal buffers and all external 
//   references. But after reset() call filter should be able to report its 
//   state as it was before for most of its parameters.
//
// set_input() [working thread]
//   If current and new configuration differs then it acts as reset() call,
//   in other case nothing happens.
//
// process() [working thread, critical path]
//   May be used for data processing. Do main processing here if filter 
//   produces exactly one output chunk for one input chunk.
//
// get_chunk() [working thread, critical path]
//   May be used for data processing. Do main processing here if filter 
//   may produce many output chunks for one input chunk.
//
// Filter have 3 states:
//
// * empty
//   Filter have no data to output and waits for more data to input. In this 
//   state filter may have some data locked at internal buffes. But it is 
//   guaranteed that filter has no references to any external data received
//   with process() call.
//
//   is_empty() returns true.
//   process() with non-eof chunk may switch filter to full state.
//   process() with eof-chunk must switch filter to flushing state.
//   get_chunk() returns empty chunk with no state change.
//
// * full
//   Filter have some data to output. In this state it cannot receive data
//   and may have references to the data received with process() call, so 
//   external procedures should not alter this data.
//
//   is_empty() returns false.
//   get_chunk() may return empty and non-empty chunks.
//   get_chunk() may switch filter to empty state.
//   process() call result is undefined (maybe it should fail?)
//
// * flushing
//   In this state filter releases data blocked in its internal buffers.
//   After this call all external references of the filter are dropped.
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
  virtual Speakers get_input() const = 0;
  virtual bool process(const Chunk *chunk) = 0;

  virtual Speakers get_output() const = 0;
  virtual bool is_empty() const = 0;
  virtual bool get_chunk(Chunk *chunk) = 0;

  inline bool process_to(const Chunk *chunk, Sink *sink);
  inline bool get_from(Chunk *chunk, Source *source);
  inline bool transform(Source *source, Sink *sink);
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
  vtime_t   time;
  bool      flushing;

  uint8_t  *rawdata;
  samples_t samples;
  size_t    size;

  inline bool receive_chunk(const Chunk *_chunk)
  {
    // format change
    if (spk != _chunk->get_spk())
      FILTER_SAFE(set_input(_chunk->get_spk()));

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

    if (_size > size)
      _size = size;

    if (spk.format == FORMAT_LINEAR)
    {
      _chunk->set
      (
        spk, 
        samples, _size,
        sync, time,
        flushing && (size == _size)
      );
      samples += _size;
    }
    else
    {
      _chunk->set
      (
        spk, 
        rawdata, _size,
        sync, time,
        flushing && (size == _size)
      );
      rawdata += _size;
    }

    size -= _size;
    sync = false;
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

  virtual Speakers get_input() const
  {
    return spk;
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
    return !size && !flushing;
  };

  virtual bool get_chunk(Chunk *_chunk)
  {
    send_chunk_inplace(_chunk, size);
    return true;
  };
};



///////////////////////////////////////////////////////////////////////////////
// Filter inlines
///////////////////////////////////////////////////////////////////////////////

inline bool 
Filter::process_to(const Chunk *_chunk, Sink *_sink)
{
  Chunk chunk;

  while (!is_empty())
  {
    FILTER_SAFE(get_chunk(&chunk));
    FILTER_SAFE(_sink->process(&chunk));
  }

  FILTER_SAFE(process(_chunk));

  while (!is_empty())
  {
    FILTER_SAFE(get_chunk(&chunk));
    FILTER_SAFE(_sink->process(&chunk));
  }

  return true;
}

inline bool 
Filter::get_from(Chunk *_chunk, Source *_source)
{
  Chunk chunk;

  while (is_empty())
  {
    FILTER_SAFE(_source->get_chunk(&chunk));
    FILTER_SAFE(process(&chunk));
  }

  return get_chunk(_chunk);
}

inline bool 
Filter::transform(Source *_source, Sink *_sink)
{
  Chunk chunk;

  while (is_empty())
  {
    FILTER_SAFE(_source->get_chunk(&chunk));
    FILTER_SAFE(process(&chunk));
  }

  FILTER_SAFE(get_chunk(&chunk));
  return _sink->process(&chunk);
}



#endif
