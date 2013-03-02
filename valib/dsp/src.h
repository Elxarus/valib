/**************************************************************************//**
  \file src.h
  \brief Sample rate conversion: StreamingSRC, BufferSRC
******************************************************************************/

#ifndef VALIB_SRC_H
#define VALIB_SRC_H

#include "../defs.h"

//! Sample rate conversion parameters.
struct SRCParams
{
  int fs;   //!< Source sampling rate
  int fd;   //!< Destination sampling rate
  double a; //!< Attenuation in dB
  double q; //!< Quality factor

  SRCParams():
  fs(0), fd(0), a(0), q(0)
  {}

  SRCParams(int fs_, int fd_, double a_ = 100, double q_ = 0.99):
  fs(fs_), fd(fd_), a(a_), q(q_)
  {}
};

/**************************************************************************//**
  \class StreamingSRC
  \brief Sample rate conversion for streaming.

  Allows to resample long streams step-by-step.

  Converter must be initialized before use in constructor or with open() call.
  Calling close() is optional, rquired only if you want to free resources
  explicitly. is_open() tells you about the current state.
  
  Parameters for conversion:
  - Source sampling frequency.
  - Destination sampling frequency.
  - Attenuation in dB. Simply saying, it defines the amount of noise intruduced
    by the conversion (more attenuation means less noise).
  - Quality factor. It is a normalized passband bandwidth. In other words, if
    you convert 48000 to 44100, with quality 0.9, frequencies up to 19845Hz
    will be preserved perfectly and 19845Hz - 22050Hz will be filtered out.
    I.e. 90% of the frequency range will be preserved.
    Available range for this parameter is (0..1) (not including the bounds).

  Processing is done only in blocks. Thus it is required to fill the internal
  buffer before processing step. Also flushing is required at the end
  of the stream.
  
  So processing of a stream looks like:

  fill-process-fill-process-...-fill-process-flush-flush.

  \section usage Usage example

  \subsection convert_stream Convert a stream.

  \code
    StreamingSRC src;
    src.open(in_rate, out_rate, 100, 0.99);
    assert(src.is_open())
    
    while (have_data())
    {
      sample_t *chunk = get_chunk();
      size_t chunk_size = get_chunk_size();

      while (chunk_size)
      {
        size_t gone = src.fill(chunk, chunk_size);
        chunk += gone;
        chunk_size -= gone;

        if (src.can_process())
        {
          src.process();
          sample_t result = src.result();
          size_t result_size = src.size();

          // Do something with the result
          // ...
        }
      }
    }

    while (src.need_flushing())
    {
      src.flush();
      sample_t result = src.result();
      size_t result_size = src.size();

      // Do something with the result
      // ...
    }
  \endcode

  \fn StreamingSRC::StreamingSRC();
    Constructs a non-initialized converter.

  \fn StreamingSRC::StreamingSRC(int fs, int fd, double a, double q);
    \param fs Source sampling frequency.
    \param fd Destination sampling frequency.
    \param a  Attenuation in dB.
    \param q  Quality (normalized passband width).

    Constructs and initializes the converter.

  \fn void StreamingSRC::open(int fs, int fd, double a, double q);
    \param fs Source sampling frequency.
    \param fd Destination sampling frequency.
    \param a  Attenuation in dB.
    \param q  Quality (normalized passband width).

    Initialize the converter.

  \fn void StreamingSRC::close();
    Uninitialize the converter and free buffers.

  \fn bool StreamingSRC::is_open() const;
    Returns true when converter is initialized and false otherwise.

  \fn void StreamingSRC::reset();
    Resets the converter to the initial state and drops the data buffered.
    Used to forget the stream it was working on before and prepare to convert
    another stream. Does not free any buffers.

    reset() is not nessesary after open(), but required after flushing.

  \fn size_t StreamingSRC::fill(const sample_t *in, size_t size);
    Fill the internal buffer. Returns the number of samples used from input.
    May return zero when internal buffer is full and ready to be processed.

  \fn bool StreamingSRC::can_process() const;
    Return true when internal buffer is full and ready to be processed and
    false otherwise.

  \fn void StreamingSRC::process();
    Process the internal buffer. Result is available with result() and size().

    Must be called only when can_process() returns true.

  \fn bool StreamingSRC::need_flushing() const;
    Returns thrue when flushing is required to correctly finish the stream and
    false otherwise.

  \fn void StreamingSRC::flush();
    Process and return the end of the stream. Note, that several flush() calls
    may be required.

    Must be called only when need_flushing() returns true.

  \fn inline sample_t *StreamingSRC::result() const;
    Return the pointer to the buffer holding the result. It's safe to modify
    this buffer (up to its size), thus inplace processing of the result is
    possible.

  \fn inline size_t StreamingSRC::size() const;
    Returns the size of the buffer returned by result().

******************************************************************************/

