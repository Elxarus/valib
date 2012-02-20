#include <boost/lexical_cast.hpp>
#include <math.h>
#include "spk.h"

///////////////////////////////////////////////////////////////////////////////
// Constants for common audio formats
///////////////////////////////////////////////////////////////////////////////

extern const Speakers spk_unknown = Speakers(FORMAT_UNKNOWN, 0, 0, 0, 0);
extern const Speakers spk_rawdata = Speakers(FORMAT_RAWDATA, 0, 0, 0, 0);

///////////////////////////////////////////////////////////////////////////////
// Constants for common channel orders
///////////////////////////////////////////////////////////////////////////////

extern const int std_order[CH_NAMES] = 
{ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };

extern const int win_order[CH_NAMES] = 
{ CH_L, CH_R, CH_C, CH_LFE, CH_BL, CH_BR, CH_CL, CH_CR, CH_BC, CH_SL, CH_SR };

///////////////////////////////////////////////////////////////////////////////
// Tables for Speakers class
///////////////////////////////////////////////////////////////////////////////

static const struct { int mask; const char *text; } mode_map[] =
{
  { 0,              "-" },
  { MODE_1_0,       "Mono" },
  { MODE_2_0,       "Stereo" },
  { MODE_3_0,       "3/0" },
  { MODE_2_1,       "2/1" },
  { MODE_3_1,       "3/1" },
  { MODE_2_2,       "Quadro" },
  { MODE_3_2,       "5.0" },
  { MODE_3_0_2,     "5.0 (back)" },
  { MODE_1_0_LFE,   "1.1" },
  { MODE_2_0_LFE,   "2.1" },
  { MODE_3_0_LFE,   "3/0.1" },
  { MODE_2_1_LFE,   "2/1.1" },
  { MODE_3_1_LFE,   "3/1.1" },
  { MODE_2_2_LFE,   "4.1" },
  { MODE_3_2_LFE,   "5.1" },
  { MODE_3_0_2_LFE, "5.1 (back)" },
  { MODE_3_2_1,     "6.0" },
  { MODE_3_2_2,     "7.0 (3/2/2)" },
  { MODE_5_2,       "7.0 (5/2)" },
  { MODE_3_2_1_LFE, "6.1" },
  { MODE_3_2_2_LFE, "7.1 (3/2/2.1)" },
  { MODE_5_2_LFE,   "7.1 (5/2.1)" },
};

static const char *custom_modes[] =
{
  "Custom 0ch", "Custom 1ch", "Custom 2ch", "Custom 3ch",
  "Custom 4ch", "Custom 5ch", "Custom 6ch", "Custom 7ch",
  "Custom 8ch", "Custom 9ch", "Custom 10ch", "Custom 11ch"
};

void 
Speakers::set_format_data(uint8_t *format_data_, size_t data_size_)
{
  if (format_data_ && data_size_)
  {
    format_data = boost::shared_ptr<uint8_t>(new uint8_t[data_size_]);
    memcpy(format_data.get(), format_data_, data_size_);
    data_size = data_size_;
  }
  else
  {
    format_data.reset();
    data_size = 0;
  }
}

const char *
Speakers::format_text() const
{
  switch (format)
  {
    case FORMAT_RAWDATA:     return "Raw data";
    case FORMAT_LINEAR:      return "Linear PCM";

    case FORMAT_PCM16:       return "PCM16";
    case FORMAT_PCM24:       return "PCM24";
    case FORMAT_PCM32:       return "PCM32";

    case FORMAT_PCM16_BE:    return "PCM16 BE";
    case FORMAT_PCM24_BE:    return "PCM24 BE";
    case FORMAT_PCM32_BE:    return "PCM32 BE";

    case FORMAT_PCMFLOAT:    return "PCM Float";
    case FORMAT_PCMDOUBLE:   return "PCM Double";

    case FORMAT_PES:         return "MPEG Program Stream";
    case FORMAT_SPDIF:       return "SPDIF";

    case FORMAT_AAC_FRAME:   return "AAC";
    case FORMAT_AC3:         return "AC3";
    case FORMAT_EAC3:        return "EAC3";
    case FORMAT_AC3_EAC3:    return "AC3/EAC3";
    case FORMAT_MPA:         return "MPEG Audio";
    case FORMAT_DTS:         return "DTS";
    case FORMAT_FLAC:        return "Flac";

    case FORMAT_LPCM20:      return "LPCM 20bit";
    case FORMAT_LPCM24:      return "LPCM 24bit";

    default: return "Unknown";
  };
}

const char *
Speakers::mode_text() const
{
  switch (relation)
  {
    case RELATION_DOLBY:   return "Dolby Surround";
    case RELATION_DOLBY2:  return "Dolby ProLogic II";
    case RELATION_SUMDIFF: return "Sum-difference";
  }

  for (int i = 0; i < array_size(mode_map); i++)
    if (mode_map[i].mask == mask)
      return mode_map[i].text;

  int nch = mask_nch(mask);
  if (nch < array_size(custom_modes))
    return custom_modes[nch];

  return "Custom";
}

string
Speakers::print() const
{
  static const string space = " ";
  return string(format_text()) + space +
         string(mode_text()) + space +
         boost::lexical_cast<string>(sample_rate);
}


///////////////////////////////////////////////////////////////////////////////
// samples_t
///////////////////////////////////////////////////////////////////////////////

void
samples_t::reorder_to_std(Speakers _spk, const order_t _order)
{
  int i, ch;
  int mask = _spk.mask;
  assert(_spk.nch() <= NCHANNELS);

  sample_t *tmp[CH_NAMES];

  ch = 0;
  for (i = 0; i < CH_NAMES; i++)
    if (mask & CH_MASK(_order[i]))
      tmp[_order[i]] = samples[ch++];

  ch = 0;
  for (i = 0; i < CH_NAMES; i++)
    if (mask & CH_MASK(i))
      samples[ch++] = tmp[i];
}

