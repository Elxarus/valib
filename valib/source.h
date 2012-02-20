/**************************************************************************//**
  \file source.h
  \brief Source: Base interface for audio source
******************************************************************************/

#ifndef VALIB_SOURCE_H
#define VALIB_SOURCE_H

#include <boost/utility.hpp>
#include "chunk.h"
#include "exception.h"

/**************************************************************************//**
  \class Source
  \brief Abstract base for audio source.

  \name Processing

  \fn void Source::reset()
    Reset the source into an initial state and set stream position to the start
    of the stream.

    This function should not throw.

  \fn bool Source::get_chunk(Chunk &out)
    \param out Chunk that receives data

    Get next chunk from the source.

    Returns true when the chunk generated successfully, and false when it is no
    more data to return (end of the stream).

    Note, that this function may block the working thread waiting for data.

    Throws Source::Error when some error occurs and source cannot continue.

  \fn bool Source::new_stream() const
    Source returns a new stream. It may do this for the following reasons:
    \li It want the downstream to flush and prepare to receive a new stream.
    \li It wants to change the output format

    get_chunk() call affect this flag

  \fn Speakers Source::get_output() const
    Returns output format of the source.

    Generally, output format is known immediately after the source is open, so
    you can rely on this and init the following filter (or sink). But in some
    cases source may not determine the format after open. In this case output
    format must be set to FORMAT_UNKNOWN after open, and to the real format on
    the first output chunk.

    Most sources do not change output format during the processing, but some
    can. To change output format source must explicitly indicate this with
    help of new_stream() call (it should return true when format changes).

  \name Source info

  \fn std::string Source::name() const
    Returns the name of the source (class name by default).

  \fn std::string Source::info() const
    Print the source configuration. Only static parameters should be printed.
******************************************************************************/

class Source : boost::noncopyable
{
public:
  //! Processing error exception
  struct Error : public EProcessing {};

  Source() {}
  virtual ~Source() {}

  /////////////////////////////////////////////////////////
  // Processing

  virtual void reset() = 0;
  virtual bool get_chunk(Chunk &out) = 0;
  virtual bool new_stream() const = 0;
  virtual Speakers get_output() const = 0;

  // Source info
  virtual string name() const;
  virtual string info() const { return string(); }
};

#endif
