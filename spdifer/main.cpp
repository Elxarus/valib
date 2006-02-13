#include <stdio.h>
#include <fcntl.h>
#include <io.h>

#include "filters\spdifer.h"
#include "source\raw_source.h"
#include "sink\sink_raw.h"
#include "vtime.h"

///////////////////////////////////////////////////////////////////////////////
// Main
///////////////////////////////////////////////////////////////////////////////

int main(int argc, const char **argv)
{
  bool stat = true;  // display statistics

  RAWSource src;
  RAWRenderer dst;

  if (argc < 3)
  {
    printf("Spdifer\n"
           "Encapsulates AC3/DTS/MPEG Audio stream into SPDIF according to IEC 61937\n"
           "This utility is part of AC3Filter project (http://ac3filter.sourceforge.net)\n"
           "Copyright (c) 2006 by Alexander Vigovsky\n\n");

    printf("Usage:\n"
           "  Spdifer input_file output_file\n"
           "\n"
           "You may use a fictive file name - to indicate standard input/output.\n"
           "Examples:\n"
           "  > spdifer file.ac3 file.ac3.spdif transform file.ac3 to file.ac3.spdif\n"
           "  > spdifer file.ac3 -              transform file.ac3 to standard output\n"
           "  > spdifer - -                     teansform standard input to standard output\n");
    return 0;
  }

  if (!strcmp(argv[1], "-"))
  {
    _setmode(_fileno(stdin), _O_BINARY);
    src.open(spk_unknown, stdin);
  }
  else
    src.open(spk_unknown, argv[1]);

  if (!src.is_open())
  {
    fprintf(stderr, "Cannot open input file\n");
    return 1;
  }

  if (!strcmp(argv[2], "-"))
  {
    _setmode(_fileno(stdout), _O_BINARY);
    dst.open_file(stdout);
    stat = false;
  }
  else
    dst.open_file(argv[2]);

  if (!dst.is_file_open())
  {
    fprintf(stderr, "Cannot open output file\n");
    return 1;
  }

  Spdifer spdifer;

  Chunk chunk;
  vtime_t start_time = local_time();
  vtime_t old_time = start_time;

  size_t isize = 0;
  size_t osize = 0;
  size_t file_size = src.size();

  while (!src.eof())
  {
    if (!src.get_chunk(&chunk))
    {
      fprintf(stderr, "Cannot read from source\n");
      return 1;
    }

    isize += chunk.size;

    if (!spdifer.process(&chunk))
    {
      fprintf(stderr, "Processing error\n");
      return 1;
    }

    while (!spdifer.is_empty())
    {
      if (!spdifer.get_chunk(&chunk))
      {
        fprintf(stderr, "Processing error\n");
        return 1;
      }
      osize += chunk.size;

      if (!dst.write(&chunk))
      {
        fprintf(stderr, "Cannot write data\n");
        return 1;
      }
    }

    if (src.eof())
    {
      stat = stat;
    }

    if (stat) if (src.eof() || local_time() > old_time + 0.1)
    {
      int n;
      Speakers sync = spdifer.get_sync();
      old_time = local_time();

      float processed = float(isize);
      float elapsed = float(old_time - start_time);
      int eta = int((float(file_size) / processed - 1.0) * elapsed);

      printf("%02i:%02i %02i%% In: %iM (%2iMB/s) Out: %iM  %s %s %i%n", 
        eta / 60, eta % 60,
        int(processed * 100 / float(file_size)),
        isize / 1000000, int(processed/elapsed/1000000), osize / 1000000,
        sync.format_text(), sync.mode_text(), sync.sample_rate, &n);
      printf("%*s\r", 78-n, "");
    }
  }

  return 0;
}
