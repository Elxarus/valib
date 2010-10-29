/**************************************************************************//**
  \file filter.h
  \brief Filter: base interface for filters
******************************************************************************/

#ifndef VALIB_FILTER2_H
#define VALIB_FILTER2_H

#include <string>
#include <boost/utility.hpp>
#include "chunk.h"

using std::string;

class Filter;

/**************************************************************************//**
  \class Filter
  \brief Base interface for all filters

  \section filter_usage Filter usage

  \subsection no_format_change No format changes

  \code
  Filter *filter;
  Chunk in, out;
  ...

  try
  {
    filter->open(format);
    while (has_more_data())
    {
      in = get_more_data();
      while (filter->process(in, out);)
        do_something(out);
    }

    while (filter->flush(out);)
      do_something(out);

    filter->close();
  }
  catch (Filter::Error)
  {
    // Handle filtering errors
  }
  \endcode


  \subsection format_chagne Format change

  Change input format of the filter without interruption of the data flow.

  \code
  Filter *filter;
  ...

  try
  {
    filter->open(format);
    ...
    while (has_more_data())
    {
      Chunk in = get_more_data();
      Chunk out;

      if (new_stream())
      {
        while (filter->flush(out))
          do_something(out);
        filter->open(new_format());
      }

      while (filter->process(in, out))
        filter->process(in, out);
    }

    while (filter->flush(out))
      do_something(out);

    filter->close();
  }
  catch (Filter::Error)
  {
    // Handle filtering errors
  }
  \endcode

  \name Open & close the filter

  \fn bool Filter::can_open(Speakers spk) const
    \param spk Format to test
    \return Returns true when format is supported and false otherwise.

    Check format support. Returns true when the filter supports the format
    given. Note that filter may fail to open even when the format is supported
    because of resource allocation errors.

    An ability to open the filter with a certain format also depends on filter
    parameters. When parameters change filter can_open() result may also
    change.

    This function should not throw because of resource allocation errors or
    other reasons. I.e. it should catch all exceptions during the test and
    return false in such case.

  \fn bool Filter::open(Speakers spk)
    \param spk Input format for the filter
    \return Returns true on success and false otherwise.

    Open the filter with the input format provided and allocate resources.

  \fn void Filter::close()
    Close the filter and deallocate resources.

    This function should not throw.

  \fn bool Filter::is_open() const
    \return Return true when filter is open and can process data.

    This function should not throw.

  \name Processing

  \fn void Filter::reset()
    Reset the filter state, clean all internal buffers and prepare to a new
    stream. Do not deallocate resources, because we're awaiting new data.
    This call should be equivalent to filter.open(filter.get_input()), but we
    do not need to allocate resources once again, just set the internal state
    to initial values.

    This function should not throw.

  \fn bool Filter::process(Chunk &in, Chunk &out)
    \param in Input data
    \param out Output data
    \retval true  Output chunk contains data.
    \retval false Filter needs more input data. Output data is undefined.

    Process input data and make one output chunk.

    Filter may process only part of the data. In this case it may change input
    chunk's pointers to track the progress. So you may need several process()
    calls with the same input chunk (filter may make several output chunks for
    one input chunk when it is too big).

    Filter may have its own buffer. In this case output chunk points to this
    buffer. When the buffer is not big enough to process the whole input chunk
    at once filter may return several output chunks.

    Filter may process data inplace, i.e. change data at the input buffer and
    return pointers to the input buffer. In some cases you should be warned
    abount this fact. For example imagine that you have 2 filters in the chain
    and the first filter returns its own buffer, and the second is inplace
    filter. You call process() on the first and feed inplace filter with the
    output. After this, process() call on the first filter will corrupt data
    at the output of the second filter.

    When filter finds an error and cannot proceed, it must throw Filter::Error
    exception. reset() must be called on the filter after an error occurs.

  \fn bool Filter::flush(Chunk &out)
    \param  out   Output data
    \retval true  Output chunk contains data.
    \retval false Filter is flushed. Output data is undefined.

    Flush buffered data. 

    When filter does not require flushing it should just return false.

    Note, that filter may produce several chunks when flushing, so several
    flush() calls may be required.

    When filter finds an error and cannot proceed, it must throw Filter::Error
    exception. reset() must be called on the filter after an error occurs.

  \fn bool Filter::new_stream() const
    Filter returns a new stream. It may do this for the following reasons:
    \li It want the downstream to flush and prepare to receive a new stream.
    \li It wants to change the output format
    Both process() and flush() calls may affect this flag

    This flag should appear only for the first chunk in the stream. An example:

    \verbatim
    call            result  new_stream()  get_output()  comment
    ---------------------------------------------------------------------------
    open(spk)       true    false         out_spk       We know the output format immediately
    process(chunk1) true    false         out_spk       1st chunk of format out_spk
    process(chunk1) false   false         out_spk       Need more data
    process(chunk2) true    false         out_spk       2nd chunk of format out_spk
    process(chunk2) true    true          new_out_spk   Format changed, 1st chunk of the new format new_out_spk
    process(chunk2) true    false         new_out_spk   New stream continues, 2nd chunk of format new_out_spk

    call            result  new_stream()  get_output()  comment
    ---------------------------------------------------------------------------
    open(spk)       true    false         spk_unknown   Output format is initially unknown
    process(chunk1) false   false         spk_unknown   Buffering, need more data
    process(chunk2) true    true          out_spk       Ouput format is determined, 1st chunk of format out_spk
    process(chunk2) true    false         out_spk       Stream continues, 2nd chunk of format out_spk
    \endverbatim

  \name Filter state

  \fn bool Filter::is_ofdd() const
    In some cases filter cannot determine the output format after open() and
    have to receive some data to say what the output format is. In this case
    format is set to FORMAT_UNKNOWN after open(), and the real format is known
    with the first output chunk.

    Filter must report such behaviour with this function.

  \fn Speakers Filter::get_input() const
    Returns input format of the filter. It must be the same format as passed
    to open() call. This function should be used only when the filter is open,
    otherwise the result is undefined.

  \fn Speakers Filter::get_output() const
    Returns output format of the filter.

    Generally, output format is known immediately after open() call, so you can
    rely on this and init the next filter in the filter chain. But in some
    cases filter cannot determine the format after open() and have to receive
    some data to say what the output format is. In this case output format must
    be set to FORMAT_UNKNOWN after open(), and to the real format on the first
    output chunk.

    Most filters do not change output format during the processing, but some
    can. To change output format filter must explicitly indicate this with
    help of new_stream().

  \name Filter info

  \fn std::string Filter::name() const
    Returns the name of the filter (class name by default).

  \fn std::string Filter::info() const
    Print the filter configuration. Only static parameters should be printed,
    i.e. at the DRC filter we should print the drc factor, but not the current
    gain (that is constantly changing).

******************************************************************************/

