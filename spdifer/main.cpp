#include <stdio.h>
#include <fcntl.h>
#include <io.h>

#include "filters\spdifer.h"
#include "source\raw_source.h"
#include "sink\sink_raw.h"

int main()
{
  _setmode(_fileno(stdin), _O_BINARY);
  _setmode(_fileno(stdout), _O_BINARY);

  RAWSource source(spk_unknown, stdin, 8192);
  RAWRenderer sink(stdout);
  Spdifer spdifer;

  Chunk chunk;
  while (!source.eof())
  {
    source.get_chunk(&chunk);
    spdifer.process_to(&chunk, &sink);
  }

  return 0;
}
