#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <memory.h>
#include "mpeg_demux.h"

const int buf_size = 65536;

///////////////////////////////////////////////////////////////////////////////
// File information
///////////////////////////////////////////////////////////////////////////////

const char *stream_type(int stream)
{
  if (stream == 0xbc) return "Reserved stream";
  if (stream == 0xbe) return "Padding stream";
  if (stream == 0xbd) return "Private stream 1";
  if (stream == 0xbf) return "Private stream 2";
  if ((stream & 0xe0) == 0xc0) return "MPEG Audio stream";
  if ((stream & 0xf0) == 0xe0) return "MPEG Video stream";
  if ((stream & 0xf0) == 0xf0) return "Reserved data stream";
  return "unknown stream type";
}
const char *substream_type(int substream)
{
  if ((substream & 0xf8) == 0x80) return "AC3 Audio substream";
  if ((substream & 0xf8) == 0x88) return "DTS Audio substream";
  if ((substream & 0xf0) == 0xa0) return "LPCM Audio substream";
  if ((substream & 0xf0) == 0x20) return "Subtitle substream";
  if ((substream & 0xf0) == 0x30) return "Subtitle substream";
  return "unknown substream type";
}

void info(FILE *f)
{
  uint8_t buf[buf_size];
  uint8_t *data;
  int data_size;

  int gone;
  int payload_size;

  int stream[256];
  int substream[256];

  memset(stream, 0, sizeof(stream));
  memset(substream, 0, sizeof(substream));

  MPEGDemux pes;
  pes.reset();

  payload_size = 0;
  while (!feof(f) && (ftell(f) < 1024 * 1024)) // analyze only first 1MB
  {
    data_size = fread(buf, 1, buf_size, f);
    data = buf;

    while (data_size)
    {
      if (!payload_size)
      {
        payload_size = pes.packet(data, data_size, &gone);
        data += gone;
        data_size -= gone;

        if (payload_size)
        {
          if (!stream[pes.stream])
          {
            stream[pes.stream]++;
            printf("Found stream %x (%s)      \n", pes.stream, stream_type(pes.stream));
          }
          if (pes.stream == 0xBD && !substream[pes.substream])
          {
            substream[pes.substream]++;
            printf("Found substream %x (%s)   \n", pes.substream, substream_type(pes.substream));
          }
        }
        else
          continue;
      }

      if (data_size < payload_size)
      {
        payload_size -= data_size;
        data_size = 0;
      }
      else
      {
        data_size -= payload_size;
        data += payload_size;
        payload_size = 0;
      }
    }  
  }

  if (pes.errors)
    printf("Stream contains errors (not a MPEG program stream?)\n");
}

///////////////////////////////////////////////////////////////////////////////
// Demux
///////////////////////////////////////////////////////////////////////////////

void demux(FILE *f, FILE *out, int stream, int substream)
{
  uint8_t buf[buf_size];
  int data_size;
  int file_size;

  // Determine input file size

  fseek(f, 0, SEEK_END);
  file_size = ftell(f);
  fseek(f, 0, SEEK_SET);

  // validate stream/substream numbers

  if (!stream && substream) stream = 0xbd;
  if (stream != 0xbd)       substream = 0;

  MPEGDemux pes;
  pes.stream = stream;
  pes.substream = substream;
  pes.reset();

  while (!feof(f))
  {
    data_size = fread(buf, 1, buf_size, f);
    data_size = pes.streaming(buf, data_size);
    fwrite(buf, 1, data_size, out);
    // more stats!!!
  }

  printf("Errors: %i\n", pes.errors);
}

///////////////////////////////////////////////////////////////////////////////
// Argument parsing
///////////////////////////////////////////////////////////////////////////////

enum arg_type { argt_exist, argt_bool, argt_num, argt_hex };