class Filter : boost::noncopyable
{
public:
  //* Processing error exception
  class Error
  {
  public:
    Filter *filter; /// Filter that caused the exception
    int code;       /// Error code
    string text;    /// Error text

    Error(Filter *filter_, int code_, string text_):
    filter(filter_), code(code_), text(text_)
    {}

    Error(Filter *filter_, string text_):
    filter(filter_), code(-1), text(text_)
    {}
  };

  Filter() {};
  virtual ~Filter() {};

  /////////////////////////////////////////////////////////
  // Open & close the filter

  virtual bool can_open(Speakers spk) const = 0;
  virtual bool open(Speakers spk) = 0;
  virtual void close() = 0;
  virtual bool is_open() const = 0;

  /////////////////////////////////////////////////////////
  // Processing

  virtual void reset() = 0;
  virtual bool process(Chunk &in, Chunk &out) = 0;
  virtual bool flush(Chunk &out) = 0;
  virtual bool new_stream() const = 0;

  /////////////////////////////////////////////////////////
  // Filter state

  virtual bool     is_ofdd() const = 0;
  virtual Speakers get_input() const = 0;
  virtual Speakers get_output() const = 0;

  /////////////////////////////////////////////////////////
  // Filter info

  virtual string name() const;
  virtual string info() const { return string(); }
};



/**************************************************************************//**
  \class SimpleFilter
  \brief Default implementation for the most of the Filter interface.

  Following functions left unimplemented: can_open() and process().

  If filter requires an initialization, it should override init()/uninit()
  placeholders, instead of open()/close().

  Destructor of the filter must deallocate the resources allocated by init() if
  nessesary. Generally, it may be done by calling uninit().

  \name Open & close the filter

  \fn bool SimpleFilter::open(Speakers new_spk)
    Open the filter with the new format.

    Calls init().

    After this call:
    \li get_input() and get_output() return new_spk.
    \li is_open() returns true

  \fn void SimpleFilter::close()
    Closes the filter.

    Calls uninit().

    After this call:
    \li get_input() and get_output() return spk_unknown.
    \li is_open() returns false.

  \fn bool SimpleFilter::is_open() const
    Returns true when filter is open and false otherwise.

  \name Initialization

  \fn bool SimpleFilter::init()
    Override this to do some initialization.

  \fn void SimpleFilter::uninit()
    Override this to free resources allocated by init().

  \name Processing

  \fn void SimpleFilter::reset()
    Default implementation does nothing.

  \fn bool SimpleFilter::flush(Chunk &out)
    Default implementation does nothing (returns false).

  \fn bool SimpleFilter::new_stream() const
    Default implementation returns false (no format changes).

  \name Filter state

  \fn bool SimpleFilter::is_ofdd() const
    Default implementation returns false (format is known immediately after
    open() call).

  \fn Speakers SimpleFilter::get_input() const
    Default implementation returns the format passed to open() call.

  \fn Speakers SimpleFilter::get_output() const
    Default implementation returns the format passed to open() call.

******************************************************************************/

