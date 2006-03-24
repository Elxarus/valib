/*
  Test CRC class

  * Test CRC of large noise buffer with different bitstreams
  * Test CRC speed of different bitstreams
  * Test parsers' CRC check with different parsers and bitstreams
  * Test error detection coverage with different parsers
  * Determine CRC overhead in parsers
  * Test speed of simple CRC16 table algorithm
*/

#include "log.h"
#include "crc.h"
#include "source\noise.h"
#include "source\raw_source.h"
#include "win32\cpu.h"

#include "parsers\ac3\ac3_parser.h"
#include "parsers\dts\dts_parser.h"
#include "parsers\mpa\mpa_parser.h"


static const vtime_t time_per_test = 1.0; // 1 sec for each speed test
static const int size = 10000000;         // use 10MB noise buffer
static const int err_dist = 12689;        // generate error each N bytes

///////////////////////////////////////////////////////////////////////////////
// Test class
///////////////////////////////////////////////////////////////////////////////

class CRCTest
{
protected:
  CRC crc;
  Log *log;

public:
  CRCTest(Log *_log)
  {
    log = _log;
  }

  int test()
  {
    log->open_group("CRC test");
    speed_test();
    parser_test();
    return log->close_group();
  }

  void parser_test()
  {
    MPAParser mpa;
    AC3Parser ac3;
    DTSParser dts;

    parser_test(&mpa, 500, "test.mp2");
    parser_test(&mpa, 500, "test.mp2.spdif");
    parser_test(&ac3, 375, "test.ac3");
    parser_test(&ac3, 375, "test.ac3.spdif");
    parser_test(&dts, 1125, "test.dts");
    parser_test(&dts, 1125, "test.dts.spdif");
  }

  void speed_test()
  {
    Chunk chunk;
    Noise noise(spk_unknown, size, size);
    noise.set_seed(47564321);
    noise.get_chunk(&chunk);

    crc.init(POLY_CRC16, 16);
    speed_test_table(chunk.rawdata, chunk.size, 0x7589);
    speed_test(BITSTREAM_8,    "byte stream", chunk.rawdata, chunk.size, 0x75890000);
    speed_test(BITSTREAM_14BE, "14bit BE",    chunk.rawdata, chunk.size, 0xaf9b0000);
    speed_test(BITSTREAM_14LE, "14bit LE",    chunk.rawdata, chunk.size, 0xba690000);
    speed_test(BITSTREAM_16BE, "16bit BE",    chunk.rawdata, chunk.size, 0x75890000);
    speed_test(BITSTREAM_16LE, "16bit LE",    chunk.rawdata, chunk.size, 0x826f0000);
    speed_test(BITSTREAM_32BE, "32bit BE",    chunk.rawdata, chunk.size, 0x75890000);
    speed_test(BITSTREAM_32LE, "32bit LE",    chunk.rawdata, chunk.size, 0x00470000);
  }

