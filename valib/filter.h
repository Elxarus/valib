/*
  Abstract filter interface
  NullFilter implementation

  Terms
  =====
  * Aduio stream - continious block of audio data. 
    (same format, may have associated timeline, 
     stream sources: audio file, multimedia file, live source as sound card, 
     internet, etc)
  * Chunk - a structure used to carry audio data and stream events.
  * Stream events
    (format change, end-of-stream, syncronization)

  * Source
  * Sink
  * Filter

  * Timestamps
  * Syncpoints


  Data flow
  =========
  ......

  Filter data processing
  ======================

  * 2 sided data processing model (with block diagram)
  * in-place processing
  * buffered processing
  * external references
  * Buffering and flushing

  Flow control
  ============
  ........


  Rules
  =====

  Source
  ------

  [s1] When source is full it must report format exactly as it will appear at
    next output chunk. In other words get_output() may change its value only
    in following cases:
    
    1) after get_chunk() call
    2) when is_empty == true (should be avioded)
    3) by call to descendants' class functions (only at working thread).

    if (!source.is_empty())
    {
      spk1 = source.get_output();
      chunk1 = source.get_chunk();
      assert(chunk1.spk == spk1);
      ...
      if (!source.is_empty())
      {
        spk2 = source.get_output();
        chunk2 = source.get_chunk();
        assert(chunk2.spk == spk2);
        // note that spk2 may differ from spk1
      }
    }

  Sink
  ----

  [k1] Input format must be equal to spk_unknown when sink requires 
    initialization (after creation, errors, etc).

  [k2] Input format switch may occur only in following cases:
    1) set_input() call.
    2) process() call if chunk format differs from current input format.
    3) by call to descendants' class functions (only at working thread).

  [k3] Format switch call should succeed after successful query_input() call.

    if (sink.query_input(spk))
      assert(sink.set_input(spk));

    if (sink.query_input(chunk.spk))
      assert(sink.process(chunk));

  [k4] get_output() must report new format immediately after format switch.

    if (sink.query_input(spk))
    {
      assert(sink.set_input(spk));
      assert(spk == sink.get_input());
    }

    if (sink.query_input(chunk.spk))
    {
      assert(sink.process(chunk));
      assert(chunk.spk == sink.get_input());
    }

  Filter
  ------

  [f1] Filter must report spk_unknown for input formats when filter requires
    initialization (after creation, errors, etc).

  [f2] If output format depends on input data get_output() must report 
    spk_unknown after following:
    1) reset() call
    2) input format switch (see [k2] rule)
    3) call to descendants' class functions (only at working thread) that
       may affect output format.

    Filter must change its output format according to [s1] rule.

    filter.reset()
    assert(filter.get_output() == spk_unknown);
    ...
    filter.process(chunk);
    ...
    if (!filter.is_empty())
      assert(correct_format(filter.get_output())

  [f3] If output format doesn't depend on input data it may change only in
    following cases:
    1) input format switch (see [k2] rule)
    2) by call to descendants' class functions (only at working thread)

    Filter cannot switch output format during processing in this case.

  [f4] It is possible that for some input formats output format may depend on
    input data and for some it doesn't. In this case for dependent formats
    filter must follow [f2] rule and [f3] rule for independent.

  [f5] If output format changes according to [f3.2] to format that is
    incompatible with current input format input format must change to 
    spk_unknown indicating that input should be reinitialized according to
    [f1] rule.

*/

#ifndef VALIB_FILTER_H
#define VALIB_FILTER_H

#include "spk.h"

class Chunk;
class Sink;
class Source;
class Filter;

class NullSink;
class NullFilter;

// safe call to filtering functions
#define FILTER_SAFE(call) if (!call) return false;

