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
  of loaded frames is eqal to the original stream. 

  We also count and check number of streams and frames in a file.
 
  Notes
  -----
  * we load the whole file into memory
  * load_frame() must either load a frame or process all data given.
  * SPDIF file is added and have SPDIF headers so we should skip all this.

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
    const HeaderParser *headers[] = { &ac3_header, &mpa_header, &dts_header };
    MultiHeader multi_header(headers, array_size(headers));

    log->open_group("StreamBuffer test");

    passthrough("a.mp2.002.mp2",   &mpa_header, 1, 500);
    passthrough("a.mp2.005.mp2",   &mpa_header, 1, 500);
    passthrough("a.mp2.mix.mp2",   &mpa_header, 3, 1500);
    passthrough("a.mp2.005.spdif", &mpa_header, 1, 500, true);
                                   
    passthrough("a.ac3.005.ac3",   &ac3_header, 1, 375);
    passthrough("a.ac3.03f.ac3",   &ac3_header, 1, 375);
    passthrough("a.ac3.mix.ac3",   &ac3_header, 3, 1500);
    passthrough("a.ac3.03f.spdif", &ac3_header, 1, 375, true);
                                   
    // We cannot load the last frame of SPDIF/DTS stream.
    // See note at StreamBuffer class comments...
    passthrough("a.dts.03f.dts",   &dts_header, 1, 1125);
    passthrough("a.dts.03f.spdif", &dts_header, 1, 1124, true);
                                   
    passthrough("a.mad.mix.mad",   &multi_header, 7, 4375);
    passthrough("a.mad.mix.spdif", &multi_header, 7, 4375, true);

    speed_noise(&ac3_header);

    speed_file("a.mp2.002.mp2",   &mpa_header);
    speed_file("a.mp2.005.mp2",   &mpa_header);
    speed_file("a.mp2.mix.mp2",   &mpa_header);
    speed_file("a.mp2.005.spdif", &mpa_header);

    speed_file("a.ac3.005.ac3",   &ac3_header);
    speed_file("a.ac3.03f.ac3",   &ac3_header);
    speed_file("a.ac3.mix.ac3",   &ac3_header);
    speed_file("a.ac3.03f.spdif", &ac3_header);

    speed_file("a.dts.03f.dts",   &dts_header);
    speed_file("a.dts.03f.spdif", &dts_header);

    speed_file("a.mad.mix.mad",   &multi_header);
    speed_file("a.mad.mix.spdif", &multi_header);

    return log->close_group();
  }

  int passthrough(const char *filename, const HeaderParser *hparser, int file_streams, int file_frames, bool is_spdif = false)
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

    // setup cycle
    uint8_t *frame;
    size_t frame_size;

    int frames = 0;
    int streams = 0;
    bool data_differ = false;

    streambuf.set_parser(hparser);
    while (streambuf.load_frame(&ptr, end))
    {
      // count frames and streams
      frames++;
      if (streambuf.is_new_stream())
        streams++;

      // compare data
      frame = streambuf.get_frame();
      frame_size = streambuf.get_frame_size();

      if (ref_ptr + frame_size > end)
      {
        log->err("Frame ends after the end of reference file");
        break; // while (streambuf.load_frame(&ptr, end))
      }
      else
      {
        if (is_spdif)
        {
          // Spdif stream has a header and padding.
          // Therefore we must seek a frame start to compare data.
          static const size_t max_spdif_frame_size = 4096 * 4;
          size_t shift, i;
          data_differ = true;

          for (shift = 0; shift < (max_spdif_frame_size - frame_size); shift++)
          {
            for (i = 0; i < frame_size; i++)
              if (ref_ptr[shift + i] != frame[i])
                break; // for (i = 0; i < frame_size; i++)

            if (i == frame_size)
            {
              data_differ = false;
              break; // for (size_t shift = 0; shift < max_spdif_frame_size - frame_size; shift++)
            }
          }

          if (data_differ)
            log->err("Cannot find frame data at frame %i, starting from pos %i", frames, ref_ptr - buf);

          ref_ptr += frame_size + shift;
        }
        else
        {
          // Compare data directly
          data_differ = false;
          for (size_t i = 0; i < frame_size; i++)
            if (ref_ptr[i] != frame[i])
            {
              data_differ = true;
              log->err("Data differ at frame %i, pos %i", frames, ref_ptr - buf + i);
              break;
            }
          ref_ptr += frame_size;
        }

        if (data_differ)
          break;
      }       
    }

    if (!data_differ && ptr < end)
      log->err("Frame load error");

    if (!data_differ && file_frames > 0 && frames != file_frames)
      log->err("Wrong number of frames; found: %i, but must be %i", frames, file_frames);

    if (!data_differ && file_streams > 0 && streams != file_streams)
      log->err("Wrong number of streams; found: %i, but must be %i", streams, file_streams);

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
      if (streambuf.load_frame(&ptr, end))
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
        streambuf.load_frame(&ptr, end);
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
