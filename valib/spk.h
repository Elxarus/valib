/*
  ==Speakers class==

  Audio format definition class. Minimal set of audio parameters we absolutely
  have to know. Main purpose is to accompany audio data blocks.

  format - audio format. Formats are different only when we have to 
    distinguish them. For example, format definition have no bit-per-pixel
    parameter, but we have to distinguish PCM 16bit and PCM 32bit formats. So
    it is FORMAT_PCM16 and FORMAT_PCM32 formats. On other hand it is several
    different AC3 format variations: big-endian, low-endian and sparse
    zero-padded format (IEC 61937 aka SPDIF/AC3 aka AC3/AudioCD). But AC3 
    parser can work properly with all this formats so we may not distinguish 
    them and it is only one FORMAT_AC3 format. 

    Format is applied to audio data but not to file format. It means that 
    stereo 16bit PCM .WAV and .RAW files will be characterized with the same 
    format.

    There are three special audio formats: 
    * FORMAT_UNKNOWN
    * FORMAT_LINEAR
    * FORMAT_RAWDATA

    FORMAT_UNKNOWN may have two purposes: 
    1) indicate an error happen on previous processing stage, so output data
       is erroneous.
    2) indicate uninitialized state of the filter.
    No format parameters have meaning for unknown format.

    FORMAT_RAWDATA: indicates general binary data block with unknown data
    format. It may be used for automatic format detection. Some other format
    parameters may have meaning in this case (sample_rate for example, because
    it may not be determined in some cases).

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
    ambiguity. But it is nch() function that returns number of channels for
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
    for PCM16 format it is 32767.0, so I think about to get rid of this 
    parameter. Now it is used to pre-scale data.

  ==Channels==

  NCHANNELS - number of channels passed to processing functions
  CH_NAMES - number of channel names.

  ===Channel name===

  *Channel name* is the channel destination, like 'Front Left' or 'Right
  Surround'. Note, that we can configure the channel by name. For example, set
  delays for left and right surrounds. Also note, that it may be more
  destinations, that channels we can simultaneously process (number of channels
  passed to processing functions). I.e. NCHANNELS <= CH_NAMES.

  Channel name is an integer from 0 to CH_NAMES-1.

  Constants for channel names:
  CH_{NAME}

  Where {NAME} is one of:

  | L   |- Front left
  | C   |- Front center
  | R   |- Front right
  | SL  |- Surround left
  | SR  |- Surround right
  | LFE |- LFE
  | CL  |- Front left center
  | CR  |- Front right center
  | BL  |- Back left
  | BC  |- Back center
  | BR  |- Back right

  ===Channel mask===

  A set of channel names forms *speaker configuration*, or *mode*. Stereo
  config has 2 channels: left and right. 5.1 config has 6 channels, 3 front,
  2 surround and 1 low-frequency channel.

  Speaker configuration is represented as an integer value where each bit
  represents some channel, bit mask, or simply *mask*. Term *channel mask*
  usually points to a mask with a single bit set for a certain channel.

  Note, that we can process only NCHANNELS at once. Therefore, channel mask
  cannot have more than NCHANNELS bits set (with some exceptions).

  Constants for channel masks:
  CH_MASK_{NAME}

  Convert channel name to channel mask:
  mask = CH_MASK(ch_name1) | CH_MASK(ch_name2)

  Speaker configuration constants:
  MODE_{MODE}

  ===Channel order===

  *Channel order* defines weight of each channel name. Different sources has
  different channel orders.

  An array of channel names is used for order:
  typedef int order_t[CH_NAMES];

  For example, internally this library uses *standard channel order*:
  CH_L, CH_C, CH_R, CH_SL, CH_SR, CH_LFE, CH_CL, CH_CR, CH_BL, CH_BC, CH_BR

  Windows has the following order:
  CH_L, CH_R, CH_C, CH_LFE, CH_BL, CH_BR, CH_CL, CH_CR, CH_BC, CH_SL, CH_SR

  AC3 order:
  CH_L, CH_C, CH_R, CH_SL, CH_SR, CH_LFE, CH_NONE, CH_NONE ...

  Note, that in the last case we have less channels because AC3 format does not
  know about others. But order array must have a fixed size. To fill the rest
  CH_NONE constant is used.

  Note that each channel name must be unique in the array and array must be
  compact. The following orders are prohibited:

  CH_L, CH_R, CH_L, ...    // CH_L is not unique
  CH_L, CH_NONE, CH_R, ... // gap in the middle

  Each specific channel configuration of each specific source has its own
  order. For example, 4.1 config has different orders in Windows and AC3:

  CH_L, CH_R, CH_SL, CH_SR, CH_LFE // AC3 order
  CH_L, CH_R, CH_LFE, CH_SL, CH_SR // Windows order

  But we can decide the specific order from the 'global' order for this source
  and channel mask for this configuration, by 'throuwing out' unused channels.

  Internally, library uses only one, standard order. You can get order for any
  configuration by calling Speakers::get_order().

  ===Channel index===

  Channel order is an array of channel names. Zero-based index in this array
  is the *channel index*, or simply 'channel'. The same index may point to the
  different channels in different modes. For example, index 1 may point to the
  right channel in stereo mode or center channel in 5.1 mode.

  Note, that at processing time channel index is in range form 0 to NCHANNELS.

  ===Channel settings===

  Some filters provides individual channel settings (delay, equalizer, gain,
  etc). This settings are applied to the certain *destination* and therefore
  should be contained in an array of CH_NAMES elements in standard order.

  The standard order channel name *is its own index*, so we can address left
  channel in the settings array as:

  sample_t gains[CH_NAMES];
  left_gain = gains[CH_L];

  But at the processing time we deal with custom order. Therefore, to retrieve
  a parameter from an array we have to know the channel name by its index. We
  can do this with an order array built for the given configuration:

  sample_t gains[CH_NAMES];

  function process(Speakers spk, samples_t sampels)
  {
    order_t order;
    spk.get_order(order);
    for (int ch = 0; ch < spk.nch(); ch++) // ch is index
    {
      int ch_name = order[ch];
      double gain = gains[ch_name];
      ...
    }
    ...
  }
*/

