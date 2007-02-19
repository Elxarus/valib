#include <stdio.h>
#include <fcntl.h>
#include <io.h>

#include "parsers\ac3\ac3_header.h"
#include "parsers\dts\dts_header.h"
#include "parsers\mpa\mpa_header.h"
#include "parsers\spdif\spdif_header.h"
#include "parsers\spdif\spdif_wrapper.h"
#include "parsers\multi_header.h"
#include "parsers\file_parser.h"

#include "sink\sink_raw.h"
#include "sink\sink_wav.h"
#include "vtime.h"

///////////////////////////////////////////////////////////////////////////////
// Main
///////////////////////////////////////////////////////////////////////////////

int main(int argc, const char **argv)
{
  bool stat = true;  // display statistics

  FileParser file;

  RAWSink   raw;
  WAVSink   wav;
  Sink     *sink;

  if (argc < 3)
  {
    printf(

"Spdifer\n"
"=======\n"
"This utility encapsulates AC3/DTS/MPEG Audio stream into SPDIF stream\n"
"according to IEC 61937\n"
"\n"
"This utility is a part of AC3Filter project (http://ac3filter.net)\n"
"Copyright (c) 2006 by Alexander Vigovsky\n"
"\n"
"Usage:\n"
"  spdifer input_file output_file [-raw | -wav]\n"
"\n"
"Options:\n"
"  input_file  - file to convert\n"
"  output_file - file to write result to\n"
"  -raw - produce raw SPDIF stream output (default)\n"
"  -wav - produce PCM WAV file with SPDIF data (for writing to CD Audio)\n"

    );
    return 0;
  }

  /////////////////////////////////////////////////////////
  // Parse command line

  const char *input_filename = argv[1];
  const char *output_filename = argv[2];
  enum { mode_raw, mode_wav } mode = mode_raw;

  if (argc > 3)
  {
    if (!strcmp(argv[3], "-raw")) mode = mode_raw;
    if (!strcmp(argv[3], "-wav")) mode = mode_wav;
  }

  /////////////////////////////////////////////////////////
  // Open files

  const HeaderParser *parser_list[] = { &spdif_header, &ac3_header, &dts_header, &mpa_header };
  MultiHeader multi_parser(parser_list, array_size(parser_list));

  if (!file.open(input_filename, &multi_parser))
  {
    fprintf(stderr, "Cannot open input file '%s'\n", input_filename);
    return 1;
  }

  switch (mode)
  {
  case mode_wav:
    if (!wav.open(output_filename))
    {
      fprintf(stderr, "Cannot open output file '%s'\n", output_filename);
      return 1;
    }
    sink = &wav;
    break;

  case mode_raw:
    if (!raw.open(output_filename))
    {
      fprintf(stderr, "Cannot open output file '%s'\n", output_filename);
      return 1;
    }
    sink = &raw;
    break;

  }

  /////////////////////////////////////////////////////////
  // Do the job

  SPDIFWrapper spdifer;
  int streams = 0;
  int frames = 0;
  int skipped = 0;
  int errors = 0;

  size_t size = 0;

  vtime_t time = local_time();
  vtime_t old_time = time;
  vtime_t start_time = time;

  Chunk chunk;
  while (!file.eof())
  {
    if (file.load_frame())
    {
      frames++;
      if (file.is_new_stream())
        streams++;

      if (spdifer.parse_frame(file.get_frame(), file.get_frame_size()))
      {
        Speakers spk = spdifer.get_spk();
        if (spk.format == FORMAT_SPDIF)
        {
          chunk.set_rawdata(Speakers(FORMAT_PCM16, MODE_STEREO, spk.sample_rate), spdifer.get_rawdata(), spdifer.get_rawsize());
          if (sink->process(&chunk))
            size += chunk.size;
          else
            errors++;
        }
        else
          skipped++;
      }
      else
        errors++;
    } // if (file.load_frame())

    time = local_time();
    if (file.eof() || time > old_time + 0.1)
    {
      old_time = time;

      float file_size = float(file.get_size());
      float processed = float(file.get_pos());
      float elapsed   = float(old_time - start_time);
      int eta = int((file_size / processed - 1.0) * elapsed);

      fprintf(stderr, "%02i%% %02i:%02i In: %iM (%2iMB/s) Out: %iM Frames: %i (%s)       \r", 
        int(processed * 100 / file_size),
        eta / 60, eta % 60,
        int(processed / 1000000), int(processed/elapsed/1000000), size / 1000000, frames,
        file.is_in_sync()? file.get_spk().format_text(): "Unknown");
    }
  } // while (!file.eof())

  // flush output
  chunk.set_empty(sink->get_input(), false, 0, true);
  sink->process(&chunk);

  // close file
  switch (mode)
  {
    case mode_wav: wav.close(); break;
    case mode_raw: raw.close(); break;
  }

  // Final notes
  printf("\n");
  if (streams > 1) printf("%i streams converted\n", streams);
  if (skipped)     printf("%i frames skipped\n", skipped);
  if (errors)      printf("%i frame conversion errors\n", errors);

  return 0;
}
