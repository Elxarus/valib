/*
  SPDIF passthrough test
  Compare output of AudioDecoder with reference file
*/

#include <stdio.h>
#include "filters\spdifer.h"
#include "auto_file.h"

int test_spdifer(const char *fn, const char *fn_spdif, Speakers spk)
{
  printf("Testing file %s, compare output with %s\n", fn, fn_spdif);
  AutoFile f(fn);
  AutoFile fspdif(fn_spdif);
  Spdifer spdifer;

  if (!f.is_open())
  {
    printf("!!!Error: Cannot open file %s\n", fn);
    return 1;
  }
  if (!fspdif.is_open())
  {
    printf("!!!Error: Cannot open file %s\n", fn_spdif);
    return 1;
  }

  const int buf_size = 1024;
  uint8_t buf[buf_size];
  int buf_data;

  const int spdif_size = 2048 * 4; // max spdif frame size
  uint8_t spdif_buf[spdif_size];
  int spdif_data;

  Chunk input;
  Chunk output;

  int spdif_data_size = 0;
  while (!f.eof())
  {
    buf_data = f.read(buf, buf_size);

    input.set_spk(spk);
    input.set_buf(buf, buf_data);
    input.set_time(false);

    if (!spdifer.process(&input))
    {
      printf("!!!Error: error in process() call\n");
      return 1;
    }

    while (!spdifer.is_empty())
    {
      if (!spdifer.get_chunk(&output))
      {
        printf("!!!Error: error in get_chunk() call\n");
        return 1;
      }

      // implicit assumption: output.size < max_spdif_frame_size
      if (!output.is_empty())
      {
        spdif_data_size += output.size;
        spdif_data = fspdif.read(spdif_buf, output.size);
        if (spdif_data != output.size)
        {
          printf("!!!Error: stream length does not match file size (more data returned)\n");
          return 1;
        }
        if (memcmp(spdif_buf, output.buf, output.size))
        {
          printf("!!!Error: data difference!\n");
          return 1;
        }
      } // if (!output.is_empty())
    } // while (!dec.is_empty())
  } // while (!f.eof())

  if (spdif_data_size != fspdif.size())
  {
    printf("!!!Error: stream length does not match file size (less data returned)\n");
    return 1;
  }

  return 0;
}