#ifndef VALIB_SPK_H
#define VALIB_SPK_H

#include "defs.h"
#include <string>
#include <boost/shared_ptr.hpp>

using std::string;

///////////////////////////////////////////////////////////////////////////////
// Formats
///////////////////////////////////////////////////////////////////////////////

// special-purpose formats
#define FORMAT_UNKNOWN     (-1)
#define FORMAT_RAWDATA     0
#define FORMAT_LINEAR      1

// PCM low-endian formats
#define FORMAT_PCM16       2
#define FORMAT_PCM24       3
#define FORMAT_PCM32       4

// PCM big-endian formats
#define FORMAT_PCM16_BE    5
#define FORMAT_PCM24_BE    6
#define FORMAT_PCM32_BE    7

// PCM floating-point
#define FORMAT_PCMFLOAT    8
#define FORMAT_PCMDOUBLE   9

// container formats
#define FORMAT_PES        10 // MPEG1/2 Program Elementary Stream
#define FORMAT_SPDIF      11 // IEC 61937 stream

// compressed spdifable formats
#define FORMAT_MPA        12
#define FORMAT_AC3        13
#define FORMAT_DTS        14
#define FORMAT_EAC3       15
#define FORMAT_AC3_EAC3   16 // AC3 or EAC3 stream
                             // Later: include Dolby TrueHD and rename to
                             // FORMAT_DOLBY
// DVD LPCM
// Note: the sample size for this formats is doubled because
// LPCM samples are packed into blocks of 2 samples.
#define FORMAT_LPCM20     17
#define FORMAT_LPCM24     18

// AAC
#define FORMAT_AAC_FRAME  19
#define FORMAT_AAC_ADTS   20

#define FORMAT_FLAC       21
#define FORMAT_VORBIS     22

///////////////////////////////////////////////////////////////////////////////
// Format masks
///////////////////////////////////////////////////////////////////////////////

// macro to convert format number to format mask
#define FORMAT_MASK(format)  (1 << (format))

// special-purpose format masks
#define FORMAT_MASK_RAWDATA      FORMAT_MASK(FORMAT_RAWDATA)
#define FORMAT_MASK_LINEAR       FORMAT_MASK(FORMAT_LINEAR)

// PCM low-endian format masks
#define FORMAT_MASK_PCM16        FORMAT_MASK(FORMAT_PCM16)
#define FORMAT_MASK_PCM24        FORMAT_MASK(FORMAT_PCM24)
#define FORMAT_MASK_PCM32        FORMAT_MASK(FORMAT_PCM32)

