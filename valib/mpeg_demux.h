/*
  MPEG parser&demuxer

  //////////////////////////////////////////////////////////////////////////////
  // PSParser - a simple MPEG1/2 Program Stream parser.

  uint8_t header[268];
    Packet header (including the substream header)

  uint8_t *subheader;
    Pointer to the beginning of the subheader (0 for no subheader)

  size_t header_size;
    The size of the header (amount of the data at the header buffer).

  size_t payload_size;
    Packet payload size.

  int stream;
    Program stream number (stream_id).

  int substream;
    Substream number (0 for no substream)

  int packets;
    Number of packets processed.

  int errors;
    Number of parser errors.

  void reset();
    Drop the parser state to initial (except frame and error counters).

  size_t parse(uint8_t **buf, uint8_t *end);
    Parse data buffer.

    When packet is found it sets buf to the start of the payload and returns
    the size of the payload. header, subheader, header_size, payload_size, 
    stream, substream contain current frame info.

    When no packet is found, it returns zero. header_size and payload_size are
    set to zero.

  bool is_audio();
    Returns true when the packed loaded is an audio packet.

  Speakers spk();
    Returns the format of an audio packed loaded. spk_unknown when no packet is
    loaded, not an audio packet or unknown format of the packet.

  //////////////////////////////////////////////////////////////////////////////
  // PSDemux - a simple MPEG1/2 Program Stream demuxer.

  This class is also a good example how to use PSParser class.

  Demux may work in 2 modes:
  1) Extract certain elementary stream from a multiplexed MPEG stream.
  2) Extract elementart stream from a PES stream.

  In first mode we must to know stream/substream numbers we want to extract. 
  Demuxer will extract only the specified stream.

  In second mode we may not know stream/substream numbers (set it to 0). 
  Demuxer will extract any stream it finds. So input must contain exactly one 
  stream (PES stream).

  Demuxer interface is extremely simple: we must just specify stream/substream
  numbers, get stream buffer and call demux() function. It replaces original
  buffer data with demuxed data. New data size is returned.
  //////////////////////////////////////////////////////////////////////////////

  todo: Transport Stream parser
*/

#ifndef VALIB_MPEG_DEMUX_H
#define VALIB_MPEG_DEMUX_H

#include "defs.h"
#include "spk.h"
#include "syncscan.h"

class PSParser
{
private: // private data
  SyncScan scanner;
  enum { state_sync, state_header, state_drop } state;
  size_t data_size;     // data size for internal use

public: // public data
  uint8_t header[268];  // packet header (including substream header)
  uint8_t *subheader;   // pointer to subheader start (0 - no subheader)

  size_t  header_size;  // header size
  size_t  payload_size; // packet payload size;

  int stream;           // stream number
  int substream;        // substream number (0 for no substream)

  int packets;          // packets processed
  int errors;           // errors

public: // public interface
  PSParser();

  void   reset();
  size_t parse(uint8_t **buf, uint8_t *end);

  bool is_audio();
  Speakers spk();
};



class PSDemux
{
public: // public data
  PSParser parser;
  int stream;
  int substream;

public: // public interface
  PSDemux(int stream = 0, int substream = 0);

  void   reset();
  void   set(int stream, int substream = 0);
  size_t demux(uint8_t *buf, size_t size);
};

#endif
