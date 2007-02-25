#include <stdio.h>

#include "parser.h"
#include "bitstream.h"
#include "parsers\mpa\mpa_header.h"
#include "parsers\ac3\ac3_header.h"
#include "parsers\dts\dts_header.h"
#include "parsers\spdif\spdif_header.h"
#include "parsers\spdif\spdif_parser.h"
#include "parsers\multi_header.h"
#include "auto_file.h"


inline const char *bs_name(int bs_type);
void print_info(StreamBuffer &stream, int bs_type);
inline bool is_14bit(int bs_type)
{
  return bs_type == BITSTREAM_14LE || bs_type == BITSTREAM_14BE;
}

int main(int argc, char **argv)
{
  if (argc < 2)
  {
    printf(
"Bitstream converter\n"
"===================\n"
"This utility does conversion between numerous MPA/AC3/DTS stream types:\n"
"SPDIF padded, 8/14/16bit big/low endian. By default, it converts any\n"
"stream type to most common byte stream."
"\n"
"This utility is a part of AC3Filter project (http://ac3filter.net)\n"
"Copyright (c) 2006 by Alexander Vigovsky\n"
"\n"
"Usage:\n"
"  Detect file type and print file information:\n"
"  > bsconvert input_file\n"
"\n"
"  Convert a file:\n"
"  > bsconvert input_file output_file [format]\n"
"\n"
"Options:\n"
"  input_file  - file to convert\n"
"  output_file - file to write result to\n"
"  format      - output file format:\n"
"    8     - byte stream (default)\n"
"    16le  - 16bit low endian\n"
"    14be  - 14bit big endian (DTS only)\n"
"    14le  - 14bit low endian (DTS only)\n"
"\n"
"Notes:\n"
"  File captured from SPDIF input may contain several parts of different type.\n"
"  For example SPDIF transmission may be started with 5.1 ac3 format then\n"
"  switch to 5.1 dts and then to stereo ac3. In this case all stream parts\n"
"  will be converted and writed to the same output file\n"
"\n"
"  SPDIF stream is padded with zeros, therefore output file size may be MUCH\n"
"  smaller than input. It is normal and this does not mean that some data was\n"
"  lost. This conversion is loseless! You can recreate SPDIF stream back with\n"
"  'spdifer' utility. (Currently, compacting works only for AC3/MPA)\n"
"\n"
"  14bit streams are supported only for DTS format. Note, that conversion\n"
"  between 14bit and non-14bit changes actual frame size (frame interval),\n"
"  but does not change the frame header.\n"
    );
    return -1;
  }

  /////////////////////////////////////////////////////////
  // Parse command line

  char *in_filename = 0;
  char *out_filename = 0;
  int bs_type = BITSTREAM_8;

  switch (argc)
  {
  case 2:
    in_filename = argv[1];
    break;

  case 3:
    in_filename = argv[1];
    out_filename = argv[2];
    break;

  case 4:
    in_filename = argv[1];
    out_filename = argv[2];
    if (!strcmp(argv[3], "8"))
      bs_type = BITSTREAM_8;
    else if (!strcmp(argv[3], "16le"))
      bs_type = BITSTREAM_16LE;
    else if (!strcmp(argv[3], "14be"))
      bs_type = BITSTREAM_14BE;
    else if (!strcmp(argv[3], "14le"))
      bs_type = BITSTREAM_14LE;
    else
    {
      printf("Unknown stream format: %s", argv[3]);
      return -1;
    }
    break;

  default:
    printf("Wrong number of arguments");
    return -1;
  }

  /////////////////////////////////////////////////////////
  // Allocate buffers

  const HeaderParser *headers[] = { &spdif_header, &ac3_header, &mpa_header, &dts_header };
  MultiHeader multi_header(headers, array_size(headers));

  SPDIFParser spdif_parser(false);
  StreamBuffer stream(&multi_header);

  const size_t buf_size = 512*1024;
  uint8_t *buf = new uint8_t[buf_size];

  const size_t framebuf_size = (multi_header.max_frame_size()) / 7 * 8 + 8;
  uint8_t *framebuf = new uint8_t[framebuf_size];

  if (!buf || !framebuf)
  {
    printf("Cannot allocate buffer");
    return -1;
  }

  /////////////////////////////////////////////////////////
  // Open input file

  AutoFile in_file(in_filename);
  if (!in_file.is_open())
  {
    printf("Cannot open file %s", in_filename);
    return -1;
  }

  /////////////////////////////////////////////////////////
  // Detect input file format and print stream info

  while (in_file.pos() < 1000000 && !stream.is_in_sync())
  {
    size_t data_size = in_file.read(buf, buf_size);
    uint8_t *ptr = buf;
    uint8_t *end = ptr + data_size;
    stream.load_frame(&ptr, end);
  }

  if (!stream.is_in_sync())
  {
    printf("Cannot detect file format\n");
    return -1;
  }
  else
  {
    printf("%s\n", in_filename);
    print_info(stream, bs_type);
  }

  /////////////////////////////////////////////////////////
  // Open output file

  if (!out_filename)
    return 0;

  AutoFile out_file(out_filename, "wb");
  if (!out_file.is_open())
  {
    printf("Cannot open file %s\n", out_filename);
    return -1;
  }

  /////////////////////////////////////////////////////////
  // Process data

  bs_conv_t conv;
  bool show_info = false;

  int frames = 0;
  int bs_target = bs_type;
  bool is_spdif = false;
  in_file.seek(0);
  stream.reset();

  while (!in_file.eof())
  {
    size_t data_size = in_file.read(buf, buf_size);
    uint8_t *ptr = buf;
    uint8_t *end = ptr + data_size;

    while (ptr < end)
      if (stream.load_frame(&ptr, end))
      {
        frames++;

        // Switch to a new stream
        if (stream.is_new_stream())
        {
          if (show_info)
          {
            printf("\n\n");
            print_info(stream, bs_type);
          }
          else
            show_info = true;

          // find conversion function
          HeaderInfo hdr = stream.header_info();
          is_spdif = hdr.spk.format == FORMAT_SPDIF;
          if (is_spdif)
          {
            if (spdif_parser.parse_frame(stream.get_frame(), stream.get_frame_size()))
              hdr = spdif_parser.header_info();
            else
              hdr.drop();
          }

          bs_target = bs_type;
          if (is_14bit(bs_target) && hdr.spk.format != FORMAT_DTS)
            bs_target = BITSTREAM_8;

          printf("Conversion from %s to %s\n", bs_name(hdr.bs_type), bs_name(bs_target));
          conv = bs_conversion(hdr.bs_type, bs_target);
          if (!conv)
            printf("Cannot convert this stream!\n");
        }

        // Do the job here
        if (conv)
        {
          const uint8_t *frame = stream.get_frame();
          size_t frame_size = stream.get_frame_size();

          if (is_spdif)
            if (spdif_parser.parse_frame(stream.get_frame(), stream.get_frame_size()))
            {
              frame = spdif_parser.get_rawdata();
              frame_size = spdif_parser.get_rawsize();
            }
            else
            {
              frame = 0;
              frame_size = 0;
            }

          size_t framebuf_data = (*conv)(frame, frame_size, framebuf);

          // Correct DTS header
          if (bs_target == BITSTREAM_14LE)
            framebuf[3] = 0xe8;
          else if (bs_target == BITSTREAM_14BE)
            framebuf[2] = 0xe8;

          out_file.write(framebuf, framebuf_data);
        }
      }

    if (conv)
      fprintf(stderr, "Frame: %i\r", frames);
    else
      fprintf(stderr, "Skipping: %i\r", frames);
  }
  printf("Frames found: %i\n\n", frames);

  delete buf;
  return 0;
}


