/**************************************************************************//**
  \file file_parser.h
  \brief FileParser: Provides synchronization and reads frames from a file.
******************************************************************************/

#ifndef VALIB_FILE_PARSER_H
#define VALIB_FILE_PARSER_H

#include <stdio.h>
#include "../auto_file.h"
#include "../buffer.h"
#include "../parser.h"
#include "../source.h"

/**************************************************************************//**
  \class FileParser
  \brief Provides synchronization and reads frames from a file.

  Uses StreamBuffer to sychronize and read frames. Allows seeking and provides
  extended info about the file.

  This source has data-driven output format. I.e. it does not report the
  format immediately after file open. To actually detect the data format use
  probe().

  Also, seek() function drops output format back to FORMAT_UNKNOWN. You may
  also use probe() to determine the data format at the new point.

  Note, that it is a difference in handling new_stream() with and without
  probe(). Without probe(), new_stream() returns true after first get_chunk()
  call because output format is determined for the first time. With probe()
  new_stream() is not reported because format is already known.

  Call sequence without probe():
  \verbatim
  +-------------+----------------------------+----------
  | call        | result                     | Comment
  +-------------+----------------------------+----------
    open()        get_output() = spk_unknown   Data format is not known

    get_chunk()   get_output() = data_format   Data format is detected
                  new_stream() = true          Indicate the format change

    get_chunk()   get_output() = data_format   Data format remains the same
                  new_stream() = false         No format change
  \endverbatim

  Call sequence with probe():
  \verbatim
  +-------------+----------------------------+----------
  | call        | result                     | Comment
  +-------------+----------------------------+----------
    open()        get_output() = spk_unknown   Data format is not known

    probe()       get_output() = data_format   Detect data format explicitly

    get_chunk()   get_output() = data_format   Data format remains the same
                  new_stream() = false         No format change
  \endverbatim

  Therefore, you have to setup the downstream after probe() explicitly, but
  this setup will not be broken with new_stream() mechanism.
  
  If you still wish to use new_stream() format change mechanism to initialize
  downstream after probe(), you may call seek(0). This will drop the format
  back to FROMAT_UNKNOWN and new_stream() will work as usual.

  Usage example:
  \code
    Chunk chunk;
    FileParser f(file_name);

    if (!probe())
    {
      // File format cannot be determined
      // Handle error and exit
    }

    // Seeking
    f.seek(0.5, FileParser::relative);  // Seek to the middle of the file
    f.seek(1000000, FileParser::bytes); // Seek to 1Mb from the start

    // Gather stats to be able to navigate using time and frames
    if (f.stats())
    {
      f.seek(100, FileParser::time);    // Seek to 100sec (1:40)
      f.seek(1000, FileParser::frames); // Seek to 1000th frame
    }

    while (f.get_chunk(chunk))
    {
      if (f.new_stream())
      {
        // Handle file format change
      }

      // Now you have a frame at the chunk
      // Frame format is known with f.get_output()
    }
  \endcode

  \name File operations

  \fn bool FileParser::open(const string &filename, const HeaderParser *parser, size_t max_scan = 0)
    \param filename Name of the file to open
    \param parser   Parser to use
    \param max_scan Limit amount of data for synchronization

    Opens the file with name \c filename with the parser \c parser. The file
    format is not actually checked. You can do this explicitly with probe()
    call because it may take a lot of time.

    Parser must live until close() call.

    \c max_scan parameter limits amount of data that is passed to the stream
    parser before aborting the synchronization. Zero means no limiting.

    Limiting is useful because the file may be very large and when parser
    cannot find synchronization, it may read the whole file that takes \b much
    of time.

    \c max_scan must be large enough to load at least 3 frames. The recommended
    size is about 1Mb. It is enough to sychronize even when file has a header
    (WAV file for instance) and does not take a much of time to scan.

  \fn bool FileParser::open_probe(const string &filename, const HeaderParser *parser, size_t max_scan = 0)
    \param filename Name of the file to open
    \param parser   Parser to use
    \param max_scan Limit amount of data for synchronization

    The same as open(), but also calls probe() and if it fails, closes the file.

  \fn void FileParser::close()
    Close the file.

  \fn bool FileParser::probe()
    Tries to synchronize and determine the file format. It does it at the
    current position, so you can seek to a certain file position before probing.
    Returns true when synchronization was successful and false otherwise.

    Amount of data scanned during probing does not exceed \c max_scan specified
    at open().

    File format is available with get_output() after successful probing.

  \fn bool FileParser::stats(vtime_t precision = 0.5, unsigned min_measurements = 10, unsigned max_measurements = 100)
    \param min_measurements Minimum number of measurements
    \param max_measurements Maximum number of measurements
    \param precision Precision of the file duration in secs.

    Determine the file duration. It is required to navigate the file using
    time and frame units. Average bitrate and frame interval are found too.

    It does number of probes at different file positions. An average is found
    and file duration is determined.

    Minimum number of probes is limited with \c min_measurements. Constant
    bitrate file does not require more measurements because the duration may
    be determined exactly. VBR file may require more probes. Measurements are
    done until we determine the file duration with the required precision or
    we reach max_measurements.

    If precision is zero, min_measurements are always done.

    Returns true on success and false otherwise.

  \fn bool FileParser::is_open() const
    Returns true when file is open and false otherwise.

  \fn bool FileParser::eof() const
    Returns true when we reach the end of the file and false otherwise.

  \fn string FileParser::get_filename() const
    Returns the name of the file.

  \fn const HeaderParser *FileParser::get_parser() const
    Returns header parser used.

  \name Positioning

  \fn fsize_t FileParser::get_pos() const
    Returns current file position in bytes.

  \fn double FileParser::get_pos(units_t units) const
    \param units Units

    Returns current file position in the units specified.

    To use FileParser::frames and FileParser::time units, stat() must be
    called before.

  \fn fsize_t FileParser::get_size() const
    Returns the file size in bytes.

  \fn double FileParser::get_size(units_t units) const
    \param units Units

    Returns the file size in the units specified.
  
    To use FileParser::frames and FileParser::time units, stat() must be
    called before.

  \fn int FileParser::seek(fsize_t pos)
    \param pos Position in bytes.

    Moves the current file position to the position \c pos.

  \fn int FileParser::seek(double pos, units_t units)
    \param pos Position
    \param units Units

    Moves the current file position to the position \c pos in units specified.

    To use FileParser::frames and FileParser::time units, stat() must be
    called before.

  \name Info

  \fn int FileParser::get_frames() const
    Returns number of frames loaded since construction.

  \fn HeaderInfo FileParser::header_info() const
    Returns current frame's info.

  \fn size_t FileParser::get_frame_interval() const
    Returns current frame interval.

  \fn double FileParser::get_avg_frame_interval() const
    Returns average frame interval. stats() must be called before.

  \fn double FileParser::get_avg_bitrate() const
    Returns average bitrate interval. stats() must be called before.

  \fn string FileParser::stream_info() const
    Prints stream information.

  \fn string FileParser::file_info() const;
    Prints file information.

******************************************************************************/


