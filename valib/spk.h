/*
  Speaker configuration and format definition class
*/

#ifndef SPK_H
#define SPK_H

#include "defs.h"

///////////////////////////////////////////////////////////////////////////////
// Known stream formats
// note: implied assumption used in Converter class is that
// sizeof(float) == 4 (32bit float) for FORMAT_PCMFLOAT_LE 
///////////////////////////////////////////////////////////////////////////////

// indicates uninitialized configuration
#define FORMAT_UNKNOWN     0

// main format for internal processing
#define FORMAT_LINEAR      1

// PCM formats
#define FORMAT_PCM16       2
#define FORMAT_PCM24       3
#define FORMAT_PCM32       4
#define FORMAT_PCMFLOAT    5

#define FORMAT_PCM16_LE    6
#define FORMAT_PCM24_LE    7
#define FORMAT_PCM32_LE    8
#define FORMAT_PCMFLOAT_LE 9

// data containers
#define FORMAT_PES        10
#define FORMAT_SPDIF      11

// compressed formats
#define FORMAT_AC3        12
#define FORMAT_MPA        13
#define FORMAT_DTS        14
#define FORMAT_AAC        15
#define FORMAT_OGG        16

// macro to convert format number to format mask
#define FORMAT_MASK(format)  (1 << (format & 0x1f))

#define FORMAT_MASK_UNKNOWN  (FORMAT_MASK(FORMAT_UNKNOWN))
#define FORMAT_MASK_LINEAR   (FORMAT_MASK(FORMAT_LINEAR))

#define FORMAT_MASK_PCM   (0x1e)
#define FORMAT_MASK_PES   (FORMAT_MASK(FORMAT_PES))
#define FORMAT_MASK_SPDIF (FORMAT_MASK(FORMAT_SPDIF))
#define FORMAT_MASK_AC3   (FORMAT_MASK(FORMAT_AC3))
#define FORMAT_MASK_MPA   (FORMAT_MASK(FORMAT_MPA))
#define FORMAT_MASK_DTS   (FORMAT_MASK(FORMAT_DTS))

///////////////////////////////////////////////////////////////////////////////
// Channel numbers (that also define 'standard' channel order)
// may used as index in arrays
///////////////////////////////////////////////////////////////////////////////

#define CH_L    0  // Left channel
#define CH_C    1  // Center channel
#define CH_R    2  // Right channel
#define CH_SL   3  // Surround left channel
#define CH_SR   4  // Surround right channel
#define CH_LFE  5  // LFE channel
#define CH_NONE 6  // indicates that channel is not used in channel order

// synonyms
#define CH_M    1  // Mono channel = center channel
#define CH_CH1  0  // Channel 1 in Dual mono mask
#define CH_CH2  2  // Channel 2 in Dual mono mask
#define CH_S    3  // Surround channel for x/1 masks

///////////////////////////////////////////////////////////////////////////////
// Channel masks
// used as channel presence flag in a mask definition
///////////////////////////////////////////////////////////////////////////////

#define CH_MASK_L    1
#define CH_MASK_C    2
#define CH_MASK_R    4
#define CH_MASK_SL   8
#define CH_MASK_SR   16
#define CH_MASK_LFE  32

// synonyms
#define CH_MASK_M    2
#define CH_MASK_C1   0
#define CH_MASK_C2   4
#define CH_MASK_S    8

// macro to convert channel number to channel mask
#define CH_MASK(ch)  (1 << (ch & 0x1f))

///////////////////////////////////////////////////////////////////////////////
// Common channel configs
///////////////////////////////////////////////////////////////////////////////

#define MODE_UNDEFINED 0
#define MODE_1_0     (CH_MASK_M)
#define MODE_2_0     (CH_MASK_L | CH_MASK_R)
#define MODE_3_0     (CH_MASK_L | CH_MASK_C | CH_MASK_R)
#define MODE_2_1     (MODE_2_0 | CH_MASK_S)
#define MODE_3_1     (MODE_3_0 | CH_MASK_S)
#define MODE_2_2     (MODE_2_0 | CH_MASK_SL | CH_MASK_SR)
#define MODE_3_2     (MODE_3_0 | CH_MASK_SL | CH_MASK_SR)
#define MODE_1_0_LFE (CH_MASK_M | CH_MASK_LFE)
#define MODE_2_0_LFE (CH_MASK_L | CH_MASK_R | CH_MASK_LFE)
#define MODE_3_0_LFE (CH_MASK_L | CH_MASK_C | CH_MASK_R | CH_MASK_LFE)
#define MODE_2_1_LFE (MODE_2_0 | CH_MASK_S | CH_MASK_LFE)
#define MODE_3_1_LFE (MODE_3_0 | CH_MASK_S | CH_MASK_LFE)
#define MODE_2_2_LFE (MODE_2_0 | CH_MASK_SL | CH_MASK_SR | CH_MASK_LFE)
#define MODE_3_2_LFE (MODE_3_0 | CH_MASK_SL | CH_MASK_SR | CH_MASK_LFE)

// synonyms
#define MODE_MONO    MODE_1_0
#define MODE_STEREO  MODE_2_0
#define MODE_QUADRO  MODE_2_2
#define MODE_5_1     MODE_3_2_LFE


///////////////////////////////////////////////////////////////////////////////
// Interchannel relations
///////////////////////////////////////////////////////////////////////////////

#define NO_RELATION       0  // Channels are not interrelated
#define RELATION_DOLBY    1  // Dolby Surround/ProLogic (2 channels)
#define RELATION_DOLBY2   2  // Dolby ProLogic2 (2 channels)
#define RELATION_SUMDIFF  3  // Sum-difference (2 channels)

///////////////////////////////////////////////////////////////////////////////
// Speakers class
// defines audio stream format
///////////////////////////////////////////////////////////////////////////////

