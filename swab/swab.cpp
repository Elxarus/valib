#include "auto_file.h"

size_t bs_conv_swab16(const uint8_t *in_buf, size_t size, uint8_t *out_buf)
{
  uint16_t *in16 = (uint16_t *)in_buf;
  uint16_t *out16 = (uint16_t *)out_buf;
  size_t i = size >> 1;
  while (i--)
    out16[i] = swab_u16(in16[i]);

  return size;
}


int main(int argc, char **argv)
{
  if (argc < 3)
  {
    printf(
"Swab utility - swap bytes in the file\n"
"\n"
"Usage:\n"
"  > swab input_file output_file\n");
    return -1;
  }

  /////////////////////////////////////////////////////////
  // Open files

  char *in_filename = argv[1];
  char *out_filename = argv[2];

  AutoFile in_file(in_filename);
  if (!in_file.is_open())
  {
    printf("Cannot open file %s", in_filename);
    return -1;
  }

  AutoFile out_file(out_filename, "wb");
  if (!out_file.is_open())
  {
    printf("Cannot open file %s\n", out_filename);
    return -1;
  }

  /////////////////////////////////////////////////////////
  // Allocate buffer

  const size_t buf_size = 65536;
  uint8_t *buf = new uint8_t[buf_size];

  /////////////////////////////////////////////////////////
  // Do the job

  while (!in_file.eof())
  {
    size_t data_size = in_file.read(buf, buf_size);
    data_size = bs_conv_swab16(buf, data_size, buf);
    out_file.write(buf, data_size);
  }

  return 0;
}