// PCM big-endian format masks
#define FORMAT_MASK_PCM16_BE     FORMAT_MASK(FORMAT_PCM16_BE)
#define FORMAT_MASK_PCM24_BE     FORMAT_MASK(FORMAT_PCM24_BE)
#define FORMAT_MASK_PCM32_BE     FORMAT_MASK(FORMAT_PCM32_BE)

// PCM floating-point format masks
#define FORMAT_MASK_PCMFLOAT     FORMAT_MASK(FORMAT_PCMFLOAT)
#define FORMAT_MASK_PCMDOUBLE    FORMAT_MASK(FORMAT_PCMDOUBLE)

// container format masks
#define FORMAT_MASK_PES          FORMAT_MASK(FORMAT_PES)
#define FORMAT_MASK_SPDIF        FORMAT_MASK(FORMAT_SPDIF)

// compressed format masks
#define FORMAT_MASK_AC3          FORMAT_MASK(FORMAT_AC3)
#define FORMAT_MASK_MPA          FORMAT_MASK(FORMAT_MPA)
#define FORMAT_MASK_DTS          FORMAT_MASK(FORMAT_DTS)
#define FORMAT_MASK_FLAC         FORMAT_MASK(FORMAT_FLAC)

// DVD LPCM
#define FORMAT_MASK_LPCM20       FORMAT_MASK(FORMAT_LPCM20)
#define FORMAT_MASK_LPCM24       FORMAT_MASK(FORMAT_LPCM24)

///////////////////////////////////////////////////////////////////////////////
// Format classes (bitmasks)
///////////////////////////////////////////////////////////////////////////////

#define FORMAT_CLASS_PCM_LE      (FORMAT_MASK_PCM16    | FORMAT_MASK_PCM24    | FORMAT_MASK_PCM32)
#define FORMAT_CLASS_PCM_BE      (FORMAT_MASK_PCM16_BE | FORMAT_MASK_PCM24_BE | FORMAT_MASK_PCM32_BE)
#define FORMAT_CLASS_PCM_FP      (FORMAT_MASK_PCMFLOAT | FORMAT_MASK_PCMDOUBLE)
#define FORMAT_CLASS_PCM         (FORMAT_CLASS_PCM_LE  | FORMAT_CLASS_PCM_BE  | FORMAT_CLASS_PCM_FP)
#define FORMAT_CLASS_LPCM        (FORMAT_MASK_LPCM20   | FORMAT_MASK_LPCM24)
#define FORMAT_CLASS_CONTAINER   (FORMAT_MASK_PES | FORMAT_MASK_SPDIF)
#define FORMAT_CLASS_SPDIFABLE   (FORMAT_MASK_MPA | FORMAT_MASK_AC3 | FORMAT_MASK_DTS)
#define FORMAT_CLASS_COMPRESSED  (FORMAT_MASK_MPA | FORMAT_MASK_AC3 | FORMAT_MASK_DTS)

///////////////////////////////////////////////////////////////////////////////
// Channel numbers (that also define 'standard' channel order)
// may used as index in arrays
///////////////////////////////////////////////////////////////////////////////

#define CH_L    0  // Front left channel
#define CH_C    1  // Front center channel
#define CH_R    2  // Front right channel
#define CH_SL   3  // Surround left channel
#define CH_SR   4  // Surround right channel
#define CH_LFE  5  // LFE channel
#define CH_CL   6  // Front left center
#define CH_CR   7  // Front right center
#define CH_BL   8  // Back left
#define CH_BC   9  // Back center
#define CH_BR   10 // Back right
#define CH_NONE -1 // indicates that channel is not used in channel order

// synonyms
#define CH_M    CH_C   // Mono channel
#define CH_S    CH_BC  // Single surround channel

///////////////////////////////////////////////////////////////////////////////
// Channel masks
// used as channel presence flag in a mask definition
///////////////////////////////////////////////////////////////////////////////

// macro to convert channel number to channel mask
#define CH_MASK(ch)  (1 << ch)

// channel masks
#define CH_MASK_L    1     // Front left
#define CH_MASK_C    2     // Front center
#define CH_MASK_R    4     // Front right
#define CH_MASK_SL   8     // Surround left
#define CH_MASK_SR   16    // Surround right
#define CH_MASK_LFE  32    // LFE
#define CH_MASK_CL   64    // Front left center
#define CH_MASK_CR   128   // Front right center
#define CH_MASK_BL   256   // Back left
#define CH_MASK_BC   512   // Back center
#define CH_MASK_BR   1024  // Back right