class Speakers
{
public:
  int format;           // data format
  int mask;             // channel mask
  int sample_rate;      // sample rate
  int relation;         // interchannel relation
  sample_t level;       // 0dB level

  Speakers() 
  {
    set_error();
  };

  Speakers(int _format, int _mask, int _sample_rate, sample_t _level = 1.0, int _relation = NO_RELATION)
  {
    set(_format, _mask, _sample_rate, _level, _relation);
  }

  inline void set(int format, int mask, int sample_rate, sample_t level = 1.0, int relation = NO_RELATION);
  inline void set_error();

  inline int  nch()   const;
  inline bool error() const;
  inline bool lfe()   const;

  inline bool is_spdif() const;
  inline bool is_pcm()   const;

  inline const int *order() const;

  inline int  sample_size() const;

  inline bool operator ==(const Speakers &spk) const;
  inline bool operator !=(const Speakers &spk) const;

  inline const char *format_text() const;
  inline const char *mode_text() const;
};

extern const int std_order[NCHANNELS];
extern const int win_order[NCHANNELS];

extern const Speakers def_spk;
extern const Speakers err_spk;
extern const Speakers unk_spk;

///////////////////////////////////////////////////////////////////////////////
// Speakers class inlines
///////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
// Speakers class inlines
///////////////////////////////////////////////////////////////////////////////

extern const int sample_size_tbl[32];
extern const int mask_nch_tbl[64];
extern const int mask_order_tbl[64][6];

inline int sample_size(int format)
{
  return sample_size_tbl[format & 0x2f];
}

inline int mask_nch(int mask)
{
  return mask_nch_tbl[mask & 0x3f];
}

inline const int *mask_order(int mask)
{
  return mask_order_tbl[mask & 0x3f];
}


inline void 
Speakers::set(int _format, int _mask, int _sample_rate, sample_t _level, int _relation)
{
  format = _format;
  mask = _mask;
  sample_rate = _sample_rate;
  level = _level;
  relation = _relation;
}

inline void 
Speakers::set_error()
{
  format = FORMAT_UNKNOWN;
  mask = 0;
  sample_rate = 0;
  relation = NO_RELATION;
}

inline int 
Speakers::nch() const
{
  return mask_nch(mask);
}

inline bool 
Speakers::error() const
{
  return format == FORMAT_UNKNOWN;
}

inline bool 
Speakers::lfe() const
{
  return (mask & CH_MASK_LFE) != 0;
}

inline bool 
Speakers::is_spdif() const
{
  return format == FORMAT_SPDIF;
}

inline bool 
Speakers::is_pcm() const
{
  return (FORMAT_MASK(format) & FORMAT_MASK_PCM) != 0;
}

inline const int *
Speakers::order() const
{
  return ::mask_order(mask);
}

inline int 
Speakers::sample_size() const
{
  return ::sample_size(format);
}

inline bool
Speakers::operator ==(const Speakers &_spk) const
{
  return (format == _spk.format) &&
         (mask == _spk.mask) &&
         (sample_rate == _spk.sample_rate) &&
         (level == _spk.level) &&
         (relation == _spk.relation);
}

inline bool
Speakers::operator !=(const Speakers &_spk) const
{
  return (format != _spk.format) ||
         (mask != _spk.mask) ||
         (sample_rate != _spk.sample_rate) ||
         (level != _spk.level) ||
         (relation != _spk.relation);
}

inline const char *
Speakers::format_text() const
{
  switch (format)
  {
    case FORMAT_LINEAR:      return "Linear PCM";

    case FORMAT_PCM16:       return "PCM16";
    case FORMAT_PCM24:       return "PCM24";
    case FORMAT_PCM32:       return "PCM32";
    case FORMAT_PCMFLOAT:    return "PCM Float";

    case FORMAT_PCM16_LE:    return "PCM16 LE";
    case FORMAT_PCM24_LE:    return "PCM24 LE";
    case FORMAT_PCM32_LE:    return "PCM32 LE";
    case FORMAT_PCMFLOAT_LE: return "PCM Float LE";

    case FORMAT_PES:         return "MPEG Program Stream";
    case FORMAT_SPDIF:       return "SPDIF";

    case FORMAT_AC3:         return "AC3";
    case FORMAT_MPA:         return "MPEG Audio";
    case FORMAT_DTS:         return "DTS";

    default: return "Unknown";
  };
}

inline const char *
Speakers::mode_text() const
{
  switch (relation)
  {
    case RELATION_DOLBY:   return "Dolby Surround";
    case RELATION_DOLBY2:  return "Dolby ProLogic II";
    case RELATION_SUMDIFF: return "Sum-difference";
  }

  int nfront  = ((mask >> CH_L)  & 1) + ((mask >> CH_C)  & 1) + ((mask >> CH_R) & 1);
  int nrear   = ((mask >> CH_SL) & 1) + ((mask >> CH_SR) & 1);
  int code    = nfront * 100 + nrear * 10 + (lfe()? 1: 0);

  switch (code)
  {
    case 000: return "-";
    case 100: return "1/0 (mono)";
    case 200: return "2/0 (stereo)";
    case 300: return "3/0";
    case 210: return "2/1 (surround)";
    case 310: return "3/1 (surround)";
    case 220: return "2/2 (quadro)";
    case 320: return "3/2 (5 channels)";

    case 101: return "1/0+LFE";
    case 201: return "2/0+LFE (2.1)";
    case 301: return "3/0+LFE";
    case 211: return "2/1+LFE";
    case 311: return "3/1+LFE";
    case 221: return "2/2+LFE (4.1)";
    case 321: return "3/2+LFE (5.1)";
    default: return "non-standard format";
  }
}


#endif
