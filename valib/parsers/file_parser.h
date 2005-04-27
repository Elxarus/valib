/*
  File parser class
*/

#ifndef FILE_PARSER_H
#define FILE_PARSER_H

#include <stdio.h>
#include "parser.h"
#include "mpeg_demux.h"

class FileParser
{
protected:
  FILE *f;
  char *filename;
  int   filesize;

  // file statistics
  int   sample_rate;    // sample rate
  int   frame_size;     // average frame size
  int   frame_samples;  // average frame samples

  unsigned frames_overhead;
  unsigned errors_overhead;

  MPEGDemux demux;
  Parser   *parser;

  uint8_t *buf;
  int buf_size;
  int buf_data;
  int buf_pos;

  bool fill_buf();
  void file_stats();

public:
  int  max_scan;
  bool is_pes;
  enum units_t { bytes, frames, samples, ms, relative };

  FileParser();
  ~FileParser();

  bool open(Parser *parser, const char *filename, unsigned max_scan = 32768);
  void close();

  bool probe();
  void stats(int nframes = 100);

  void set_mpeg_stream(int stream = 0, int substream = 0);

  const char *get_filename() const { return filename; };
  bool is_open() const { return f != 0; }
  bool eof() const { return feof(f) != 0 && !buf_data; }

  // seeking
  void   seek(int pos);
  void   seek(double pos, units_t units);

  // file position & size
  int    get_pos() const;
  double get_pos(units_t units) const;
  int    get_size() const;
  double get_size(units_t units) const;

  double get_bitrate() const;

  /////////////////////////////////////////////////////////
  // Parser-equivalent functions

  virtual void reset();

  // load/decode frame
  inline  bool     frame();
  virtual unsigned load_frame();
  virtual bool     decode_frame()   { return parser->decode_frame(); }

  // Stream information
  virtual Speakers get_spk()        const { return parser->get_spk();        }
  virtual unsigned get_frame_size() const { return parser->get_frame_size(); }
  virtual unsigned get_nsamples()   const { return parser->get_nsamples();   }

  virtual void get_info(char *buf, size_t len) const;
  virtual unsigned get_frames()     const { return parser->get_frames() - frames_overhead; }
  virtual unsigned get_errors()     const { return parser->get_errors() + demux.errors - errors_overhead; }

  // Buffers
  virtual uint8_t *get_frame()      const { return parser->get_frame();     }
  virtual samples_t get_samples()   const { return parser->get_samples();   }
};


inline bool 
FileParser::frame()
{
  if (load_frame())
    if (decode_frame())
      return true;
  return false;
}



#endif
