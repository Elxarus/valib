/*
  Source

  void reset()
    Reset the source into initial state and set stream position to the start of
    the stream.

  bool get_chunk(Chunk &out);
    Get a new chunk from the source. Returns true when the chunk made
    successfully, and false when it is no more data to return (end of the
    stream). Throws SourceError when some error occurs and source cannot
    continue.

  bool new_stream() const;
    Source returns a new stream. It may do this for the following reasons:
    * It want the downstream to flush and prepare to receive a new stream.
    * It wants to change the output format
    get_chunk() call affect this flag

  Speakers get_output() const;
    Returns output format of the source.

    Generally, output format is known immediately after the source is open, so
    you can rely on this and init the following filter (or sink). But in some
    cases source may not determine the format after open. In this case output
    format must be set to FORMAT_UNKNOWN after open, and to the real format on
    the first output chunk.

    Most sources do not change output format during the processing, but some
    can. To change output format source must explicitly indicate this with
    help of new_stream() call (it should return true when format changes).

  std::string name() const
    Returns the name of the source (class name by default).

  std::string info() const
    Print the source configuration. Only static parameters should be printed.
*/

#ifndef VALIB_SOURCE_H
#define VALIB_SOURCE_H

#include <string>
#include <boost/utility.hpp>
#include "chunk.h"

using std::string;

class Source;
class SourceError;



class Source : boost::noncopyable
{
public:
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

///////////////////////////////////////////////////////////////////////////////
// SourceError exception

class SourceError : public ProcError
{
public:
  SourceError(Source *source_, int error_code_, string text_):
  ProcError(source_->name(), source_->info(), error_code_, text_)
  {}
};

#endif
