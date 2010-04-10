#ifndef VALIB_SINK_WAV
#define VALIB_SINK_WAV

#include "../sink.h"
#include "../auto_file.h"

class WAVSink : public SimpleSink
{
protected:
  AutoFile f;
  uint32_t header_size;  // WAV header size;
  uint64_t data_size;    // data size written to the file
  uint8_t *file_format;  // WAVEFORMAT *

  void init_riff();
  void close_riff();

public:
  WAVSink();
  WAVSink(const char *file_name);
  ~WAVSink();

  bool open_file(const char *file_name);
  void close_file();
  bool is_file_open() const;

  /////////////////////////////////////////////////////////
  // Sink interface

  virtual bool can_open(Speakers new_spk) const;
  virtual bool init();
  virtual void process(const Chunk2 &chunk); 
};

#endif
