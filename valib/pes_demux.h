#ifndef PESDEMUX_H
#define PESDEMUX_H
#include "defs.h"


#define DEMUX_SYNC   0
#define DEMUX_HEADER 1
#define DEMUX_DATA   2
#define DEMUX_DROP   3

//////////////////////////////////////////////////
// MPEG1/2 PES demuxer

class PESDemux
{
private:
  int state;
  uint8_t header[268];
  int data_size;

public:
  int stream;
  int substream;
  uint8_t subheader[6];

  int frames;
  int errors;

  PESDemux()
  {
    frames = 0;
    errors = 0;
    stream = 0;
    substream = 0;
    state = DEMUX_SYNC;
    data_size = 0;
  };

  void reset()
  {
    state = DEMUX_SYNC; 
    data_size = 0; 
  };

/*
  void reset(int _stream = 0, int _substream = 0)
  {
    stream = _stream;
    substream = _substream;
    state = DEMUX_SYNC; 
    data_size = 0; 
  };
*/

  /////////////////////////////////////////////////////////
  // Stream-based demux
  //
  // replaces original buffer with demuxed stream 
  // returns demuxed data size

  int streaming(uint8_t *buf, int len);

  /////////////////////////////////////////////////////////
  // Packet-based demux
  //
  // finds PES payload data
  // returns payload data size

  int packet(uint8_t *buf, int len, int *gone);
};

#endif
