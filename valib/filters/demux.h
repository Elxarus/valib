/*
  Demuxer - demux any container stream 
  MPEG1/2 PES supported 
  MPEG2 transport stream planned
  Encoded stream detector in PCM planned

  Speakers: can change format
  Input formats: PES, SPDIF, PCM16_BE
  Output formats: AC3, MPA, DTS, PCM16_BE, PCM16_LE PCM24_LE
  Format conversions:
    PES -> AC3
    PES -> MPA
    PES -> DTS
    PES -> PCM16_LE
    PES -> PCM24_LE
    [TS -> AC3]
    [TS -> MPA]
    [TS -> DTS]
    [TS -> PCM16_LE]
    [TS -> PCM24_LE]
  Timing: preserve original (todo: apply input timestamp to the first syncpoint found at the input data?)
  Buffering: no
  Parameters:
    -
*/

#ifndef DEMUX_H
#define DEMUX_H

#include "mpeg_demux.h"
#include "filter.h"

class Demux : public NullFilter
{
protected:
  MPEGDemux pes;        // MPEG demuxer

  int pes_size;         // PES packet payload size
  int stream;           // current stream
  int substream;        // current substream
  Speakers stream_spk;  // speaker configuration of contained stream

public:
  Demux();

  inline int get_stream()    const { return stream;    }
  inline int get_substream() const { return substream; }
    
  // Filter interface
  virtual void reset();
  virtual bool query_input(Speakers spk) const;
  virtual bool process(const Chunk *chunk);
  virtual Speakers get_output() const;
};



#endif;