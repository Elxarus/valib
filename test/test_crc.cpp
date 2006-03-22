/*
  Test CRC class
*/

#include "log.h"
#include "crc.h"
#include "source\noise.h"
#include "win32\cpu.h"


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
    bytestream();
    //test_bitstream()
    speed_test();
    return log->close_group();
  }

  int bytestream()
  {
    log->msg("Byte stream test");
    return 0;
  }

  void speed_test()
  {
    const int runs = 1;
    const int size = 10000000;
    const int crc_test = 0x7589;
    uint32_t crc;

    Chunk chunk;
    Noise noise(spk_unknown, size, size);
    noise.set_seed(47564321);
    noise.get_chunk(&chunk);

    CPUMeter cpu;
    CRC crc_calc;
    crc_calc.init(POLY_CRC16, 16);
    cpu.start();
    for (int i = 0; i < runs; i++)
    {
//      crc = calc_crc(0, chunk.rawdata, chunk.size);
      crc = crc_calc.bytestream(0, chunk.rawdata, chunk.size);
      crc >>= 16;
    }
    cpu.stop();

    if (crc != crc_test)
      log->err("crc = 0x%x but must be 0x%x", crc, crc_test);

    log->msg("CRC calc speed: %iMB/s", 
      int(double(chunk.size) * runs / cpu.get_thread_time() / 1000000));

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
