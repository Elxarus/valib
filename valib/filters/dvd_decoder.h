/*
  All-in-one decoder & audio processor

  Speakers: can change format and mask
  Input formats: PCMxx, PES, AC3, MPA, DTS
  Ouptupt formats: PCMxx, SPDIF
  Format conversions:
    PCMxx -> PCMxx
    PES   -> PCMxx
    AC3   -> PCMxx
    MPA   -> PCMxx
    DTS   -> PCMxx
    PCMxx -> SPDIF
    PES   -> SPDIF
    AC3   -> SPDIF
    MPA   -> SPDIF
    DTS   -> SPDIF
  Buffering: no
  Timing: complex
  Parmeters:
    output - output speakers configuration
    spdif  - SPDIF passthrough formats mask
*/

#ifndef DVD_DECODER_H
#define DVD_DECODER_H

#include "demux.h"
#include "decoder.h"
#include "proc.h"
#include "spdifer.h"
#include "parsers\ac3\ac3_enc.h"


#define SPDIF_MODE_NONE        0
#define SPDIF_MODE_PASSTHROUGH 1
#define SPDIF_MODE_ENCODE      2

class DVDDecoder : public Filter
{
protected:
  Speakers in_spk;
  Speakers out_spk;
  Chunk    chunk;

  Demux   demux;
  AC3Enc  encoder;
  Spdifer spdifer;
  int     spdif;
  int     spdif_mode;
  bool    ac3_encode;

  int stream;
  int substream;

  bool process_internal();

  // stack of states
  enum state_t { state_none = 0, state_demux, state_dec, state_proc, state_enc, state_spdif };
  state_t state_stack[8];
  int state_ptr;

  inline state_t state()     { return state_ptr? state_stack[state_ptr-1]: state_none; }
  inline void push_state(state_t s) { state_stack[state_ptr++] = s; }
  inline state_t pop_state() { return state_ptr? state_stack[--state_ptr]: state_none; }

public:
  AudioDecoder   dec;
  AudioProcessor proc;

  DVDDecoder();

  bool query_output(Speakers spk);
  bool set_output(Speakers spk);

  inline int  get_spdif();
  inline void set_spdif(int spdif);
  inline int  get_spdif_mode();

  inline void get_info(char *buf, int len);

  // Filter interface
  virtual void reset();

  virtual bool query_input(Speakers spk) const;
  virtual bool set_input(Speakers spk);
  virtual Speakers get_input() const;
  virtual bool process(const Chunk *chunk);

  virtual Speakers get_output() const;
  virtual bool is_empty() const;
  virtual bool get_chunk(Chunk *chunk);
};


int DVDDecoder::get_spdif()
{ return spdif; }

void DVDDecoder::set_spdif(int _spdif)
{ spdif = _spdif; }

int DVDDecoder::get_spdif_mode()
{ return spdif_mode; }

void DVDDecoder::get_info(char *_buf, int _len)
{
  if (spdif_mode == SPDIF_MODE_PASSTHROUGH)
    spdifer.get_info(_buf, _len);
  else
    dec.get_info(_buf, _len);
}

#endif