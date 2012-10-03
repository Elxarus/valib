#ifndef VALIB_SINK_WAV
#define VALIB_SINK_WAV

#include "../sink.h"
#include "../auto_file.h"

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
