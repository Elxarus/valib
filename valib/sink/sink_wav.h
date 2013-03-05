/**************************************************************************//**
  \file sink_wav.h
  \brief WAVSink class
******************************************************************************/

#ifndef VALIB_SINK_WAV
#define VALIB_SINK_WAV

#include "../sink.h"
#include "../auto_file.h"

/**************************************************************************//**
  \class WAVSink
  \brief Writes incoming data to WAV file.

  File must be open with open_file() funtion before Sink interface may be used.
  Before open_file() call sink refuses to open at all (can_open() returns
  false).

  After file is open, sink may be open with any format that is convertable to
  WAVEFORMATEX with spk2wfe() function.

  flush(), close() and close_file() are optional, WAVSink closes the file
  correctly on destruction. But the resulting wav file becomes valid only after
  closing the file.

  Usage example:
  \code
  WAVSink sink;
  if (!sink.open(file_name))
  {
    // Error: file cannot be open
    return;
  }
  sink.open_throw(spk);

  // Write data to WAV file
  while (get_data(chunk))
    sink.process(chunk);

  // Close
  sink.flush(); // optional
  sink.close(); // optional
  sink.close_file(); // optional
  \endcode

  \fn WAVSink::WAVSink();
    Default constructor. Does not open file or sink.

  \fn WAVSink::WAVSink(const char *file_name);
    This constructor opens the file specified. Does not throw on error. You
    should check is_file_open() explicitly before using the sink.

  \fn WAVSink::~WAVSink()
    Closes the file if nessesary.

  \fn bool WAVSink::open_file(const char *file_name);
    \paran file_name File name to write to.

    Open the file specified for writing. Does not add .wav extension, it must
    be done explicitly if required.

    Does not opens the sink itself. It must be done explicitly with open() call.

    Returns true on success and false otherwise.

  \fn void WAVSink::close_file();
    Closes the file currently open.

  \fn bool WAVSink::is_file_open() const;
    Returns true if file is currently open.

  \fn string WAVSink::filename() const;
    Returns the name of the file currently open.

******************************************************************************/

class WAVSink : public SimpleSink
{
protected:
  AutoFile f;            //!< file we're writing to
  string fname;          //!< open file name
  uint32_t header_size;  //!< WAV header size;
  uint64_t data_size;    //!< data size written to the file
  void *file_format;     //!< WAVEFORMAT * of the open file

  void init_riff();      //!< Writes dummy RIFF header
  void close_riff();     //!< Updates the RIFF header before closing the file

public:
  WAVSink();
  WAVSink(const char *file_name);
  ~WAVSink();

  bool open_file(const char *file_name);
  void close_file();
  bool is_file_open() const;
  string filename() const;

  /////////////////////////////////////////////////////////
  // Sink interface

  virtual bool can_open(Speakers new_spk) const;
  virtual bool init();
  virtual void process(const Chunk &chunk); 
};

#endif