class StreamingSRC
{
public:
  StreamingSRC();
  StreamingSRC(const SRCParams &params);
  StreamingSRC(int fs, int fd, double a = 100, double q = 0.99);
  ~StreamingSRC();

  bool open(const SRCParams &params);
  bool open(int fs, int fd, double a, double q);
  void close();
  bool is_open() const;

  void reset();
  size_t fill(const sample_t *in, size_t size);

  bool can_process() const;
  void process();

  bool need_flushing() const;
  void flush();

  inline sample_t *result() const
  { return f_result; }
  inline size_t size() const
  { return f_size; }
  inline const SRCParams &params() const
  { return f_params; }

protected:
  class Impl;
  Impl *pimpl;

  SRCParams f_params;
  sample_t *f_result;
  size_t f_size;

private:
  // noncopyable
  StreamingSRC(const StreamingSRC &);
  StreamingSRC &operator =(const StreamingSRC &);
};

/**************************************************************************//**
  \class BufferSRC
  \brief Sample rate conversion for buffer.

  Does the sample rate conversion for the whole buffer at once.

  Converter must be initialized before use in constructor or with open() call.
  Calling close() is optional, rquired only if you want to free resources
  explicitly. is_open() tells you about the current state.
  
  Parameters for conversion:
  - Source sampling frequency.
  - Destination sampling frequency.
  - Attenuation in dB. Simply saying, it defines the amount of noise intruduced
    by the conversion (more attenuation means less noise).
  - Quality factor. It is a normalized passband bandwidth. In other words, if
    you convert 48000 to 44100, with quality 0.9, frequencies up to 19845Hz
    will be preserved perfectly and 19845Hz - 22050Hz will be filtered out.
    I.e. 90% of the frequency range will be preserved.
    Available range for this parameter is (0..1) (not including the bounds).

  \section usage Usage example

  \subsection convert_single Convert single buffer

  \code
    BufferSRC src(in_rate, out_rate, 100, 0.99);
    src.process(buf, size);

    sample_t *result = src.result();
    size_t result_size = src.size();

    // Do something with the converted buffer
    // ...
  \endcode

  \subsection convert_many Convert multiple buffers.

  \code
    BufferSRC src;
    src.open(in_rate, out_rate, 100, 0.99);
    assert(src.is_open())

    for (each buffer)
    {
      src.convert(get_buffer(), get_buffer_size());
      sample_t *result = src.result();
      size_t result_size = src.size();

      // Do something with the converted buffer
      // ...
    }
  \code

******************************************************************************/

class BufferSRC
{
public:
  BufferSRC();
  BufferSRC(const SRCParams &params);
  BufferSRC(int fs, int fd, double a, double q);
  ~BufferSRC();

  bool open(const SRCParams &params);
  bool open(int fs, int fd, double a, double q);
  void close();
  bool is_open() const;

  void process(const sample_t *in, size_t size);

  inline sample_t *result() const
  { return f_result; }
  inline size_t size() const
  { return f_size; }
  inline const SRCParams &params() const
  { return f_params; }

protected:
  class Impl;
  Impl *pimpl;

  SRCParams f_params;
  sample_t *f_result;
  size_t f_size;

private:
  // noncopyable
  BufferSRC(const BufferSRC &);
  BufferSRC &operator =(const BufferSRC &);
};

#endif
