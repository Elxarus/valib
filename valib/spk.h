/*
  Speakers class
  Audio format definition class. Minimal set of audio parameters we absolutely
  have to know. Main purpose is to accompany audio data blocks.

  format - audio format. Formats are different only when we have to 
    distinguish them. For example, format definition have no bit-per-pixel
    parameter, but we have to distinguish PCM 16bit and PCM 32bit formats. So
    it is FORMAT_PCM16 and FORMAT_PCM32 formats. On other hand it is several
    different AC3 format variations: low-endian, big endian and sparse
    zero-padded format (IEC 61937 aka SPDIF/AC3 aka AC3/AudioCD). But AC3 
    parser can work properly with all this formats so we may not distinguish 
    them and it is only one FORMAT_AC3 format. 
    
    Format is applied to audio data but not to file format. It means that 
    stereo 16bit PCM .WAV and .RAW files will be characterized with the same 
    format.

    There are two special audio formats: FORMAT_UNKNOWN and FORMAT_LINEAR.

    FORMAT_UNKNOWN may have two purposes: 
    1) to indicate an error happen on previous processing stage, so output 
       data is erroneous. No other format parameters have meaning in this case.
    2) to indicate general binary data block with unknown data format. It
       may be used for automatic format detection. Some other format parameters
       may have meaning in this case (sample_rate for example, because it may 
       not be determined in some cases).

    FORMAT_LINEAR: most of internal processing is done with this format.

    Formats are divided into several format classes: PCM formats, compressed 
    formats, SPDIF'able formats and container formats.

    To specify set of formats format bitmasks are used. To convert format to 
    bitmask FORMAT_MASK(format) macro is used.

    Formats are defined by FORMAT_X constants. Format masks are defined by 
    FORMAT_MASK_X constants. Format classes are defined by FORMAT_CLASS_X 
    constants.
    
  mask - channels bitmask. Defines a set of existing channels. Number of bits
    set defines number of channels so class have no separate field to avoid 
    ambiguity. But is is nch() function that returns number of channels for
    current mask. 
    
    Format and mask also define channel ordering. Different formats may have 
    different channel ordering. Channel order for FORMAT_LINEAR is called 
    'standard channel order'. In this case channel number defined by CH_X 
    constant have meaning of channel priority (0 - highest priority). It 
    means that channels with small channel number will be placed before 
    channels with big channels numbers.

    For compressed formats that contain channel configuration in the bitstream
    so parser may always know it, it is acceptable not to specify mask field at
    the input of parser. But at output parser must specify correct mask.
    
  sample_rate - sampling rate. this is fundamental parameter we always have to
    know.

    For compressed formats that contain sample rate in the bitstream so parser
    may always know it, it is acceptable not to specify sample_rate field at
    the input of parser. But at output parser must specify correct sample rate.
    
  relation - relation between channels. Format and mask may not always fully 
    define audio format. Audio channels may have an internal relation between 
    each other. For example sum-difference when one channel have meaning of 
    mono channel and other is interchannel difference. Other example is 
    Dolby-encoded audio source. It is independent audio characteristic and 
    required to take it into account.

    For compressed formats that contain relation in the bitstream so decoder
    may always know it, it is acceptable not to specify relation field at
    the input of decoder. But at output decoder must specify correct relation.
    
  level - absolute value for 0dB level. Generally depends on format, i.e.
    for PCM16 format it is 32767.0, so I'm thinking about to get rid of this 
    parameter. Now it is used to pre-scale data.
*/

#ifndef SPK_H
#define SPK_H

#include "defs.h"

///////////////////////////////////////////////////////////////////////////////
// Formats
///////////////////////////////////////////////////////////////////////////////

// special-purpose formats
#define FORMAT_UNKNOWN     0
#define FORMAT_LINEAR      1

// PCM big-endian formats
#define FORMAT_PCM16       2
#define FORMAT_PCM24       3
#define FORMAT_PCM32       4
#define FORMAT_PCMFLOAT    5

// PCM low-endian formats
#define FORMAT_PCM16_LE    6
#define FORMAT_PCM24_LE    7
#define FORMAT_PCM32_LE    8
#define FORMAT_PCMFLOAT_LE 9

// container formats
#define FORMAT_PES        10 // MPEG1/2 Program Elementary Stream
#define FORMAT_SPDIF      11 // IEC 61937 stream

// compressed formats
#define FORMAT_AC3        12
#define FORMAT_MPA        13
#define FORMAT_DTS        14
#define FORMAT_AAC        15
#define FORMAT_OGG        16

///////////////////////////////////////////////////////////////////////////////
// Format masks
///////////////////////////////////////////////////////////////////////////////

// macro to convert format number to format mask
#define FORMAT_MASK(format)  (1 << format)

