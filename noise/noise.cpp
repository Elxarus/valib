#include <stdio.h>

#include "source\noise.h"
#include "filter_graph.h"
#include "sink\sink_dsound.h"


int main()
{
  Speakers    spk(FORMAT_PCM16, MODE_STEREO, 48000);

  Noise       noise(spk, spk.sample_rate * spk.nch() * spk.sample_size());
  DSoundSink  ds(0);
  FilterChain filter;

  Sink *sink = &ds;

  /////////////////////////////////////////////////////////
  // Setup output
  /////////////////////////////////////////////////////////

  printf("Opening %s %s %iHz audio output...\n", spk.format_text(), spk.mode_text(), spk.sample_rate);
  if (!ds.set_input(spk))
  {
    printf("Error: Cannot open audio output!");
    return 1;
  }

  /////////////////////////////////////////////////////////
  // Process
  /////////////////////////////////////////////////////////

  Chunk chunk;
  do {
    if (!noise.get_chunk(&chunk))
    {
      printf("noise.get_chunk() failed\n");
      return 1;
    }

    if (chunk.eos)
      chunk.eos = true;

    if (!sink->process(&chunk))
    {
      printf("sink.process() failed\n");
      return 1;
    }
  } while (!chunk.eos);

  return 0;
}