#include "log.h"
#include "auto_file.h"
#include "filter_tester.h"

#include "source\noise.h"
#include "source\raw_source.h"
#include "win32\cpu.h"

#include "parsers\ac3\ac3_header.h"
//#include "parsers\dts\dts_header.h"
//#include "parsers\mpa\mpa_header.h"

///////////////////////////////////////////////////////////////////////////////
// Test constants

static const vtime_t time_per_test = 1.0; // 1 sec for each speed test
static const int noise_size = 10000000;   // noise speed test buffer size

///////////////////////////////////////////////////////////////////////////////
// Test class

class StreamBuffer_test
{
protected:
  StreamBuffer syncbuf;
  Log *log;

public:
  StreamBuffer_test(Log *_log, const HeaderParser *_hparser)
  {
    log = _log;
    syncbuf.set_hparser(_hparser);
  }

  int test()
  {
    log->open_group("StreamBuffer test");
    speed_noise();
    speed_file("a.ac3.03f.ac3", 1, 375);
    speed_file("a.ac3.03f.spdif", 1, 375);
    speed_file("a.ac3.mix.ac3", 3, 1500);
    speed_file("a.ac3.mix.spdif", 3, 1500);
    return log->close_group();
  }

  int speed_noise()
  {
    CPUMeter cpu;
    Chunk chunk;
    Noise noise(spk_unknown, noise_size, noise_size);
    noise.get_chunk(&chunk);

    cpu.reset();
    cpu.start();
    int runs = 0;
    while (cpu.get_thread_time() < time_per_test)
    {
      runs++;

      // Try to load a frame
      uint8_t *ptr = chunk.rawdata;
      uint8_t *end = ptr + chunk.size;
      if (syncbuf.load_frame(&ptr, end))
        return log->err("Syncronized on noise!");

      // Ensure that all data was gone
      if (ptr < end)
        return log->err("Some data was not processed!");
    }
    cpu.stop();

    log->msg("StreamBuffer speed on noise: %iMB/s", 
      int(double(chunk.size) * runs / cpu.get_thread_time() / 1000000));

    return 0;
  }

  int speed_file(const char *filename, int file_streams, int file_frames)
  {
    CPUMeter cpu;
    AutoFile f(filename);
    if (!f.is_open())
      return log->err("Cannot open file %s", filename);

    size_t buf_size = f.size();
    uint8_t *buf = new uint8_t[buf_size];
    f.read(buf, buf_size);

    int runs = 0;
    int frames = 0;
    int streams = 0;

    cpu.reset();
    cpu.start();
    while (cpu.get_thread_time() < time_per_test)
    {
      runs++;

      // Process the whole file
      uint8_t *ptr = buf;
      uint8_t *end = ptr + buf_size;
      syncbuf.reset();
      while (1)
      {
        if (syncbuf.load_frame(&ptr, end))
        {
          frames++;
          if (syncbuf.is_new_stream())
            streams++;
        }
        else if (ptr >= end)
          break;
      }
    }
    cpu.stop();
    frames /= runs;
    streams /= runs;

    log->msg("StreamBuffer speed on %s: %iMB/s, Streams: %i, Frames : %i", 
      filename, int(double(buf_size) * runs / cpu.get_thread_time() / 1000000), streams, frames);

    if (file_frames > 0 && frames != file_frames)
      log->err("Wrong number of frames; found: %i, but must be %i", frames, file_frames);

    if (file_streams > 0 && streams != file_streams)
      log->err("Wrong number of streams; found: %i, but must be %i", streams, file_streams);

    return log->get_errors();
  }
};


///////////////////////////////////////////////////////////////////////////////
// Test function

int test_streambuffer(Log *log)
{
  StreamBuffer_test test(log, &ac3_header);
  return test.test();
}