void
samples_t::reorder_from_std(Speakers _spk, const order_t _order)
{
  int i, ch;
  int mask = _spk.mask;
  assert(_spk.nch() <= NCHANNELS);

  sample_t *tmp[CH_NAMES];

  ch = 0;
  for (i = 0; i < CH_NAMES; i++)
    if (mask & CH_MASK(i))
      tmp[i] = samples[ch++];

  ch = 0;
  for (i = 0; i < CH_NAMES; i++)
    if (mask & CH_MASK(_order[i]))
      samples[ch++] = tmp[_order[i]];
}

void
samples_t::reorder(Speakers _spk, const order_t _input_order, const order_t _output_order)
{
  int i, ch;
  int mask = _spk.mask;
  assert(_spk.nch() <= NCHANNELS);

  sample_t *tmp[CH_NAMES];

  ch = 0;
  for (i = 0; i < CH_NAMES; i++)
    if (mask & CH_MASK(_input_order[i]))
      tmp[_input_order[i]] = samples[ch++];

  ch = 0;
  for (i = 0; i < CH_NAMES; i++)
    if (mask & CH_MASK(_output_order[i]))
      samples[ch++] = tmp[_output_order[i]];
}

void zero_samples(sample_t *s, size_t size)
{
  memset(s, 0, size * sizeof(sample_t));
}
void zero_samples(samples_t s, int nch, size_t size)
{
  for (int ch = 0; ch < nch; ch++)
    memset(s[ch], 0, size * sizeof(sample_t));
}

void zero_samples(sample_t *s, size_t offset, size_t size)
{
  memset(s + offset, 0, size * sizeof(sample_t));
}
void zero_samples(samples_t s, size_t offset, int nch, size_t size)
{
  for (int ch = 0; ch < nch; ch++)
    memset(s[ch] + offset, 0, size * sizeof(sample_t));
}

void copy_samples(sample_t *dst, sample_t *src, size_t size)
{
  memcpy(dst, src, size * sizeof(sample_t));
}
void copy_samples(samples_t dst, samples_t src, int nch, size_t size)
{
  for (int ch = 0; ch < nch; ch++)
    copy_samples(dst[ch], src[ch], size);
}

void copy_samples(sample_t *dst, size_t dst_offset, sample_t *src, size_t src_offset, size_t size)
{
  memcpy(dst + dst_offset, src + src_offset, size * sizeof(sample_t));
}
void copy_samples(samples_t dst, size_t dst_offset, samples_t src, size_t src_offset, int nch, size_t size)
{
  for (int ch = 0; ch < nch; ch++)
    copy_samples(dst[ch] + dst_offset, src[ch] + src_offset, size);
}

void move_samples(sample_t *dst, sample_t *src, size_t size)
{
  memmove(dst, src, size * sizeof(sample_t));
}
void move_samples(samples_t dst, samples_t src, int nch, size_t size)
{
  for (int ch = 0; ch < nch; ch++)
    move_samples(dst[ch], src[ch], size);
}

void move_samples(sample_t *dst, size_t dst_offset, sample_t *src, size_t src_offset, size_t size)
{
  memmove(dst + dst_offset, src + src_offset, size * sizeof(sample_t));
}
void move_samples(samples_t dst, size_t dst_offset, samples_t src, size_t src_offset, int nch, size_t size)
{
  for (int ch = 0; ch < nch; ch++)
    move_samples(dst[ch] + dst_offset, src[ch] + src_offset, size);
}

void gain_samples(sample_t gain, sample_t *s, size_t size)
{
  for (size_t i = 0; i < size; i++)
    s[i] *= gain;
}
void gain_samples(sample_t gain, samples_t s, int nch, size_t size)
{
  for (int ch = 0; ch < nch; ch++)
  {
    sample_t *sch = s[ch];
    for (size_t i = 0; i < size; i++)
      sch[i] *= gain;
  }
}

void gain_samples(sample_t gain, sample_t *s, size_t offset, size_t size)
{
  for (size_t i = offset; i < offset + size; i++)
    s[i] *= gain;
}
void gain_samples(sample_t gain, samples_t s, size_t offset, int nch, size_t size)
{
  for (int ch = 0; ch < nch; ch++)
  {
    sample_t *sch = s[ch];
    for (size_t i = offset; i < offset + size; i++)
      sch[i] *= gain;
  }
}

void sum_samples(sample_t *dst, sample_t *src, size_t size)
{
  for (size_t i = 0; i < size; i++)
    dst[i] += src[i];
}

void mul_samples(sample_t *dst, sample_t *src, size_t size)
{
  for (size_t i = 0; i < size; i++)
    dst[i] *= src[i];
}

sample_t max_samples(sample_t max, sample_t *s, size_t size)
{
  for (size_t i = 0; i < size; i++)
    if (fabs(s[i]) > max)
      max = fabs(s[i]);
  return max;
}

sample_t peak_diff(sample_t *s1, sample_t *s2, size_t size)
{
  sample_t diff = 0;
  for (size_t i = 0; i < size; i++)
    if (fabs(s1[i] - s2[i]) > diff)
      diff = fabs(s1[i] - s2[i]);
  return diff;
}

sample_t rms_diff(sample_t *s1, sample_t *s2, size_t size)
{
  sample_t sum = 0;
  for (size_t i = 0; i < size; i++)
    sum += (s1[i] - s2[i]) * (s1[i] - s2[i]);

  return size? sqrt(sum / size): 0.0;
}