// channel pairs
#define CH_MASK_L_R   (CH_MASK_L  | CH_MASK_R)
#define CH_MASK_SL_SR (CH_MASK_SL | CH_MASK_SR)
#define CH_MASK_BL_BR (CH_MASK_BL | CH_MASK_BR)
#define CH_MASK_CL_CR (CH_MASK_CL | CH_MASK_CR)

// synonyms
#define CH_MASK_M    CH_MASK_C  // Mono channel
#define CH_MASK_S    CH_MASK_BC // Single surround channel

// Mask for all channels available
#define CH_MASK_ALL  0x3ff

///////////////////////////////////////////////////////////////////////////////
// Common channel configs
// Name has 2 forms:
// * MODE_A_B[_LFE]
// * MODE_A_B_C[_LFE]
// Where
//   A - number of front channels (1, 3, 5)
//   B - number of surround side channels (0, 1, 2)
//   C - number of surround back channels (1, 2, 3)
//   LFE - optional low-frequency channel
//
// Note, that back center and single surround is the same channel. So
// MODE_A_0_1 and MODE_A_1 is the same layout and only the last form is used.
///////////////////////////////////////////////////////////////////////////////

#define MODE_UNDEFINED 0
#define MODE_1_0     (CH_MASK_M)
#define MODE_2_0     (CH_MASK_L | CH_MASK_R)
#define MODE_3_0     (CH_MASK_L | CH_MASK_C  | CH_MASK_R)
#define MODE_5_0     (MODE_3_0  | CH_MASK_CL | CH_MASK_CR)
#define MODE_2_1     (MODE_2_0  | CH_MASK_S)
#define MODE_3_1     (MODE_3_0  | CH_MASK_S)
#define MODE_2_2     (MODE_2_0  | CH_MASK_SL | CH_MASK_SR)
#define MODE_3_2     (MODE_3_0  | CH_MASK_SL | CH_MASK_SR)
#define MODE_5_2     (MODE_3_2  | CH_MASK_CL | CH_MASK_CR)
#define MODE_1_0_LFE (MODE_1_0  | CH_MASK_LFE)
#define MODE_2_0_LFE (MODE_2_0  | CH_MASK_LFE)
#define MODE_3_0_LFE (MODE_3_0  | CH_MASK_LFE)
#define MODE_5_0_LFE (MODE_5_0  | CH_MASK_LFE)
#define MODE_2_1_LFE (MODE_2_1  | CH_MASK_LFE)
#define MODE_3_1_LFE (MODE_3_1  | CH_MASK_LFE)
#define MODE_2_2_LFE (MODE_2_2  | CH_MASK_LFE)
#define MODE_3_2_LFE (MODE_3_2  | CH_MASK_LFE)
#define MODE_5_2_LFE (MODE_5_2  | CH_MASK_LFE)

#define MODE_2_0_2     (MODE_2_0   | CH_MASK_BL | CH_MASK_BR)
#define MODE_3_0_2     (MODE_3_0   | CH_MASK_BL | CH_MASK_BR)
#define MODE_5_0_2     (MODE_5_0   | CH_MASK_BL | CH_MASK_BR)
#define MODE_3_2_1     (MODE_3_2   | CH_MASK_BC)
#define MODE_3_2_2     (MODE_3_2   | CH_MASK_BL | CH_MASK_BR)
#define MODE_2_0_2_LFE (MODE_2_0_2 | CH_MASK_LFE)
#define MODE_3_0_2_LFE (MODE_3_0_2 | CH_MASK_LFE)
#define MODE_5_0_2_LFE (MODE_5_0_2 | CH_MASK_LFE)
#define MODE_3_2_1_LFE (MODE_3_2_1 | CH_MASK_LFE)
#define MODE_3_2_2_LFE (MODE_3_2_2 | CH_MASK_LFE)

