/*
  RAW File class
  (not working state)
*/

#ifndef FILE_RAW_H
#define FILE_RAW_H

#include <stdio.h>

class RAWFile
{
protected:
  FILE *f;
  char *filename;
  int   filesize;

  // file statistics
  int   sample_rate;    // sample rate
  int   frame_size;     // mean frame size
  int   frame_samples;  // mean frame samples

  int frames_overhead;
  int errors_overhead;

  PESDemux demux;
  Parser  *parser;

  uint8_t *buf;
  int buf_size;
  int buf_data;
  int buf_pos;

  bool fill_buf();
  void file_stats();

public:
  int  max_scan;
  bool inverse;
  bool is_pes;
  enum units_t { bytes, frames, samples, ms, relative };

  FileParser(Parser *parser, const char *filename = 0, int max_scan = 32768);
  ~FileParser();

  bool open(const char *filename);
  void close();

  bool probe();
  void stats(int nframes = 10);

  void set_mpeg_stream(int stream = 0, int substream = 0);

  bool is_open()                { return f != 0; }
  bool eof()                    { return feof(f) != 0; }

  const char *get_filename()    { return filename; };

  void   seek(int pos);
  void   seek(double pos, units_t units);

  int    get_pos();
  double get_pos(units_t units);
  int    get_size();
  double get_size(units_t units);

  // Parser-equivalent functions
  virtual void reset();

  // load/decode frame
  virtual bool frame();
  virtual int  load_frame();
  virtual bool decode_frame()   { return parser->decode_frame();    }

  // stream configuration
  virtual Speakers get_spk()    { return parser->get_spk();         }

  // raw frame deata
  virtual uint8_t *get_frame()  { return parser->get_frame();       }
  virtual int  get_frame_size() { return parser->get_frame_size();  }

  // decoded data
  virtual samples_t get_samples() { return parser->get_samples();   }
  virtual int get_nsamples()      { return parser->get_nsamples();  }

  virtual int get_frames()      { return parser->get_frames() - frames_overhead; }
  virtual int get_errors()      { return parser->get_errors() + demux.errors - errors_overhead; }
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