class SimpleFilter : public Filter
{
protected:
  bool f_open;
  Speakers spk;

private:
  /////////////////////////////////////////////////////////
  // Open & close the filter implementation

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
    spk = spk_unknown;
  }

  virtual bool is_open() const
  { return f_open; }

public:
  SimpleFilter(): f_open(false)
  {}

  /////////////////////////////////////////////////////////
  // Init/Uninit placeholders

  virtual bool init() { return true; }
  virtual void uninit() {}

  /////////////////////////////////////////////////////////
  // Processing implementation

  virtual void reset()
  {}

  virtual bool flush(Chunk &out)
  { return false; }

  virtual bool new_stream() const
  { return false; }


  /////////////////////////////////////////////////////////
  // Filter state implementaion

  virtual bool is_ofdd() const
  { return false; }

  virtual Speakers get_input() const
  { return spk; }

  virtual Speakers get_output() const
  { return spk; }
};



/**************************************************************************//**
  \class SamplesFilter
  \brief Filter that works with linear format.

  Most of the linear format filters accept any reasonable linear format.
  These class implements can_open() function to accept any fully-specified
  linear format.

  \fn bool SamplesFilter::can_open(Speakers spk) const
    Return true for any fully-specified linear format. I.e. the format must
    meet following conditions:
    \li Speakers::format == FORMAT_LINEAR
    \li Speakers::mask != 0
    \li Speakers::sample_rate != 0

******************************************************************************/

class SamplesFilter : public SimpleFilter
{
public:
  SamplesFilter()
  {}

  virtual bool can_open(Speakers spk) const
  {
    return spk.is_linear() && spk.mask != 0 && spk.sample_rate != 0;
  }
};



/**************************************************************************//**
  \class FilterWrapper
  \brief Wrapper that delegated the interface to another filter.

  Useful to make 'customized' filters that are special cases for more general
  filters. For example, Equalizer class is a specialization for Convolver.

  If no delegate was set, wrapper just does not work (you cannot open the
  filter).

  Filter name is not delegated. So the default behaviour is preserved, and
  you will get wrapper's class name, not the delegate's name.

  \fn void FilterWrapper::wrap(Filter *)
    Set the filter to delegate the interface to.

  \fn void FilterWrapper::unwrap()
    Forget the delegatee filter.

******************************************************************************/

class FilterWrapper : public Filter
{
private:
  Filter *f; /// Delegatee filter.

protected:
  void wrap(Filter *)
  { f = f; }

  void unwrap()
  { f = 0; }

public:
  FilterWrapper(): f(0) {}
  FilterWrapper(Filter *f_): f(f_) {}

  /////////////////////////////////////////////////////////
  // Open/close the filter

  virtual bool can_open(Speakers spk) const
  { return f? f->can_open(spk): false; }

  virtual bool open(Speakers spk)
  { return f? f->open(spk): false; }

  virtual void close()
  { if (f) f->close(); }

  /////////////////////////////////////////////////////////
  // Processing

  virtual void reset()
  { if (f) f->reset(); }

  virtual bool process(Chunk &in, Chunk &out)
  { return f? f->process(in, out): false; }

  virtual bool flush(Chunk &out)
  { return f? f->flush(out): false; }

  virtual bool new_stream() const
  { return f? f->new_stream(): false; }

  /////////////////////////////////////////////////////////
  // Filter state

  virtual bool is_open() const
  { return f? f->is_open(): false; }

  virtual bool is_ofdd() const
  { return f? f->is_ofdd(): false; }

  virtual Speakers get_input() const
  { return f? f->get_input(): spk_unknown; }

  virtual Speakers get_output() const
  { return f? f->get_output(): spk_unknown; }

  /////////////////////////////////////////////////////////
  // Filter info

  virtual string info() const
  { return f? f->info(): string(); }
};

#endif