// special-purpose format masks
#define FORMAT_MASK_UNKNOWN      FORMAT_MASK(FORMAT_UNKNOWN)
#define FORMAT_MASK_LINEAR       FORMAT_MASK(FORMAT_LINEAR)

// PCM big-endian format masks
#define FORMAT_MASK_PCM16        FORMAT_MASK(FORMAT_PCM16)
#define FORMAT_MASK_PCM24        FORMAT_MASK(FORMAT_PCM24)
#define FORMAT_MASK_PCM32        FORMAT_MASK(FORMAT_PCM32)
#define FORMAT_MASK_PCM_FLOAT    FORMAT_MASK(FORMAT_PCM_FLOAT)

// PCM low-endian format masks
#define FORMAT_MASK_PCM16_LE     FORMAT_MASK(FORMAT_PCM16_LE)
#define FORMAT_MASK_PCM24_LE     FORMAT_MASK(FORMAT_PCM24_LE)
#define FORMAT_MASK_PCM32_LE     FORMAT_MASK(FORMAT_PCM32_LE)
#define FORMAT_MASK_PCM_FLOAT_LE FORMAT_MASK(FORMAT_PCM_FLOAT_LE)

// container format masks
#define FORMAT_MASK_PES          FORMAT_MASK(FORMAT_PES)
#define FORMAT_MASK_SPDIF        FORMAT_MASK(FORMAT_SPDIF)

// compressed format masks
#define FORMAT_MASK_AC3          FORMAT_MASK(FORMAT_AC3)
#define FORMAT_MASK_MPA          FORMAT_MASK(FORMAT_MPA)
#define FORMAT_MASK_DTS          FORMAT_MASK(FORMAT_DTS)
#define FORMAT_MASK_AAC          FORMAT_MASK(FORMAT_AAC)
#define FORMAT_MASK_OGG          FORMAT_MASK(FORMAT_OGG)

///////////////////////////////////////////////////////////////////////////////
// Format classes (bitmasks)
///////////////////////////////////////////////////////////////////////////////

#define FORMAT_CLASS_PCM         0x003fc
#define FORMAT_CLASS_CONTAINER   0x00c00
#define FORMAT_CLASS_COMPRESSED  0x1f000
#define FORMAT_CLASS_SPDIFABLE   0x07000

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
#define CH_CH1  0  // Channel 1 in Dual mono mode
#define CH_CH2  2  // Channel 2 in Dual mono mode
#define CH_S    3  // Surround channel for x/1 modes

///////////////////////////////////////////////////////////////////////////////
// Channel masks
// used as channel presence flag in a mask definition
///////////////////////////////////////////////////////////////////////////////

// macro to convert channel number to channel mask
#define CH_MASK(ch)  (1 << (ch & 0x1f))

// channel masks
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

///////////////////////////////////////////////////////////////////////////////
// Common channel configs
///////////////////////////////////////////////////////////////////////////////

#define MODE_UNDEFINED 0
#define MODE_1_0     (CH_MASK_M)
#define MODE_2_0     (CH_MASK_L | CH_MASK_R)
#define MODE_3_0     (CH_MASK_L | CH_MASK_C  | CH_MASK_R)
#define MODE_2_1     (MODE_2_0  | CH_MASK_S)
#define MODE_3_1     (MODE_3_0  | CH_MASK_S)
#define MODE_2_2     (MODE_2_0  | CH_MASK_SL | CH_MASK_SR)
#define MODE_3_2     (MODE_3_0  | CH_MASK_SL | CH_MASK_SR)
#define MODE_1_0_LFE (CH_MASK_M | CH_MASK_LFE)
#define MODE_2_0_LFE (CH_MASK_L | CH_MASK_R  | CH_MASK_LFE)
#define MODE_3_0_LFE (CH_MASK_L | CH_MASK_C  | CH_MASK_R  | CH_MASK_LFE)
#define MODE_2_1_LFE (MODE_2_0  | CH_MASK_S  | CH_MASK_LFE)
#define MODE_3_1_LFE (MODE_3_0  | CH_MASK_S  | CH_MASK_LFE)
#define MODE_2_2_LFE (MODE_2_0  | CH_MASK_SL | CH_MASK_SR | CH_MASK_LFE)
#define MODE_3_2_LFE (MODE_3_0  | CH_MASK_SL | CH_MASK_SR | CH_MASK_LFE)

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

extern const int sample_size_tbl[32];
extern const int mask_nch_tbl[64];
extern const int mask_order_tbl[64][6];
extern const char *mode_text[64];

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
  return (FORMAT_MASK(format) & FORMAT_CLASS_PCM) != 0;
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
    case FORMAT_AAC:         return "AAC";
    case FORMAT_OGG:         return "OGG";

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

  return ::mode_text[mask];
}


#endif
