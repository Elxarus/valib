/*
  Stream buffer test
  Uses samples\test files

  Tests:
  * Passthrough test
  * Noise speed test
  * File speed test

  All tests use MultiHeader with MPA, AC3 and DTS parsers.

  Passthrough test
  ================

  This test ensures that we load frames correctly and stream constructed back
  of frames loaded and debris is eqal to the original stream.

  Also this test checks correct frame loading after inplace frame processing.
  To simulate such processing we zap the frame buffer after comparing.

  We also count and check number of streams and frames in a file.
 
  Notes
  -----
  * we load the whole file into memory
  * SPDIF parser uses scanning
  * DTS parser has unknown frame size
  * DTS parser can work with both raw DTS and SPDIF/DTS, but cannot load the
    last frame of SPDIF/DTS stream (see notes for StreamBuffer class)

  Noise speed test
  ================

  Measure the speed of syncronization scanning.

  File speed test
  ================

  Measure the speed of stream walk for different file formats.

*/


#include "log.h"
#include "auto_file.h"
#include "filter_tester.h"

#include "source\noise.h"
#include "source\raw_source.h"
#include "win32\cpu.h"

#include "parsers\ac3\ac3_header.h"
#include "parsers\dts\dts_header.h"
#include "parsers\mpa\mpa_header.h"
#include "parsers\spdif_header.h"
#include "parsers\multi_header.h"

///////////////////////////////////////////////////////////////////////////////
// Test constants

static const vtime_t time_per_test = 1.0; // 1 sec for each speed test
static const int noise_size = 10000000;   // noise speed test buffer size

///////////////////////////////////////////////////////////////////////////////
// Test class

class StreamBuffer_test
{
protected:
  StreamBuffer streambuf;
  Log *log;

public:
  StreamBuffer_test(Log *_log)
  {
    log = _log;
  }

  int test()
  {
    const HeaderParser *headers[] = { &spdif_header, &ac3_header, &mpa_header, &dts_header };
    MultiHeader multi_header(headers, array_size(headers));

    log->open_group("StreamBuffer test");

    log->open_group("Passthrough test");

    log->msg("MPAHeader");
    passthrough("a.mp2.002.mp2",   &mpa_header, 1, 500);
    passthrough("a.mp2.005.mp2",   &mpa_header, 1, 500);
    passthrough("a.mp2.mix.mp2",   &mpa_header, 3, 1500);
                                   
    log->msg("AC3Header");
    passthrough("a.ac3.005.ac3",   &ac3_header, 1, 375);
    passthrough("a.ac3.03f.ac3",   &ac3_header, 1, 375);
    passthrough("a.ac3.mix.ac3",   &ac3_header, 3, 1500);
                                   
    // We cannot load the last frame of SPDIF/DTS stream.
    // See note at StreamBuffer class comments.
    log->msg("DTSHeader");
    passthrough("a.dts.03f.dts",   &dts_header, 1, 1125);
    passthrough("a.dts.03f.spdif", &dts_header, 1, 1124);
                                   
    // SPDIFHeader must work with SPDIF/DTS stream correctly
    log->msg("SPDIFHeader");
    passthrough("a.mp2.005.spdif", &spdif_header, 1, 500);
    passthrough("a.ac3.03f.spdif", &spdif_header, 1, 375);
    passthrough("a.dts.03f.spdif", &spdif_header, 1, 1125);
    passthrough("a.mad.mix.spdif", &spdif_header, 7, 4375);
                                   
    log->msg("MultiHeader");
    passthrough("a.mad.mix.mad",   &multi_header, 7, 4375);
    passthrough("a.mad.mix.spdif", &multi_header, 7, 4375);

    log->close_group();

    log->open_group("Speed test");

    speed_noise(&dts_header);

    log->msg("MPAHeader");
    speed_file("a.mp2.002.mp2",   &mpa_header);
    speed_file("a.mp2.005.mp2",   &mpa_header);
    speed_file("a.mp2.mix.mp2",   &mpa_header);

    log->msg("AC3Header");
    speed_file("a.ac3.005.ac3",   &ac3_header);
    speed_file("a.ac3.03f.ac3",   &ac3_header);
    speed_file("a.ac3.mix.ac3",   &ac3_header);

    log->msg("DTSHeader");
    speed_file("a.dts.03f.dts",   &dts_header);
    speed_file("a.dts.03f.spdif", &dts_header);

    log->msg("SPDIFHeader");
    speed_file("a.mp2.005.spdif", &spdif_header);
    speed_file("a.ac3.03f.spdif", &spdif_header);
    speed_file("a.dts.03f.spdif", &spdif_header);
    speed_file("a.mad.mix.spdif", &spdif_header);

    log->msg("MultiHeader");
    speed_file("a.mad.mix.mad",   &multi_header);
    speed_file("a.mad.mix.spdif", &multi_header);

    log->close_group();
    return log->close_group();
  }

