/*
  Sink
  Abstract audio sink.

  bool can_open(Speakers spk) const;
    Check format support.
    Return value:
    * true: sink supports the format given. Note that sink may fail to open
      even when the format is supported because of resource allocation errors.
      Also sink may change its mind when you change its parameters.
    * false: filter cannot be open with the format given.


  bool open(Speakers spk);
    Open the sink and allocate resources.
    Return value:
    * true: success
    * false: fail

  void close();
    Close the filter and deallocate resources.

  bool is_open() const;
    Return true when filter is open and can process data.

  Speakers get_input() const;
    Returns input format of the filter. It must be the same format as passed
    to open() call. This function should be used only when the filter is open,
    otherwise the result is undefined.

  void reset();
    Terminate the current stream and prepare to receive a new one.
    Audio renderer should immediately stop playback and drop buffered data.

  void process(const Chunk &in);
    Put data into the sink. In contrast to Filter class, we do not need to
    call process() several times for the same chunk, and it does not change
    the data in the chunk.

    For audio renderers this call may block the working thread during audio 
    playback.

    After receiving the first chunk after open() call, audio renderer may
    start the playback immediately, or it may buffer some data before. I.e.
    it may be nessesary to process several chunks befor the playback actually
    starts.

    When sink finds an error and cannot proceed, it must throw SinkError
    exception. reset() must be called on the sink after an error occurs.

  void flush();
    Flush buffered data. In contrast to Filter class, we do not need to
    call flush() several times.

    For audio renderers this call may block the working thread during audio 
    playback.

    When sink finds an error and cannot proceed, it must throw SinkError
    exception. reset() must be called on the sink after an error occurs.

  std::string name() const
    Returns the name of the sink (class name by default).

  std::string info() const
    Print the sink configuration. Only static parameters should be printed.
*/

#ifndef VALIB_SINK_H
#define VALIB_SINK_H

#include <string>
#include <boost/utility.hpp>
#include "chunk.h"

using std::string;

class Sink;
class SinkError;
class SimpleSink;



class Sink : boost::noncopyable
{
public:
  Sink() {}
  virtual ~Sink() {}

  /////////////////////////////////////////////////////////
  // Open/close the filter

  virtual bool can_open(Speakers spk) const = 0;
  virtual bool open(Speakers spk) = 0;
  virtual void close() = 0;

  /////////////////////////////////////////////////////////
  // Processing

  virtual void reset() = 0;
  virtual void process(const Chunk &in) = 0;
  virtual void flush() = 0;

  // Sink state
  virtual bool     is_open() const = 0;
  virtual Speakers get_input() const = 0;

  // Sink info
  virtual string name() const;
  virtual string info() const { return string(); }
};

///////////////////////////////////////////////////////////////////////////////
// FilterError exception

class SinkError : public ProcError
{
public:
  SinkError(Sink *sink_, int error_code_, string text_):
  ProcError(sink_->name(), sink_->info(), error_code_, text_)
  {}
};

///////////////////////////////////////////////////////////////////////////////
// SimpleSink
// Default implementation for the most of the Sink interface. Following
// functions left unimplemented: can_open() and process()
//
// When filter sink initialization, it should override init()/uninit()
// placeholders, instead of open()/close().

class SimpleSink : public Sink
{
protected:
  bool f_open;
  Speakers spk;

public:
  SimpleSink(): f_open(false)
  {}

  /////////////////////////////////////////////////////////
  // Init/Uninit placeholders

  virtual bool init() { return true; }
  virtual void uninit() {}

  /////////////////////////////////////////////////////////
  // Open/close the sink
  // Do not override it directly, override init/uninit()
  // placehoilders.

  virtual bool open(Speakers new_spk)
  {
    if (!can_open(new_spk))
      return false;

    spk = new_spk;
    if (!init())
    {
      spk = spk_unknown;
      return false;
    }

    f_open = true;
    return true;
  }

  virtual void close()
  {
    uninit();
    f_open = false;
  }

  /////////////////////////////////////////////////////////
  // Processing

  virtual void reset()
  {}

  virtual void flush()
  {}

  virtual bool is_open() const
  { return f_open; }

  virtual Speakers get_input() const
  { return spk; }
};

#endif