class FileParser : public Source
{
protected:
  StreamBuffer stream;

  AutoFile f;                //!< File we operate on
  string filename;           //!< File name

  bool has_probe;            //!< probe() was done
  bool is_new_stream;        //!< new_stream flag

  Rawdata buf;               //!< Data buffer
  uint8_t *buf_pos;          //!< Current buffer position pointer
  uint8_t *buf_end;          //!< End of buffer data pointer

  size_t stat_size;          //!< Number of measurments done by stat() call
  double avg_frame_interval; //!< Average frame interval
  double avg_bitrate;        //!< Average bitrate

  bool load_frame();
  void stream_reset();

public:
  typedef AutoFile::fsize_t fsize_t;
  size_t max_scan;

  enum units_t { bytes, relative, frames, time };
  inline double units_factor(units_t units) const;

  FileParser();
  ~FileParser();

  /////////////////////////////////////////////////////////////////////////////
  // File operations

  bool open(const string &filename, const HeaderParser *parser, size_t max_scan = 0);
  bool open_probe(const string &filename, const HeaderParser *parser, size_t max_scan = 0);
  void close();

  bool probe();
  bool stats(vtime_t precision = 0.5, unsigned min_measurements = 10, unsigned max_measurements = 100);

  bool is_open() const { return f != 0; }
  bool eof() const { return f.eof() && (buf_pos >= buf_end) && !stream.has_frame(); }

  const string get_filename() const { return filename; }
  const HeaderParser *get_parser() const { return stream.get_parser(); }

  /////////////////////////////////////////////////////////////////////////////
  // Positioning

  fsize_t get_pos() const;
  double  get_pos(units_t units) const;

  fsize_t get_size() const;
  double  get_size(units_t units) const;
  
  int     seek(fsize_t pos);
  int     seek(double pos, units_t units);

  /////////////////////////////////////////////////////////////////////////////
  // Info

  int        get_frames()  const { return stream.get_frames();  }
  HeaderInfo header_info() const { return stream.header_info(); }
  size_t     get_frame_interval() const { return stream.get_frame_interval(); }

  double     get_avg_frame_interval() const { return avg_frame_interval; }
  double     get_avg_bitrate() const { return avg_bitrate; }

  string     stream_info() const { return stream.stream_info(); }
  string     file_info()   const;

  /////////////////////////////////////////////////////////////////////////////
  // Source interface

  virtual void reset();
  virtual bool get_chunk(Chunk &out);
  virtual bool new_stream() const;
  virtual Speakers get_output() const;
};

#endif