// synonyms
#define MODE_MONO    MODE_1_0
#define MODE_STEREO  MODE_2_0
#define MODE_QUADRO  MODE_2_2
#define MODE_4_1     MODE_2_2_LFE
#define MODE_5_1     MODE_3_2_LFE
#define MODE_6_1     MODE_3_2_1_LFE
#define MODE_7_1     MODE_3_2_2_LFE


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

  boost::shared_ptr<uint8_t> format_data;
  size_t data_size;     // format data size

  Speakers()
  {
    format = FORMAT_UNKNOWN;
    mask = 0;
    sample_rate = 0;
    relation = NO_RELATION;
    level = 1.0;
    data_size = 0;
  }

  Speakers(int _format, int _mask, int _sample_rate, sample_t _level = -1, int _relation = NO_RELATION)
  {
    format = _format;
    mask = _mask;
    sample_rate = _sample_rate;
    level = _level;
    if (level < 0) switch (format)
    {
      // See filters/convert_func.cpp for notes about fractional levels
      case FORMAT_PCM16: case FORMAT_PCM16_BE: level = 32767.5; break;
      case FORMAT_PCM24: case FORMAT_PCM24_BE: level = 8388607.5; break;
      case FORMAT_PCM32: case FORMAT_PCM32_BE: level = 2147483647.5; break;
      case FORMAT_LPCM20: level = 524288.5; break;
      case FORMAT_LPCM24: level = 8388607.5; break;
      default: level = 1.0;
    }
    relation = _relation;

    data_size = 0;
  }

  void set_format_data(uint8_t *format_data_, size_t data_size_);

  inline bool is_unknown() const;
  inline bool is_linear() const;
  inline bool is_rawdata() const;

  inline bool is_pcm() const;
  inline bool is_floating_point() const;
  inline bool is_spdif() const;

  inline int  nch() const;
  inline bool lfe() const;

  inline int  sample_size() const;

  inline bool operator ==(const Speakers &spk) const;
  inline bool operator !=(const Speakers &spk) const;

  inline void get_order(order_t order) const;

  const char *format_text() const;
  const char *mode_text() const;
  string print() const;
};

///////////////////////////////////////////////////////////////////////////////
// Constants for common audio formats
///////////////////////////////////////////////////////////////////////////////

extern const Speakers spk_unknown;
extern const Speakers spk_rawdata;

///////////////////////////////////////////////////////////////////////////////
// Constants for common channel orders
///////////////////////////////////////////////////////////////////////////////

extern const int std_order[CH_NAMES];
extern const int win_order[CH_NAMES];

///////////////////////////////////////////////////////////////////////////////
// samples_t
// Block of pointers to sample buffers for each channel for linear format.
///////////////////////////////////////////////////////////////////////////////

struct samples_t
{
  sample_t *samples[NCHANNELS];

  inline sample_t *&operator [](unsigned ch)       { return samples[ch]; }
  inline sample_t  *operator [](unsigned ch) const { return samples[ch]; }

  inline samples_t &operator +=(int n);
  inline samples_t &operator -=(int n);
  inline samples_t &operator +=(size_t n);
  inline samples_t &operator -=(size_t n);

  inline samples_t operator +(int n)    const { samples_t s(*this); return s += n; }
  inline samples_t operator -(int n)    const { samples_t s(*this); return s -= n; }
  inline samples_t operator +(size_t n) const { samples_t s(*this); return s += n; }
  inline samples_t operator -(size_t n) const { samples_t s(*this); return s -= n; }

  inline bool operator ==(const samples_t &other) const;
  inline bool operator !=(const samples_t &other) const;

  inline samples_t &zero();

  void reorder_to_std(Speakers spk, const order_t order);
  void reorder_from_std(Speakers spk, const order_t order);
  void reorder(Speakers spk, const order_t input_order, const order_t output_order);

};

///////////////////////////////////////////////////////////////////////////////
// Samples functions
///////////////////////////////////////////////////////////////////////////////

void zero_samples(sample_t *s, size_t size);
void zero_samples(samples_t s, int nch, size_t size);

void zero_samples(sample_t *s, size_t offset, size_t size);
void zero_samples(samples_t s, size_t offset, int nch, size_t size);

void copy_samples(sample_t *dst, const sample_t *src, size_t size);
void copy_samples(samples_t dst, const samples_t src, int nch, size_t size);

void copy_samples(sample_t *dst, size_t dst_offset, const sample_t *src, size_t src_offset, size_t size);
void copy_samples(samples_t dst, size_t dst_offset, const samples_t src, size_t src_offset, int nch, size_t size);

void move_samples(sample_t *dst, const sample_t *src, size_t size);
void move_samples(samples_t dst, const samples_t src, int nch, size_t size);

