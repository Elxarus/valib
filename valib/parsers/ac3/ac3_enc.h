#ifndef VALIB_AC3_ENC_H
#define VALIB_AC3_ENC_H

#include "../../filter.h"
#include "../../bitstream.h"
#include "../../buffer.h"
#include "ac3_defs.h"
#include "ac3_mdct.h"

inline int sym_quant(int m, int levels);
inline int asym_quant(int m, int bits);

class AC3Enc : public SimpleFilter
{
public:
  // filter data
  size_t    sample;
  SampleBuf frame_samples;
  Rawdata   frame_buf;
  SampleBuf window;

  bool    sync;
  vtime_t time;

  // decoder mode
  int sample_rate;
  int bitrate;
  int frame_size;
  int frames;

  MDCT mdct;
  WriteBS bs;

  // stream-level data
  int  acmod;
  bool dolby;
  bool lfe;
  int  nfchans;

  int bsid;                            // 'bsid' - bitstream identification
  int fscod;                           // 'fscod' - sample rate code
  int halfratecod;                     // 'halfratecod' - half-rate code
  int frmsizecod;                      // 'frmsizecod' - frame size code
  // bit allocation
  int sdcycod;                         // 'sdcycod' - slow decay code
  int fdcycod;                         // 'fdcycod' - fast decay code
  int sgaincod;                        // 'sgaincod' - slow gain code
  int dbpbcod;                         // 'dbpbcod' - dB per bit code
  int floorcod;                        // 'floorcod' - floor code
  int fgaincod;                        // 'fgaincod' - fast gain code

  int csnroffst;                       // 'csnroffst' - coarse SNR offset
  int fsnroffst;                       // 'fsnroffst' - fine SNR offset

  int16_t  delay[AC3_NCHANNELS][AC3_BLOCK_SAMPLES]; // delay buffer (not normalized)
  int      delay_exp[AC3_NCHANNELS];                // delay buffer normalization
                                           
  int32_t  mant[AC3_NCHANNELS][AC3_NBLOCKS][AC3_BLOCK_SAMPLES];    // mdct coeffitients
  int8_t   exp[AC3_NCHANNELS][AC3_NBLOCKS][AC3_BLOCK_SAMPLES];     // exponents
  int8_t   expcod[AC3_NCHANNELS][AC3_NBLOCKS][AC3_BLOCK_SAMPLES];  // encoded exponents
  int8_t   bap[AC3_NCHANNELS][AC3_NBLOCKS][AC3_BLOCK_SAMPLES];     // bit allocation pointers

  int      chbwcod[AC3_NCHANNELS-1];                      // 'chbwcod' - channel bandwidth code (fbw only)
  int      nmant[AC3_NCHANNELS];                          // number of mantissas
  int      expstr[AC3_NCHANNELS][AC3_NBLOCKS];            // 'expstr'/'lfeexpstr' - exponent strategy
  int      ngrps[AC3_NCHANNELS][AC3_NBLOCKS];             // number of exponent groups

  inline void output_mant(WriteBS &pb, int8_t bap[AC3_BLOCK_SAMPLES], int32_t mant[AC3_BLOCK_SAMPLES], int8_t exp[AC3_BLOCK_SAMPLES], int start, int end) const;
  inline void compute_expstr(int expstr[AC3_NBLOCKS], int8_t exp[AC3_NBLOCKS][AC3_BLOCK_SAMPLES], int endmant) const;
  inline void restrict_exp(int8_t expcod[AC3_NBLOCKS][AC3_BLOCK_SAMPLES], int ngrps[AC3_NBLOCKS], int8_t exp[AC3_NBLOCKS][AC3_BLOCK_SAMPLES], int expstr[AC3_NBLOCKS], int endmant) const;
  inline int  encode_exp(int8_t expcod[AC3_BLOCK_SAMPLES], int8_t exp[AC3_BLOCK_SAMPLES], int expstr, int endmant) const;

  bool fill_buffer(Chunk &in);
  int  encode_frame();

public:
  //! Encoding error exception
  struct EncodingError : public Filter::Error {};

  AC3Enc();

  int  get_bitrate() const;
  bool set_bitrate(int bitrate);

  /////////////////////////////////////////////////////////
  // Filter interface

  virtual bool can_open(Speakers spk) const;
  virtual bool init();

  virtual void reset();
  virtual bool process(Chunk &in, Chunk &out);

  virtual Speakers get_output() const
  { return Speakers(FORMAT_AC3, spk.mask, spk.sample_rate, 1.0, spk.relation); }
};



// symmetric quantization
inline int sym_quant(int c, int levels)
{
  int v = (c + 32768) * levels >> 16;
//  _ASSERT(v < levels);
  return v;
}

// asymmetric quantization
inline int asym_quant(int c, int bits)
{
  c >>= 16 - bits;
  c &= (1 << bits) - 1; // truncate unused bits
  return c;
}

#endif
