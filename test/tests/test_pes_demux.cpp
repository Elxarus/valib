/*
  PESDemux test
  Test equality of packet-level and stream-level opeations
  Compare output (packet-level and stream-level) with reference decoder
*/

#include <stdio.h>
#include <string.h>

#include "mpeg_demux.h"
#include "test_pes_demux\demux_ref.h"

const int buf_size = 8192;

///////////////////////////////////////////////////////////////////////////////
// PESDemux2
///////////////////////////////////////////////////////////////////////////////

class PESDemux2
{
protected:
  MPEGDemux pes;
  int stream;
  int substream;
  int payload_size;

public:
  PESDemux2() { reset(); };

  // sumulate stream level demux with packet level demux
  void reset();
  int streaming(uint8_t *buf, int len);
};

void 
PESDemux2::reset()
{
  stream = 0; 
  substream = 0; 
  payload_size = 0;
  pes.reset();
}
int 
PESDemux2::streaming(uint8_t *buf, int len)
{
  uint8_t *buf_read  = buf;
  uint8_t *buf_write = buf;
  int gone;

  while (len)
  {
    if (!payload_size)
    {
      payload_size = pes.packet(buf_read, len, &gone);
      buf_read += gone;
      len -= gone;
    }
    else
    {
      if (pes.stream && !stream)
        stream = pes.stream;

      if (pes.stream == stream && pes.substream && !substream)
        substream = pes.substream;

      if (stream == pes.stream && substream == pes.substream)
      {
        if (len < payload_size)
        {
          memmove(buf_write, buf_read, len);
          buf_read     += len;
          buf_write    += len;
          payload_size -= len;
          len = 0;
        }
        else
        {
          memmove(buf_write, buf_read, payload_size);
          buf_read  += payload_size;
          buf_write += payload_size;
          len       -= payload_size;
          payload_size = 0;
        }
      }
      else
      {
        if (len < payload_size)
        {
          buf_read     += len;
          payload_size -= len;
          len = 0;
        }
        else
        {
          buf_read  += payload_size;
          len       -= payload_size;
          payload_size = 0;
        }
      } // if (stream check) ... else ...
    } // if (!payload_size) ... else ...
  }

  return buf_write - buf;
}


///////////////////////////////////////////////////////////////////////////////
// Tests
///////////////////////////////////////////////////////////////////////////////

bool 
test_packet(FILE *f)
{
//  printf("test_packet(): compare with reference demuxer at packet level...\n");

  uint8_t buf[buf_size];

  int payload_size1;
  int payload_size2;
  int gone1;
  int gone2;

  PESDemux     pes1;
  PESDemux_ref pes2;

  pes1.reset();
  pes2.reset();

  payload_size1 = 0;
  payload_size2 = 0;
  while (!feof(f))
  {
    int data_size = fread(buf, 1, buf_size, f);
    uint8_t *data = buf; 

    while (data_size)
    {
      if (!payload_size1)
      {
        payload_size1 = pes1.packet(data, data_size, &gone1);
        payload_size2 = pes2.packet(data, data_size, &gone2);

        if (payload_size1 != payload_size2)
        {
          printf("test_packet(): different payload sizes returned at filepos 0x%x\n", ftell(f) - buf_size);
          return false;
        }

        if (gone1 != gone2)
        {
          printf("test_packet(): different payload sizes returned at filepos 0x%x\n", ftell(f) - buf_size);
          return false;
        }

        data      += gone1;
        data_size -= gone1;
      }
      else
      {
        if (data_size < payload_size1)
        {
          payload_size1 -= data_size;
          data_size = 0;
        }
        else
        {
          data_size -= payload_size1;
          data += payload_size1;
          payload_size1 = 0;
        }
      } // if (!payload_size1) ... else ...
    } // while (data_size)
  } // while (!feof(f))
  return true;
}

bool 
test_streaming(FILE *f)
{
//  printf("test_streaming(): compare with reference demuxer at streaming level...\n");

  uint8_t buf1[buf_size];
  uint8_t buf2[buf_size];
  int buf_size1;
  int buf_size2;

  PESDemux     pes1;
  PESDemux_ref pes2;

  pes1.reset();
  pes2.reset();

  while (!feof(f))
  {
    buf_size1 = fread(buf1, 1, buf_size, f);
    memcpy(buf2, buf1, buf_size1);
    buf_size2 = buf_size1;

    buf_size1 = pes1.streaming(buf1, buf_size1);
    buf_size2 = pes2.streaming(buf2, buf_size2);

    if (buf_size1 != buf_size2)
    {
      printf("test_streaming(): different sizes returned at filepos 0x%x\n", ftell(f) - buf_size);
      return false;
    }

    if (memcmp(buf1, buf2, buf_size1))
    {
      printf("test_streaming(): different data returned at filepos 0x%x\n", ftell(f) - buf_size);
      return false;
    }
  }
  return true;
}

bool 
test_conformance(FILE *f)
{
//  printf("test_conformance(): compare packet and stream level demux...\n");
  
  uint8_t buf1[buf_size];
  uint8_t buf2[buf_size];
  int buf_size1;
  int buf_size2;

  PESDemux  pes1;
  PESDemux2 pes2;

  pes1.reset();
  pes2.reset();

  while (!feof(f))
  {
    buf_size1 = fread(buf1, 1, buf_size, f);
    memcpy(buf2, buf1, buf_size1);
    buf_size2 = buf_size1;

    if (ftell(f) - buf_size >= 0x1f0000)
      buf_size1 = buf_size1;

    buf_size1 = pes1.streaming(buf1, buf_size1);
    buf_size2 = pes2.streaming(buf2, buf_size2);

    if (buf_size1 != buf_size2)
    {
      printf("test_conformance(): different sizes returned at filepos 0x%x\n", ftell(f) - buf_size);
      return false;
    }

    if (memcmp(buf1, buf2, buf_size1))
    {
      printf("test_conformance(): different data returned at filepos 0x%x\n", ftell(f) - buf_size);
      return false;
    }
  }
  return true;
}

///////////////////////////////////////////////////////////////////////////////
// Main
///////////////////////////////////////////////////////////////////////////////

int test_pes_demux(const char *filename)
{
  printf("Testing file %s\n", filename);

  FILE *f;

  if (!(f = fopen(filename, "rb")))
  {
    printf("!!!Error: Cannot open file %s for reading\n\n", filename);
    return 1;
  }

  int errors = 0;

  fseek(f, 0, SEEK_SET);
  if (!test_packet(f)) 
    errors++;

  fseek(f, 0, SEEK_SET);
  if (!test_streaming(f))
    errors++;  

  fseek(f, 0, SEEK_SET);
  if (!test_conformance(f))
    errors++;  

  fclose(f);

//  printf("\n");

  return errors;
}