  int parser_test(BaseParser *parser, int frames, const char *filename)
  {
    int runs;
    int frame_count;
    int broken_frames;
    float speed_nocrc;
    float speed_crc;
    float speed_broken;

    uint8_t *ptr;
    uint8_t *end;

    Chunk chunk;
    RAWSource f;
    CPUMeter cpu;

    if (!f.open(spk_unknown, filename))
      return log->err("Cannot open file %s", filename);
    else
      log->msg("Scanning file %s (%i frames):", filename, frames);

    ///////////////////////////////////////////////////////
    // Load all frames without CRC check

    runs = 0;
    frame_count = 0;
    parser->do_crc = false;
    parser->reset();
    cpu.reset();
    cpu.start();
    while (cpu.get_thread_time() < time_per_test)
    {
      runs++;

      if (!f.open(spk_unknown, filename))
        return log->err("Cannot open file %s", filename);

      while (!f.is_empty()) 
      {
        f.get_chunk(&chunk);
        ptr = chunk.rawdata;
        end = chunk.rawdata + chunk.size;
        while (ptr < end)
          if (parser->load_frame(&ptr, end))
//            if (parser->decode_frame())
              frame_count++;
      }
    }
    cpu.stop();
    speed_nocrc = (float)(f.size() * runs / cpu.get_thread_time() / 1000000);
    frame_count /= runs;

    if (frame_count != frames)
      return log->err("Frames found without CRC check: %i (must be %i)", frame_count, frames);


    ///////////////////////////////////////////////////////
    // Load all frames with CRC check

    runs = 0;
    frame_count = 0;
    parser->do_crc = true;
    parser->reset();
    cpu.reset();
    cpu.start();
    while (cpu.get_thread_time() < time_per_test)
    {
      runs++;

      if (!f.open(spk_unknown, filename))
        return log->err("Cannot open file %s", filename);

      while (!f.is_empty()) 
      {
        f.get_chunk(&chunk);
        ptr = chunk.rawdata;
        end = chunk.rawdata + chunk.size;
        while (ptr < end)
          if (parser->load_frame(&ptr, end))
//            if (parser->decode_frame())
              frame_count++;
      }
    }
    cpu.stop();
    speed_crc = (float)(f.size() * runs / cpu.get_thread_time() / 1000000);
    frame_count /= runs;

    if (frame_count != frames)
      return log->err("Frames found with CRC check: %i (must be %i)", frame_count, frames);


    ///////////////////////////////////////////////////////
    // Break some frames
    // Do not change 0 value because 0 is used at spdif -
    // wrapped stream as padding and not participates at 
    // crc check.

    runs = 0;
    frame_count = 0;
    broken_frames = 0;
    parser->do_crc = true;
    parser->reset();
    cpu.reset();
    cpu.start();
    while (cpu.get_thread_time() < time_per_test)
    {
      runs++;

      if (!f.open(spk_unknown, filename))
        return log->err("Cannot open file %s", filename);

      while (!f.is_empty()) 
      {
        f.get_chunk(&chunk);
        ptr = chunk.rawdata;
        end = chunk.rawdata + chunk.size;

        for (size_t i = err_dist; i < chunk.size; i += err_dist)
          if (ptr[i] != 0)
          {
            ptr[i] = 0;
            broken_frames++;
          }

        while (ptr < end)
        {
          if (parser->load_frame(&ptr, end))
//            if (parser->decode_frame())
              frame_count++;
        }
      }
    }
    cpu.stop();
    speed_broken = (float)(f.size() * runs / cpu.get_thread_time() / 1000000);
    frame_count /= runs;
    broken_frames /= runs;

    if (broken_frames)
      log->msg("Broken frames: %i Detected: %i (%i%% detected)", 
        broken_frames, frames - frame_count, (frames - frame_count) * 100 / broken_frames);
    else
      log->err("No broken frames produced");

    log->msg("No CRC: %.0fMB/s CRC: %.0fMB/s (%.1f times speed diff)", 
      speed_nocrc, speed_crc, speed_nocrc / speed_crc);

    return log->get_errors();
  }

  int speed_test_table(uint8_t *data, size_t size, uint32_t crc_test)
  {
    // table method speed test
    uint32_t result;
    CPUMeter cpu;

    int runs = 0;
    cpu.start();
    while (cpu.get_thread_time() < time_per_test)
    {
      runs++;
      result = calc_crc(0, data, size);
    }
    cpu.stop();

    log->msg("CRC speed (table method): %iMB/s",
      int(double(size) * runs / cpu.get_thread_time() / 1000000));

    if (result != crc_test)
      log->err("crc = 0x%08x but must be 0x%08x", result, crc_test);

    return log->get_errors();
  }
  int speed_test(int bs_type, const char *bs_text, uint8_t *data, size_t size, uint32_t crc_test)
  {
    uint32_t result;
    CPUMeter cpu;

    int runs = 0;
    cpu.start();
    while (cpu.get_thread_time() < time_per_test)
    {
      runs++;
      result = crc.calc(0, data, size, bs_type);
    }
    cpu.stop();

    log->msg("CRC %s speed: %iMB/s", bs_text,
      int(double(size) * runs / cpu.get_thread_time() / 1000000));

    if (result != crc_test)
      log->err("crc = 0x%08x but must be 0x%08x", result, crc_test);

    return log->get_errors();
  }

};


///////////////////////////////////////////////////////////////////////////////
// Test function
///////////////////////////////////////////////////////////////////////////////

int test_crc(Log *log)
{
  CRCTest test(log);
  return test.test();
}
