#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include "parsers\ac3\ac3_parser.h"
#include "parsers\ac3\ac3_enc.h"

#include "filters\convert.h"
#include "filter_graph.h"

#include "sink\sink_raw.h"
#include "win32\cpu.h"
#include "auto_file.h"

int main(int argc, char *argv[])
{
  if (argc < 3)
  {
    printf("Please, specify input and output file names\n");
    return 1;
  }

  AutoFile f(argv[1]);
  if (!f.is_open())
  {
    printf("Cannot open file %s for reading\n", argv[1]);
    return 1;
  }

  const int buf_size = 64000;
  uint8_t buf[buf_size];
  int buf_data;

  Converter   conv(2048);
  AC3Enc      enc;
  AC3Parser   dec;
  RAWSink     sink;

  FilterChain chain;
  chain.add_back(&conv, "Converter");
  chain.add_back(&enc,  "Encoder");

  conv.set_buffer(AC3_FRAME_SAMPLES);
  conv.set_format(FORMAT_LINEAR);
  conv.set_order(win_order);

  if (!enc.set_bitrate(448000) ||
      !enc.set_input(Speakers(FORMAT_LINEAR, MODE_STEREO, 48000, 32768)))
  {
    printf("Cannot init encoder!\n");
    return 1;
  }

  if (!sink.open(argv[2]))
  {
    printf("Cannot open file %s for writing!\n", argv[2]);
    return 1;
  }

  Chunk raw_chunk;
  Chunk ac3_chunk;

  CPUMeter cpu_usage;
  CPUMeter cpu_total;
  
  double ms = 0;
  double old_ms = 0;


  cpu_usage.start();
  cpu_total.start();

  fprintf(stderr, "0.0%% Frs/err: 0/0\tTime: 0:00.000i\tFPS: 0 CPU: 0%%\r"); 
  int frames = 0;
  while (!f.eof())
  {
    buf_data = f.read(buf, buf_size);

    raw_chunk.set_rawdata(Speakers(FORMAT_PCM16, MODE_STEREO, 48000, 32768), buf, buf_data);

    if (!chain.process(&raw_chunk))
    {
      printf("Load data error!\n");
      return 1;
    }

    while (!chain.is_empty())
    {
      if (!chain.get_chunk(&ac3_chunk))
      {
        printf("Encoding error!\n");
        return 1;
      }

      sink.process(&ac3_chunk);
      frames++;

      /////////////////////////////////////////////////////
      // Statistics

      ms = double(cpu_total.get_system_time() / 10000);
      if (ms > old_ms + 100)
      {
        old_ms = ms;

        // Statistics
        fprintf(stderr, "%2.1f%% Frames: %i\tTime: %i:%02i.%03i\tFPS: %i CPU: %.1f%%  \r", 
          double(f.pos()) * 100.0 / f.size(), 
          frames,
          int(ms/60000), int(ms) % 60000/1000, int(ms) % 1000,
          int(frames * 1000 / (ms+1)),
          cpu_usage.usage() * 100);
      } // if (ms > old_ms + 100)
    }
  }

  fprintf(stderr, "%2.1f%% Frames: %i\tTime: %i:%02i.%03i\tFPS: %i CPU: %.1f%%  \n", 
    double(f.pos()) * 100.0 / f.size(), 
    frames,
    int(ms/60000), int(ms) % 60000/1000, int(ms) % 1000,
    int(frames * 1000 / (ms+1)),
    cpu_usage.usage() * 100);

  cpu_usage.stop();
  cpu_total.stop();

  printf("System time: %ims\n", int(cpu_total.get_system_time() / 10000));
  printf("Process time: %ims\n", int(cpu_total.get_thread_time() / 10000));

  return 0;
}
