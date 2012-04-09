/**************************************************************************//**
  \file sink.h
  \brief Sink: Base interface for audio sink
******************************************************************************/

#ifndef VALIB_SINK_H
#define VALIB_SINK_H

#include <boost/utility.hpp>
#include "chunk.h"
#include "exception.h"

class Sink;
class SimpleSink;

/**************************************************************************//**
  \class Sink
  \brief Abstract base for audio sink.

  \name Open & close the sink

  \fn bool Sink::can_open(Speakers spk) const
    \param spk Format to test
    \return Returns true when format is supported and false otherwise.

    Check format support. Returns true when the sink supports the format
    given. Note that sink may fail to open even when the format is supported
    because of resource allocation errors.

    An ability to open the sink with a certain format also depends on sink
    parameters. When parameters change can_open() result may also change.

    This function should not throw because of resource allocation errors or
    other reasons. I.e. it should catch all exceptions during the test and
    return false in such case.

  \fn bool Sink::open(Speakers spk)
    \param spk Input format for the sink
    \return Returns true on success and false otherwise.

    Open the sink with the input format provided and allocate resources.

    It is not nessesary to call close() on already open sink because open()
    may reuse previously allocated resources.

    After a successful call to open() get_input() must return the format
    passed to open(), and do this all the time until close() call.

  \fn void Sink::close();
    Close the filter and deallocate resources.

    The use of close() is totally optional. Moreover, use of close() before
    open() is not recommended because open() may reuse previously allocated
    resources.

    This function should not throw.

  \fn bool Sink::is_open() const
    \return Return true when the sink is open and can process data.

    This function should not throw.

  \fn Speakers Sink::get_input() const;
    Returns input format of the sink. It must be the same format as passed
    to open() call. This function should be used only when the filter is open,
    otherwise the result is undefined.

  \name Processing

  \fn void Sink::reset()
    Reset the sink state, clean all internal buffers and prepare to a new
    stream. Do not deallocate resources, because we're awaiting new data.
    This call should be equivalent to sink.open(filter.get_input()), but we
    do not need to allocate resources once again, just set the internal state
    to initial values.

    This function should not throw.

  \fn void Sink::process(const Chunk &in)
    \param in Input data

    Put data into the sink. In contrast to Filter class, we do not need to
    call process() several times for the same chunk, and it does not change
    the data in the chunk.

    For audio renderers this call may block the working thread during audio 
    playback.

    After receiving the first chunk after open() call, audio renderer may
    start the playback immediately, or it may buffer some data before. I.e.
    it may be nessesary to process several chunks befor the playback actually
    starts.

    When sink finds an error and cannot proceed, it must throw Sink::Error
    exception. reset() must be called on the sink after an error.

  \fn void Sink::flush()
    Flush buffered data. In contrast to Filter class, we do not need to
    call flush() several times.

    For audio renderers this call may block the working thread during audio 
    playback.

    When sink finds an error and cannot proceed, it must throw SinkError
    exception. reset() must be called on the sink after an error occurs.

  \name Sink info

  \fn std::string Sink::name() const
    Returns the name of the sink (class name by default).

  \fn std::string Sink::info() const
    Print the sink configuration. Only static parameters should be printed.
******************************************************************************/

class Sink : boost::noncopyable
{
public:
  //! Processing error exception
  struct Error : public EProcessing {};

  Sink() {}
  virtual ~Sink() {}

  /////////////////////////////////////////////////////////
  // Open/close the sink

  virtual bool can_open(Speakers spk) const = 0;
  virtual bool open(Speakers spk) = 0;
  virtual void close() = 0;
  virtual bool is_open() const = 0;
  virtual Speakers get_input() const = 0;

  /////////////////////////////////////////////////////////
  // Processing

  virtual void reset() = 0;
  virtual void process(const Chunk &in) = 0;
  virtual void flush() = 0;

  /////////////////////////////////////////////////////////
  // Sink info

  virtual string name() const;
  virtual string info() const { return string(); }

  /////////////////////////////////////////////////////////
  // Utilities

  inline void open_throw(Speakers spk)
  {
    assert(!spk.is_unknown());
    if (!open(spk))
      THROW(EOpenSink() 
        << errinfo_spk(spk)
        << errinfo_obj_name(name()));
  }

  inline bool flush_open(Speakers spk)
  {
    assert(!spk.is_unknown());
    flush();
    return open(spk);
  }

  inline void flush_open_throw(Speakers spk)
  {
    assert(!spk.is_unknown());
    flush();
    open_throw(spk);
  }

};



/**************************************************************************//**
  \class SimpleSink
  \brief Default implementation for the most of the Sink interface.

  Following functions left unimplemented: can_open() and process()

  When sink requires an initialization, it should override init()/uninit()
  placeholders, instead of open()/close().

  Destructor of the sink must deallocate the resources allocated by init() if
  nessesary. Generally, it may be done by calling uninit().

  \name Open & close the sink

  \fn bool SimpleSink::open(Speakers new_spk)
    \param new_spk Input format

    Open the filter with the new format.

    Calls init().

    After this call:
    \li get_input() and get_output() return new_spk.
    \li is_open() returns true

  \fn void SimpleSink::close()
    Closes the filter.

    Calls uninit().

    After this call:
    \li get_input() and get_output() return spk_unknown.
    \li is_open() returns false.

  \fn bool SimpleSink::is_open() const
    Returns true when filter is open and false otherwise.

  \fn Speakers SimpleSink::get_input() const
    Default implementation returns the format passed to open() call.

  \name Initialization

  \fn bool SimpleSink::init() { return true; }
    Override this to do some initialization.

    spk member is specifies the current input format.

  \fn void SimpleSink::uninit() {}
    Override this to free resources allocated by init().

  \name Processing

  \fn void SimpleSink::reset()
    Default implementation does nothing.

  \fn void SimpleSink::flush()
    Default implementation does nothing.
******************************************************************************/

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

  virtual bool is_open() const
  { return f_open; }

  virtual Speakers get_input() const
  { return spk; }

  /////////////////////////////////////////////////////////
  // Processing

  virtual void reset()
  {}

  virtual void flush()
  {}

};

#endif