  int passthrough(const char *filename, const HeaderParser *hparser, int file_streams, int file_frames)
  {
    AutoFile f(filename);
    if (!f.is_open())
      return log->err("Cannot open file %s", filename);

    log->msg("Passthrough test %s", filename);

    // load file into memory
    size_t buf_size = f.size();
    uint8_t *buf = new uint8_t[buf_size];
    f.read(buf, buf_size);

    // setup pointers
    uint8_t *ptr = buf;
    uint8_t *end = ptr + buf_size;
    uint8_t *ref_ptr = buf;

    uint8_t *frame;
    size_t frame_size;
    uint8_t *debris;
    size_t debris_size;

    // setup cycle
    size_t i;
    int frames = 0;
    int streams = 0;
    bool data_differ = false;

    streambuf.set_parser(hparser);
    while (ptr < end)
    {
      // process data
      streambuf.load(&ptr, end);

      // count frames and streams
      if (streambuf.is_new_stream())   streams++;
      if (streambuf.is_frame_loaded()) frames++;

      // get data
      debris      = streambuf.get_debris();
      debris_size = streambuf.get_debris_size();
      frame       = streambuf.get_frame();
      frame_size  = streambuf.get_frame_size();

      // check bounds
      if (ref_ptr + debris_size + frame_size > end)
      {
        log->err("Frame ends after the end of the reference file");
        break; // while (streambuf.load_frame(&ptr, end))
      }

      // compare debris
      for (i = 0; i < debris_size; i++)
        if (ref_ptr[i] != debris[i])
        {
          data_differ = true;
          log->err("Debris differ at stream/frame %i/%i, pos %i", streams, frames, ref_ptr - buf + i);
          break;
        }

      ref_ptr += debris_size;
      if (data_differ)
        break;

      // compare frame data
      for (i = 0; i < frame_size; i++)
        if (ref_ptr[i] != frame[i])
        {
          data_differ = true;
          log->err("Frame data differ at stream/frame %i/%i, pos %i", streams, frames, ref_ptr - buf + i);
          break;
        }

      ref_ptr += frame_size;
      if (data_differ)
        break;

      // zap the frame buffer to simulate in-place processing
      memset(frame, 0, frame_size);
    }

    if (!data_differ && file_streams > 0 && streams != file_streams)
      log->err("Wrong number of streams; found: %i, but must be %i", streams, file_streams);

    if (!data_differ && file_frames > 0 && frames != file_frames)
      log->err("Wrong number of frames; found: %i, but must be %i", frames, file_frames);

    delete buf;
    return log->get_errors();
  }

  int speed_noise(const HeaderParser *hparser)
  {
    streambuf.set_parser(hparser);

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
      while (ptr < end)
      {
        streambuf.load(&ptr, end);
        if (streambuf.is_in_sync())
          return log->err("Syncronized on noise!");
      }
    }
    cpu.stop();

    log->msg("StreamBuffer speed on noise: %iMB/s", 
      int(double(chunk.size) * runs / cpu.get_thread_time() / 1000000));

    return 0;
  }

  int speed_file(const char *filename, const HeaderParser *hparser)
  {
    CPUMeter cpu;
    AutoFile f(filename);
    if (!f.is_open())
      return log->err("Cannot open file %s", filename);

    streambuf.set_parser(hparser);

    size_t buf_size = f.size();
    uint8_t *buf = new uint8_t[buf_size];
    f.read(buf, buf_size);

    int runs = 0;

    cpu.reset();
    cpu.start();
    while (cpu.get_thread_time() < time_per_test)
    {
      runs++;

      // Process the whole file
      uint8_t *ptr = buf;
      uint8_t *end = ptr + buf_size;
      streambuf.reset();
      while (ptr < end)
        streambuf.load(&ptr, end);
    }
    cpu.stop();

    log->msg("StreamBuffer speed on %s: %iMB/s", 
      filename, int(double(buf_size) * runs / cpu.get_thread_time() / 1000000));

    delete buf;
    return log->get_errors();
  }
};


///////////////////////////////////////////////////////////////////////////////
// Test function

int test_streambuffer(Log *log)
{
  StreamBuffer_test test(log);
  return test.test();
}
