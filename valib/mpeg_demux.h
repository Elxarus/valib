#ifndef VALIB_MPEGDEMUX_H
#define VALIB_MPEGDEMUX_H

#include "defs.h"
#include "spk.h"

//////////////////////////////////////////////////
// MPEG1/2 Program Stream demuxer

class MPEGDemux
{
private:
  enum { demux_sync, demux_header, demux_data, demux_drop } state;
  uint8_t header[268];
  uint8_t subheader[6];
  int data_size;

public:
  int stream;
  int substream;

  int frames;
  int errors;

  MPEGDemux()
  {
    frames = 0;
    errors = 0;
    stream = 0;
    substream = 0;
    state = demux_sync;
    data_size = 0;
  };

  void reset()
  {
    state = demux_sync; 
    data_size = 0; 
  };

  void reset(int _stream, int _substream = 0)
  {
    stream = _stream;
    substream = _substream;
    state = demux_sync; 
    data_size = 0; 
  };

  bool is_audio();
  Speakers spk();

  /////////////////////////////////////////////////////////
  // Stream-based demux
  //
  // replaces original buffer with demuxed stream 
  // returns demuxed data size

  int streaming(uint8_t *buf, int len);

  /////////////////////////////////////////////////////////
  // Packet-based demux
  //
  // finds next packet payload data
  // returns payload data size

  int packet(uint8_t *buf, int len, int *gone);
};

#endif