///////////////////////////////////////////////////////////////////////////////
// Chunk
//
// This structure is used to transfer audio data and stream events from audio
// source to auido sink (see Source and Sink classes below). 
//
// Structure consists of:
// * Data format
// * Audio data
// * Syncronization data
// * End of stream flag
//
// Following stream events possible:
// * Format change
// * Syncronization
// * End of stream
//
// Data format
// ===========
// Chunk always carries data format in 'spk' field.
//
// Audio data
// ==========
// Chunk may carry audio data. There two kinds of data possible: raw data and
// channel samples (linear format). Raw data is a simple continious block of
// binary data. It may be PCM data, encoded data (AC3, DTS) or something else.
// Linear format data is a set of sample buffers for each channel. PCM data in
// raw form is interleaved and may have different sample format. So it's hard
// to work with it. Linear format has linear buffers for each channel and
// fixed sample format (sample_t). Most of internal data processing is done on
// linear format but input and output must be raw. So considering importance
// of both kinds of data the chunk structure may carry both. So 'rawdata'
// field is a pointer to raw buffer and 'samples' field is a set of pointers
// to sample buffers for each channel.
//
// We can find data format with 'spk' field. If it indicates FORMAT_LINEAR
// then 'samples' field must be filled and 'rawdata' field value is undefined.
// Any other format means that 'samples' field is undefined and 'rawdata'
// field has correct pointer. We must not use undefined field.
//
// Any data format has buffer size. In case of raw data it means size of data
// buffer pointed by 'rawdata' pointer in bytes. In case of linear format it
// means number of samples in each channel buffer. So size of each buffer in 
// bytes is: channel_buffer_size = sizeof(sample_t) * chunk.size .
//
// Also chunk may not contain any data (empty chunk). Such chunks may be used
// to inform downstream about different events without sending data. Empty 
// chunk means that both pointers are considered to be invalid and must not be
// used. Empty chunk has 'size' field set to zero (size == 0). There is a
// special function is_empty() to clarify this statement.
//
// Syncronization data
// ===================
// Chunk may contain time stamp that indicates position in the stream. In this
// case it is called syncronization chunk (sync-chunk). 'sync' field indicates
// that we have correct time stamp and 'time' field is this stamp.
//
// (see Sync class at sync.h for more information).
// 
// End of stream flag
// ===================
// End-of-stream chunk (eos-chunk) is method to correctly finish the stream. 
// eos-chunk may contain data and stream is assumed to end after the last 
// byte/sample of this chunk. 'eos' field indicates eos-chunk.
//
// Format change
// =============
// There are 2 ways to change the stream format and start new stream:
// * Forced. Send chunk with new format without sending eos-chunk before or
//   set_input() call without flush() call before. In this case sink should
//   drop all internal buffers immediately without waiting this data to
//   playback and switch to the new format. New data is considered to be not
//   connected with previous. This method provides fastest switching to the
//   new foramt.
// * Flushing. Send eos-chunk with old format before sending chunk with new
//   format or flush() call before set_input(). In this case sink should 
//   guarantee that all buffered data is sent to output before receiving new
//   data. Flushing is also used to correctly finish playback. This way
//   guarantees that we don't loose audio tail. 
//
// Of course new format should be supported. Unsupported format will lead to
// fail of process() or set_input() call and immediate stop of playback in 
// case of forced format change or correct finish of current stream and stop
// in case of flushing.
//
// Syncronization
// ==============
// Audio source may indicate position in the audio stream. All filters must
// pass this informtion correctly. But this may be a hard task because of
// a simple rule: time stamp is applied to the first syncpoint of incoming 
// data. Where syncpoint is:
// * each sample for linear format
// * first byte of interleaved PCM sample for PCM format
// * first byte of a packet for packeted format (ac3/dts/mpa/pes, etc)
// * reasonable place for other formats (should be documented)
//
// This rule comes from MPEG where presentation timestamp of PES packet is
// applied to the first syncpoint of wrapped elementary stream. In our case
// filter may receive timestamped chunk of multichannel PCM data beginning 
// from half-a-sample. This requires to extend the concept of syncpoint.
//
// End of stream
// =============
// See end of stream flag and format change...

class Chunk
{
public:
  /////////////////////////////////////////////////////////
  // Data

  Speakers  spk;

  uint8_t  *rawdata;
  samples_t samples;
  size_t    size;

  bool      sync;
  vtime_t   time;
            
  bool      eos;

  /////////////////////////////////////////////////////////
  // Utilities

  Chunk(): spk(spk_unknown), sync(false), time(0), eos(false), size(0), rawdata(0)
  {}

  Chunk(Speakers _spk, samples_t _samples, size_t _size,
    bool _sync = false, vtime_t _time = 0, bool _eos  = false)
  {
    set(_spk, _samples, _size, _sync, _time, _eos);
  }

  Chunk(Speakers _spk, uint8_t *_rawdata, size_t _size,
    bool _sync = false, vtime_t _time = 0, bool _eos  = false)
  {
    set(_spk, _rawdata, _size, _sync, _time, _eos);
  }

  inline void set(Speakers _spk, samples_t _samples, size_t _size,
    bool _sync = false, vtime_t _time = 0, bool _eos  = false)
  {
    spk = _spk;
    rawdata = 0;
    samples = _samples;
    size = _size;
    sync = _sync;
    time = _time;
    eos = _eos;
  }

