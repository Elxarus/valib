/**************************************************************************//**
  \file mpeg_demux.h
  \brief MPEG parser&demuxer
******************************************************************************/
// TODO: Transport Stream parser

#ifndef VALIB_MPEG_DEMUX_H
#define VALIB_MPEG_DEMUX_H

#include "defs.h"
#include "spk.h"
#include "syncscan.h"

/**************************************************************************//**
  \class PSParser
  \brief Simple MPEG1/2 Program Stream parser.

  \fn PSParser::PSParser()
    Constructor, initializes the parser. reset() is not requred after
    constructionn.

  \fn void PSParser::reset();
    Drop the parser state to initial (except frame and error counters).

    Requred to start parsing another stream.

  \fn size_t PSParser::parse(uint8_t **buf, uint8_t *end);
    \param buf Pointer to the buffer pointer.
    \param end Pointer to the end of the buffer.
    \return Payload size.

    Parse data buffer.

    'buf' is a 'moving' buffer pointer. As an intput it points to the beginning
    of the buffer to parse. When frame is found, it moves to the beginning of
    the frame's payload. When all input data is processed, it points to the end
    of the buffer.

    When packet is found parser sets 'buf' to the start of the payload and
    returns the size of the payload. header, subheader, header_size,
    payload_size, stream, substream contain current frame info.

    When all data processed and no packet was found, it returns zero.
    header_size and payload_size are set to zero.

  \fn bool PSParser::is_audio();
    Returns true when the packed loaded is an audio packet.

  \fn Speakers PSParser::spk();
    Returns the format of an audio packed loaded. spk_unknown when no packet is
    loaded, not an audio packet or unknown format of the packet.

******************************************************************************/

class PSParser
{
private:
  SyncScan scanner;
  enum { state_sync, state_header, state_drop } state;
  size_t data_size;     //!< Size of data at header[] buffer

public:
  uint8_t header[268];  //!< Packet header (including the substream header)
  uint8_t *subheader;   //!< Pointer to the beginning of the subheader (0 for no subheader)

  size_t  header_size;  //!< The size of the header (amount of the data at the header buffer).
  size_t  payload_size; //!< Packet payload size.

  int stream;           //!< Program stream number (stream_id).
  int substream;        //!< Substream number (0 for no substream)

  int packets;          //!< Number of packets processed.
  int errors;           //!< Number of parser errors.

public:
  PSParser();

  void   reset();
  size_t parse(uint8_t **buf, uint8_t *end);

  bool is_audio();
  Speakers spk();
};



/**************************************************************************//**
  \class PSDemux
  \brief Simple MPEG1/2 Program Stream demuxer.

  This class is also a good example how to use PSParser class.

  Demux may work in 2 modes:
  1) Extract certain elementary stream from a multiplexed MPEG stream.
  2) Extract elementart stream from a PES stream.

  In first mode we must know stream/substream numbers we want to extract. 
  Demuxer will extract only the specified stream.

  In second mode we may not know stream/substream numbers (set it to 0). 
  Demuxer will extract any stream it finds. So input must contain exactly one 
  stream (PES stream).

  Demuxer interface is extremely simple: we must just specify stream/substream
  numbers, get stream buffer and call demux() function. It replaces original
  buffer data with demuxed data. New data size is returned.

  \fn PSDemux::PSDemux(int stream = 0, int substream = 0)
    \param stream Stream to extract
    \param substream Sumstream to extract

    Initializes the demuxer. Stream and substream identifiy the desired stream
    to extract.

  \fn void PSDemux::reset()
    Reset the demuxer to an initial state.

  \fn void PSDemux::set(int stream, int substream = 0)
    Initializes the demuxer. Stream and substream identifiy the desired stream
    to extract.

  \fn size_t PSDemux::demux(uint8_t *buf, size_t size)
    \param buf Pointer to the start of the buffer
    \param size Size of the input data
    \return Returns the size of the data extracted.

    Demux the stream inplace. Stream is unwrapped inplace, i.e. output data is
    placed into the same buffer as input data.

******************************************************************************/

class PSDemux
{
public:
  PSParser parser;
  int stream;
  int substream;

public:
  PSDemux(int stream = 0, int substream = 0);

  void   reset();
  void   set(int stream, int substream = 0);
  size_t demux(uint8_t *buf, size_t size);
};

#endif
