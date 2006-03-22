/*
  Test CRC class
*/

#include "log.h"
#include "crc.h"
#include "source\noise.h"
#include "source\raw_source.h"
#include "win32\cpu.h"

#include "parsers\ac3\ac3_parser.h"
#include "parsers\dts\dts_parser.h"
#include "parsers\mpa\mpa_parser.h"


const int size = 10000000;
const int runs = 20;

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
    parser_test();
    speed_test();
    return log->close_group();
  }

  void parser_test()
  {
    AC3Parser ac3;
    DTSParser dts;
    MPAParser mpa;

    parser_test(&ac3, 375, "test.ac3");
    parser_test(&ac3, 375, "test.ac3.spdif");
  }

  void speed_test()
  {
    Chunk chunk;
    Noise noise(spk_unknown, size, size);
    noise.set_seed(47564321);
    noise.get_chunk(&chunk);

    crc.init(POLY_CRC16, 16);
    speed_test(BITSTREAM_8,    "byte stream", chunk.rawdata, chunk.size, 0x75890000);
    speed_test(BITSTREAM_16LE, "16bit LE",    chunk.rawdata, chunk.size, 0x826f0000);
    speed_test(BITSTREAM_32LE, "32bit LE",    chunk.rawdata, chunk.size, 0x00470000);
  }

  int parser_test(BaseParser *parser, int frames, const char *filename)
  {
    int frame_count;
    int broken_frames;
    uint8_t *ptr;
    uint8_t *end;

    Chunk chunk;
    RAWSource f;

    if (!f.open(spk_unknown, filename))
      return log->err("Cannot open file %s", filename);
    else
      log->msg("Testing file %s", filename);

    parser->do_crc = true;

    ///////////////////////////////////////////////////////
    // Load all frames

    if (!f.open(spk_unknown, filename))
      return log->err("Cannot open file %s", filename);

    parser->reset();
    frame_count = 0;
    while (!f.is_empty()) 
    {
      f.get_chunk(&chunk);
      ptr = chunk.rawdata;
      end = chunk.rawdata + chunk.size;
      while (ptr < end)
      {
        if (parser->load_frame(&ptr, end))
          frame_count++;
      }
    }

    if (frame_count != frames)
      return log->err("Frames found: %i but must be %i");

    ///////////////////////////////////////////////////////
    // Break some frames
    // Change one byte per one buffer loaded.
    // Do not change 0 value because 0 is used at spdif -
    // wrapped stream as padding and not participates at 
    // crc check.

    if (!f.open(spk_unknown, filename))
      return log->err("Cannot open file %s", filename);

    parser->reset();
    frame_count = 0;
    broken_frames = 0;
    while (!f.is_empty()) 
    {
      f.get_chunk(&chunk);
      ptr = chunk.rawdata;
      end = chunk.rawdata + chunk.size;
      if (ptr[chunk.size / 2] != 0)
      {
        ptr[chunk.size / 2] = 0;
        broken_frames++;
      }

      while (ptr < end)
      {
        if (parser->load_frame(&ptr, end))
          frame_count++;
      }
    }

    if (frame_count + broken_frames != frames)
      return log->err("Broken frames: %i, frames found: %i but must be %i", 
        broken_frames, frame_count, frames - broken_frames);

    return log->get_errors();
  }

  int speed_test(int bs_type, const char *bs_text, uint8_t *data, size_t size, uint32_t crc_test)
  {
    uint32_t result;
    CPUMeter cpu;

    cpu.start();
    for (int i = 0; i < runs; i++)
      result = crc.calc(0, data, size, bs_type);
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