void print_info(StreamBuffer &stream, int bs_type)
{
  const size_t info_size = 1024;
  char info[info_size];
  stream.stream_info(info, info_size);
  printf(info);
  printf("\n");

  SPDIFParser spdif_parser(false);
  HeaderInfo hdr = stream.header_info();

  bool is_spdif = hdr.spk.format == FORMAT_SPDIF;
  if (is_spdif)
  {
    if (spdif_parser.parse_frame(stream.get_frame(), stream.get_frame_size()))
      hdr = spdif_parser.header_info();
    else
      printf("\nERROR!!!\nCannot parse SPDIF frame\n");
  }

  if (is_14bit(bs_type) && hdr.spk.format != FORMAT_DTS)
  {
    printf(
"\nWARNING!!!\n"
"%s does not support 14bit stream format!\n"
"It will be converted to byte stream.\n\n", hdr.spk.format_text());
  }
}

inline const char *bs_name(int bs_type)
{
  switch (bs_type)
  {
    case BITSTREAM_8:    return "byte";
    case BITSTREAM_16BE: return "16bit big endian";
    case BITSTREAM_16LE: return "16bit low endian";
    case BITSTREAM_32BE: return "32bit big endian";
    case BITSTREAM_32LE: return "32bit low endian";
    case BITSTREAM_14BE: return "14bit big endian";
    case BITSTREAM_14LE: return "14bit low endian";
    default: return "??";
  }
}
