/*
  Demuxer - demux MPEG container (filter wrapper for MPEGDemux class)
  MPEG1/2 PES supported 
  todo: MPEG2 transport stream

  Input:     PES
  Output:    AC3, MPA, DTS, PCM16_LE PCM24_LE
  OFDD:      yes
  Buffering: inplace
  Timing:    passthrough
  Parameters:
    [ro] stream - current stream number
    [ro] substream - current substream number
*/

#ifndef VALIB_DEMUX_H
#define VALIB_DEMUX_H

#include "../filter2.h"
#include "../mpeg_demux.h"

class Demux : public SimpleFilter
{
protected:
  PSParser ps;          // MPEG Program Stream parser
  Speakers out_spk;     // current output format
  int      stream;      // current stream
  int      substream;   // current substream

public:
  Demux();

  inline int get_stream()    const { return stream;    }
  inline int get_substream() const { return substream; }

  /////////////////////////////////////////////////////////
  // SimpleFilter overrides

  virtual bool can_open(Speakers spk) const;
  virtual bool process(Chunk2 &in, Chunk2 &out);
  virtual void reset();

  virtual Speakers get_output() const
  { return out_spk; }

  virtual bool is_inplace() const
  { return true; }

  virtual bool is_ofdd() const
  { return true; }

  virtual bool eos() const
  {
    if ((stream && stream != ps.stream) ||
        (stream && substream && substream != ps.substream))
      return true;
    else
      return false;
  }
};

#endif