bool is_arg(char *arg, const char *name, arg_type type)
{
  if (arg[0] != '-') return false;
  arg++;

  while (*name)
    if (*name && *arg != *name) 
      return false;
    else
      name++, arg++;

  if (type == argt_exist && *arg == '\0') return true;
  if (type == argt_bool && (*arg == '\0' || *arg == '+' || *arg == '-')) return true;
  if (type == argt_num && (*arg == ':' || *arg == '=')) return true;
  if (type == argt_hex && (*arg == ':' || *arg == '=')) return true;

  return false;
}

bool arg_bool(char *arg)
{
  arg += strlen(arg) - 1;
  if (*arg == '-') return false;
  return true;
}

double arg_num(char *arg)
{
  arg += strlen(arg);
  while (*arg != ':' && *arg != '=')
    arg--;
  arg++;
  return atof(arg);
}

int arg_hex(char *arg)
{
  arg += strlen(arg);
  while (*arg != ':' && *arg != '=')
    arg--;
  arg++;

  int result;
  sscanf(arg, "%x", &result);
  return result;
}

///////////////////////////////////////////////////////////////////////////////
// Main
///////////////////////////////////////////////////////////////////////////////

int main(int argc, char **argv)
{
  int stream = 0;
  int substream = 0;

  char *filename = 0;
  char *filename_out = 0;

  FILE *f = 0;
  FILE *out = 0;

  printf("MPEG Program Stream demuxer.\n"
         "This utility is part of AC3Filter project (http://ac3filter.sourceforge.net)\n"
         "Copyright (c) 2004 by Alexander Vigovsky\n\n");
  if (argc < 2)
  {
    printf("Usage:\n"
           "  mpeg_demux file.pes [-i] [-d output.raw [-stream:x | substream:x]]\n"
           "  -i - file info (default)\n"
           "  -d - demux\n"
           "  -stream:xx - demux stream xx (hex)\n"
           "  -substeam:xx - demux substream xx (hex)\n");
    exit(0);
  }

  int  iarg = 0;
  enum { mode_none, mode_info, mode_demux } mode = mode_none;

  // Parse arguments

  filename = argv[1];

  for (iarg = 2; iarg < argc; iarg++)
  {
    // -i - info
    if (is_arg(argv[iarg], "i", argt_exist))
    {
      if (mode != mode_none)
      {
        printf("-i : ambigous mode\n");
        return 1;
      }

      mode = mode_info;
      continue;
    }  

    // -d - demux
    if (is_arg(argv[iarg], "d", argt_exist))
    {
      if (argc - iarg < 2)
      {
        printf("-d : specify a file name\n");
        return 1;
      }
      if (mode != mode_none)
      {
        printf("-d : ambigous mode\n");
        return 1;
      }

      mode = mode_demux;
      filename_out = argv[++iarg];
      continue;
    }

    // -stream - stream to demux
    if (is_arg(argv[iarg], "stream", argt_hex))
    {
      stream = arg_hex(argv[iarg]);
      continue;
    }

    // -substream - substream to demux
    if (is_arg(argv[iarg], "substream", argt_hex))
    {
      substream = arg_hex(argv[iarg]);
      continue;
    }

    printf("Unknown parameter: %s\n", argv[iarg]);
    return 1;
  }

  if (stream && stream != 0xbd && substream)
  {
    printf("Cannot demux substreams for stream 0x%x\n", stream);
    return 1;
  }

 if (!(f = fopen(filename, "rb")))
  {
    printf("Cannot open file %s for reading\n", filename);
    return 1;
  }

  if (filename_out)
    if (!(out = fopen(filename_out, "wb")))
    {
      printf("Cannot open file %s for writing\n", filename_out);
      return 1;
    }

  // Do the job

  switch (mode)
  {
  case mode_none:
  case mode_info:
    info(f);
    break;

  case mode_demux:
    demux(f, out, stream, substream);
  }

  // Finish

  fclose(f);
  if (out) fclose(out);

  return 0;
}