  inline void set(Speakers _spk, uint8_t *_rawdata, size_t _size,
    bool _sync = false, vtime_t _time = 0, bool _eos  = false)
  {
    spk = _spk;
    rawdata = _rawdata;
    samples.zero();
    size = _size;
    sync = _sync;
    time = _time;
    eos = _eos;
  }

  inline bool is_empty() const
  { 
    return size == 0; 
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
//   Check if we can change format to proposed. Should work as fast as 
//   possible because it may be used to try numerous formats to find 
//   acceptable conversion. Also, this function may be called asynchronously
//   from other thread, so it should be thread-safe. 
//
//   It is assumed that set_input() succeeds if this function returns true.
//   But it is possible that sink may change its mind because some resources
//   may be locked by other applications. For example imagine that some audio
//   device supports SPDIF output. query_input() may check it and report that
//   resource is free. But before set_input() call some other application
//   locks SPIDF output. Of course set_input() will fail.
//
//   Other scenario. Some sink has option to block some formats. So in 
//   following scenario set_input() will fall:
//
//   sink.allow(format1);
//   sink.allow(format2);
//
//   if (sink.query_input(format1)) // ok. can change format to 'format1'
//     sink.set_input(format1);     // ok. format is allowed...
// 
//   if (sink.query_input(format2)) // ok. can change format to 'format2'
//   {
//     sink.disallow(format2);      // change our mind about 'format2'
//     sink.set_input(format2);     // always fail!
//   }
//
//   Such scenarios with shared resources, sink options, etc must be 
//   documented because it may require special ways to work with such sink.
//
// set_input() [working thread, blocking]
//   Switch format. If sink has buffered data it should immediately drop it,
//   stop playback and prepare to receive new data. Even if we switch format
//   to the same one. I.e. following scenario provides fast switch to the new
//   audio stream of the same format:
//
//   sink.set_input(pcm16); // prepare to output pcm16 data
//   sink.process(chunk1);  // buffer some data and possibly start playback
//   sink.set_input(pcm16); // stop playback and drop remaining data
//   sink.process(chunk2);  // buffer new data and possibly start playback
//
//   It is expected that call succeeds after successful query_input() call 
//   (see query_input() for more info).
//
//   It may require some time to change format (initialize output device, 
//   allocate buffers, etc). Sound output is not required to be continious 
//   and clickless in case of format changes, but it is highly recommended 
//   to produce minimum artifacts.
//
// get_input() [thread-safe, fast]
//   Just report current input format. If function reports FORMAT_UNKNOWN it
//   means that sink is not initialized. In all other cases it is supposed 
//   that query_input(get_input()) returns true. If chunk format differs from
//   get_input() result it means that format change procedure will be evaluted
//
// process() [working thread, critical path]
//   Receive data chunk. Returns true on success and false otherwise. This
//   function must carry about possible events: format change, syncronization,
//   end of stream. 
//
//   For audio renderers this call may block working thread during audio 
//   playback.
//
// flush() [working thread, blocking]
//   Flush buffered data. Used to playback tail of an audio stream to correctly
//   finish playback or before format change. After flush() call all internal
//   buffers should be flushed and playback stopped so set_input() call should
//   just prepare to receive new format.
//
//   For audio renderers this call may block working thread during audio
//   playback.

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
  virtual bool process(const Chunk *_chunk)     { spk = _chunk->spk; return true; }
};


///////////////////////////////////////////////////////////////////////////////
// Source class
//
// Abstract audio source.
//
// get_output() [thread-safe, fast]
//   Report format of next output chunk. Primary purpose of this call is to 
//   setup downstream audio sink before processing because format switch may
//   be time-consuming operation. When source is empty (is_empty() == true)
//   it may report any format and may change it. But when source is full
//   (is_empty() == false) it must report format exactly as it will appear
//   at next output chunk. So returned value may change either when
//   is_empty() == true or after get_chunk() call (standard format change).
//
// is_empty() [thread-safe, critical path]
//   Check if we can get some data from this source. Depending on source type
//   it may require some explicit actions to be filled (like filters) or it 
//   may be filled asyncronously (like audio capture). May be called 
//   asynchronously.
//
// get_chunk() [working thread, critical path]
//   Receive data from the source. Should be called only from working thread.
//   If empty chunk is returned it does not mean that source becomes empty.
//   Check it with is_empty() call. This call may block working thread (?).
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
// Data processing model (working thread):
//
// . . . . . . . . . | . . . . . . . . . . . . . . . . . . . . . . . . . . . .
// .                 |                                           Initialize  .
// .                 |                                                       .
// .   +------------>|<--------------------------------------------------+   .
// .   |             v                  ^              ^                 |   .
// .   |      +-------------+           |              |                 |   .
// .   |      |   reset()   |           |              |                 |   .
// .   |      | set_input() |           |        ...any state            |   .
// .   |      +-------------+           |                                |   .
// .   |             |                  |                                |   .
// . . | . . . . . . | . . . . . . . . .|. . . . . . . . . . . . . . . . |   .
// .   |             |                  |    .                           |   .
// .   |             |<-------------+   |    .                           |   .
// .   |     ----------------       |   |    .                           |   .
// .   |   <  have more data? >---- | - | --------------------+          |   .
// .   |     ----------------   no  |   |    .                |          |   .
// .   |         yes |              |   |    .                v          |   .
// .   |             v              |   |    .           +---------+     |   .
// .   |        ----------          |   |    .           | flush() |     |   .
// .   |      (  get data  )        |   |    .           +---------+     |   .
// .   |      ( to process )        |   |    .                |          |   .
// .   |        ----------          |   |    .  +------------>|          |   .
// .   |             |              |   |    .  |             v          |   .
// .   |             v              |   |    .  |        -----------     |   .
// .   | error +-----------+        |   |    .  |      < is_empty()? >---+   .
// .   +-------| process() |        |   |    .  |        -----------  true   .
// .           +-----------+        |   |    .  |       false |              .
// .                 |              |   |    .  |             |              .
// .   +------------>|              |   |    .  |             v              .
// .   |             v              |   |    .  |  err +-------------+       .
// .   |        -----------   true  |   |<----- | -----| get_chunk() |       .
// .   |      < is_empty()? >-------+   |    .  |      +-------------+       .
// .   |        -----------             |    .  |             |              .            
// .   |       false |                  |    .  |             v              .
// .   |             |                  |    .  |      ---------------       .
// .   |             v                  |    .  |    (  do something   )     .
// .   |      +-------------+  error    |    .  |    ( with chunk data )     .
// .   |      | get_chunk() |-----------+    .  |      ---------------       .
// .   |      +-------------+                .  |             |              .
// .   |             |                       .  +-------------+              .
// .   |             v                       .                               .
// .   |      ---------------                .                Flushing loop  .
// .   |    (  do something   )              . . . . . . . . . . . . . . . . .                              
// .   |    ( with chunk data )              .
// .   |      ---------------                .
// .   |             |                       .
// .   +-------------+                       .
// .                        Processing loop  .
// . . . . . . . . . . . . . . . . . . . . . .
//
// Filter may process data in-place at buffers received with process() call.
// This allows to avoid data copy from buffer to buffer so speed up processing
// but we should always remember about data references. After process() call
// filter may hold references to upstream data buffers so we must not alter it
// until the end of processing.
//
// Filter has 3 states:
//
// * empty
//   Filter has no data to output and waits for more data to input. In this 
//   state filter may have some data cached at internal buffes. But it is 
//   guaranteed that filter has no references to any external data buffers
//   received with process() call.
//
//   * is_empty() returns true.
//   * process() with non-eof chunk may switch filter to full state.
//   * process() with eof-chunk must switch filter to flushing state.
//   * get_chunk() returns empty chunk with no state change.
//
// * full
//   Filter has some data to output. In this state it cannot receive data
//   and may have references to the data received with process() call, so 
//   external procedures should not alter this data.
//
//   * is_empty() returns false.
//   * get_chunk() may return empty and non-empty chunks.
//   * get_chunk() may switch filter to empty state.
//   * process() call result is undefined (maybe it should fail?)
//
// * flushing
//   In this state filter releases data cached at its internal buffers.
//   After this call all external references of the filter are dropped.
//
//   is_empty() returns false.
//   get_chunk() may return empty and non-empty chunks without state change.
//   get_chunk() may return eof-chunk and switch to empty state.
//   process() call result is undefined.
//           
// reset() [working thread]
//   Reset filter state to empty, drop all internal buffers and all external 
//   references.
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
//
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
    if (spk != _chunk->spk)
      FILTER_SAFE(set_input(_chunk->spk));

    // remember input chunk info
    if (_chunk->sync) // ignore non-sync chunks
    {
      sync     = true;
      time     = _chunk->time;
    }
    flushing = _chunk->eos;
    rawdata  = _chunk->rawdata;
    samples  = _chunk->samples;
    size     = _chunk->size;

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
    spk  = spk_unknown;
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