void move_samples(sample_t *dst, size_t dst_offset, const sample_t *src, size_t src_offset, size_t size);
void move_samples(samples_t dst, size_t dst_offset, const samples_t src, size_t src_offset, int nch, size_t size);

void gain_samples(sample_t gain, sample_t *s, size_t size);
void gain_samples(sample_t gain, samples_t s, int nch, size_t size);

void gain_samples(sample_t gain, sample_t *s, size_t offset, size_t size);
void gain_samples(sample_t gain, samples_t s, size_t offset, int nch, size_t size);

void sum_samples(sample_t *dst, const sample_t *src, size_t size);
void mul_samples(sample_t *dst, const sample_t *src, size_t size);

sample_t max_samples(sample_t max, const sample_t *s, size_t size);
sample_t peak_diff(const sample_t *s1, const sample_t *s2, size_t size);
sample_t rms_diff(const sample_t *s1, const sample_t *s2, size_t size);

///////////////////////////////////////////////////////////////////////////////
// Format and channel functions
///////////////////////////////////////////////////////////////////////////////

int sample_size(int format);
void channel_order(int mask, int order[CH_NAMES]);

const char *format_text(int format);
const char *ch_name_short(int ch_name);
const char *ch_name_long(int ch_name);
int nch2mask(int nch);

inline int mask_nch(int mask)
{
  // Bit counting taken from FXTBook
  int nch = mask;
  nch  = ((nch >> 1) & 0x55555555UL) + (nch & 0x55555555UL); // 0-2 in 2 bits
  nch  = ((nch >> 2) & 0x33333333UL) + (nch & 0x33333333UL); // 0-4 in 4 bits
  nch  = ((nch >> 4) + nch) & 0x0f0f0f0fUL;                  // 0-8 in 4 bits
  nch += nch >> 8;    // 0-16 in 8 bits
  nch += nch >> 16;   // 0-32 in 8 bits
  return nch & 0xff;
}

///////////////////////////////////////////////////////////////////////////////
// Speakers inlines
///////////////////////////////////////////////////////////////////////////////

inline bool Speakers::is_unknown() const
{ return format == FORMAT_UNKNOWN; }

inline bool Speakers::is_linear() const
{ return format == FORMAT_LINEAR; }

inline bool Speakers::is_rawdata() const
{ return format != FORMAT_LINEAR; }

inline bool Speakers::is_pcm() const
{ return (FORMAT_MASK(format) & FORMAT_CLASS_PCM) != 0; }

inline bool Speakers::is_floating_point() const
{ return (FORMAT_MASK(format) & FORMAT_CLASS_PCM_FP) != 0; }

inline bool Speakers::is_spdif() const
{ return format == FORMAT_SPDIF; }

inline int Speakers::nch() const
{ return mask_nch(mask); }

inline bool 
Speakers::lfe() const
{ return (mask & CH_MASK_LFE) != 0; }

inline int
Speakers::sample_size() const
{ return ::sample_size(format); }

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

inline void
Speakers::get_order(order_t order) const
{ channel_order(mask, order); }

///////////////////////////////////////////////////////////////////////////////
// samples_t inlines
///////////////////////////////////////////////////////////////////////////////

inline samples_t &
samples_t::operator +=(int _n)
{
  for (int i = 0; i < NCHANNELS; i++)
    if (samples[i])
      samples[i] += _n;
  return *this;
}

inline samples_t &
samples_t::operator -=(int _n)
{
  for (int i = 0; i < NCHANNELS; i++)
    if (samples[i])
      samples[i] -= _n;
  return *this;
}

inline samples_t &
samples_t::operator +=(size_t _n)
{
  for (int i = 0; i < NCHANNELS; i++)
    if (samples[i])
      samples[i] += _n;
  return *this;
}

inline samples_t &
samples_t::operator -=(size_t _n)
{
  for (int i = 0; i < NCHANNELS; i++)
    if (samples[i])
      samples[i] -= _n;
  return *this;
}

inline samples_t &
samples_t::zero()
{
  for (int i = 0; i < NCHANNELS; i++)
    samples[i] = 0;
  return *this;
}

inline bool
samples_t::operator ==(const samples_t &other) const
{
  for (int i = 0; i < NCHANNELS; i++)
    if (samples[i] != other.samples[i])
      return false;
  return true;
}
inline bool
samples_t::operator !=(const samples_t &other) const
{
  return !(*this == other);
}

#endif
