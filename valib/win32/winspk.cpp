#include "winspk.h"

static const GUID GUID_DOLBY_AC3_SPDIF    = { 0x00000092, 0x0000, 0x0010, { 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71 } };
static const GUID GUID_SUBTYPE_PCM        = { 0x00000001, 0x0000, 0x0010, { 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71 } };
static const GUID GUID_SUBTYPE_IEEE_FLOAT = { 0x00000003, 0x0000, 0x0010, { 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71 } };

#define SPEAKER_FRONT_LEFT              0x1
#define SPEAKER_FRONT_RIGHT             0x2
#define SPEAKER_FRONT_CENTER            0x4
#define SPEAKER_LOW_FREQUENCY           0x8
#define SPEAKER_BACK_LEFT               0x10
#define SPEAKER_BACK_RIGHT              0x20
#define SPEAKER_FRONT_LEFT_OF_CENTER    0x40
#define SPEAKER_FRONT_RIGHT_OF_CENTER   0x80
#define SPEAKER_BACK_CENTER             0x100
#define SPEAKER_SIDE_LEFT               0x200
#define SPEAKER_SIDE_RIGHT              0x400
#define SPEAKER_TOP_CENTER              0x800
#define SPEAKER_TOP_FRONT_LEFT          0x1000
#define SPEAKER_TOP_FRONT_CENTER        0x2000
#define SPEAKER_TOP_FRONT_RIGHT         0x4000
#define SPEAKER_TOP_BACK_LEFT           0x8000
#define SPEAKER_TOP_BACK_CENTER         0x10000
#define SPEAKER_TOP_BACK_RIGHT          0x20000

static const int mask_tbl[][2] =
{
  { CH_MASK_L,   SPEAKER_FRONT_LEFT },
  { CH_MASK_C,   SPEAKER_FRONT_CENTER },
  { CH_MASK_R,   SPEAKER_FRONT_RIGHT },
  { CH_MASK_SL,  SPEAKER_SIDE_LEFT },
  { CH_MASK_SR,  SPEAKER_SIDE_RIGHT },
  { CH_MASK_LFE, SPEAKER_LOW_FREQUENCY },
  { CH_MASK_CL,  SPEAKER_FRONT_LEFT_OF_CENTER },
  { CH_MASK_CR,  SPEAKER_FRONT_RIGHT_OF_CENTER },
  { CH_MASK_BL,  SPEAKER_BACK_LEFT },
  { CH_MASK_BC,  SPEAKER_BACK_CENTER },
  { CH_MASK_BR,  SPEAKER_BACK_RIGHT }
};

// Convert DirectSound channel mask to Speakers channel mask
int ds2mask(int ds_mask)
{
  int spk_mask = 0;
  for (int i = 0; i < array_size(mask_tbl); i++)
    if (ds_mask & mask_tbl[i][1])
      spk_mask |= mask_tbl[i][0];
  return spk_mask;
}

// Convert Speakers channel mask to DirectSound channel mask
int mask2ds(int spk_mask)
{
  int ds_mask = 0;
  for (int i = 0; i < array_size(mask_tbl); i++)
    if (spk_mask & mask_tbl[i][0])
      ds_mask |= mask_tbl[i][1];
  return ds_mask;
}

bool 
spk2wfx(Speakers spk, WAVEFORMATEX *wfx, bool use_extensible)
{
  if (spk.is_spdif())
  {
    // SPDIF format
    wfx->wFormatTag = WAVE_FORMAT_DOLBY_AC3_SPDIF;
    wfx->nChannels = 2;
    wfx->nSamplesPerSec = spk.sample_rate;
    wfx->wBitsPerSample = 16;
    wfx->nBlockAlign = 4;
    wfx->nAvgBytesPerSec = wfx->nSamplesPerSec * wfx->nBlockAlign;
    wfx->cbSize = 0;

    if (use_extensible)
    {
      WAVEFORMATEXTENSIBLE *ext = (WAVEFORMATEXTENSIBLE *)wfx;
      wfx->wFormatTag = WAVE_FORMAT_EXTENSIBLE;
      wfx->cbSize = 22;

      ext->SubFormat = GUID_DOLBY_AC3_SPDIF;
      ext->Samples.wValidBitsPerSample = 16;
      ext->dwChannelMask = mask2ds(MODE_STEREO);
    }
    return true;
  }

  WAVEFORMATEXTENSIBLE *ext = (WAVEFORMATEXTENSIBLE *)wfx;

  // always use WAVEFORMATEX for mono/stereo 16bit format
  use_extensible &= (spk.nch() > 2) || (spk.format != FORMAT_PCM16);

  if (use_extensible)
    memset(wfx, 0, sizeof(WAVEFORMATEXTENSIBLE));
  else
    memset(wfx, 0, sizeof(WAVEFORMATEX));

  int nchannels = spk.nch();

  switch (spk.format)
  {
    case FORMAT_AC3:
    case FORMAT_DTS:
    case FORMAT_MPA:
      switch (spk.format)
      {
        case FORMAT_AC3: wfx->wFormatTag = WAVE_FORMAT_AVI_AC3; break;
        case FORMAT_DTS: wfx->wFormatTag = WAVE_FORMAT_AVI_DTS; break;
        case FORMAT_MPA: wfx->wFormatTag = WAVE_FORMAT_MPEG;    break;
        default: return false;
      }
  
      wfx->nChannels = nchannels;
      wfx->nSamplesPerSec = spk.sample_rate;
      wfx->wBitsPerSample = 0;
      wfx->nBlockAlign = 1;
      wfx->nAvgBytesPerSec = 0;
      wfx->cbSize = 0;
      break;

    case FORMAT_PCM16:
      wfx->wFormatTag = WAVE_FORMAT_PCM;
      wfx->nChannels = nchannels;
      wfx->nSamplesPerSec = spk.sample_rate;
      wfx->wBitsPerSample = 16;
      wfx->nBlockAlign = wfx->wBitsPerSample / 8 * wfx->nChannels;
      wfx->nAvgBytesPerSec = wfx->nSamplesPerSec * wfx->nBlockAlign;
      wfx->cbSize = 0;

      if (use_extensible)
      {
        wfx->wFormatTag = WAVE_FORMAT_EXTENSIBLE;
        wfx->cbSize = 22;

        ext->SubFormat = GUID_SUBTYPE_PCM;
        ext->Samples.wValidBitsPerSample = 16;
        ext->dwChannelMask = mask2ds(spk.mask);
      }
      break;

    case FORMAT_PCM24:
      wfx->wFormatTag = WAVE_FORMAT_PCM;
      wfx->nChannels = nchannels;
      wfx->nSamplesPerSec = spk.sample_rate;
      wfx->wBitsPerSample = 24;
      wfx->nBlockAlign = wfx->wBitsPerSample / 8 * wfx->nChannels;
      wfx->nAvgBytesPerSec = wfx->nSamplesPerSec * wfx->nBlockAlign;
      wfx->cbSize = 0;

      if (use_extensible)
      {
        wfx->wFormatTag = WAVE_FORMAT_EXTENSIBLE;
        wfx->cbSize = 22;

        ext->SubFormat = GUID_SUBTYPE_PCM;
        ext->Samples.wValidBitsPerSample = 24;
        ext->dwChannelMask = mask2ds(spk.mask);
      }
      break;

    case FORMAT_PCM32:
      wfx->wFormatTag = WAVE_FORMAT_PCM;
      wfx->nChannels = nchannels;
      wfx->nSamplesPerSec = spk.sample_rate;
      wfx->wBitsPerSample = 32;
      wfx->nBlockAlign = wfx->wBitsPerSample / 8 * wfx->nChannels;
      wfx->nAvgBytesPerSec = wfx->nSamplesPerSec * wfx->nBlockAlign;
      wfx->cbSize = 0;

      if (use_extensible)
      {
        wfx->wFormatTag = WAVE_FORMAT_EXTENSIBLE;
        wfx->cbSize = 22;

        ext->SubFormat = GUID_SUBTYPE_PCM;
        ext->Samples.wValidBitsPerSample = 32;
        ext->dwChannelMask = mask2ds(spk.mask);
      }
      break;

    case FORMAT_PCMFLOAT:
      wfx->wFormatTag = WAVE_FORMAT_IEEE_FLOAT;
      wfx->nChannels = nchannels;
      wfx->nSamplesPerSec = spk.sample_rate;
      wfx->wBitsPerSample = 32;
      wfx->nBlockAlign = wfx->wBitsPerSample / 8 * wfx->nChannels;
      wfx->nAvgBytesPerSec = wfx->nSamplesPerSec * wfx->nBlockAlign;
      wfx->cbSize = 0;

      if (use_extensible)
      {
        wfx->wFormatTag = WAVE_FORMAT_EXTENSIBLE;
        wfx->cbSize = 22;

        ext->SubFormat = GUID_SUBTYPE_IEEE_FLOAT;
        ext->Samples.wValidBitsPerSample = 32;
        ext->dwChannelMask = mask2ds(spk.mask);
      }
      break;

    case FORMAT_PCMDOUBLE:
      wfx->wFormatTag = WAVE_FORMAT_IEEE_FLOAT;
      wfx->nChannels = nchannels;
      wfx->nSamplesPerSec = spk.sample_rate;
      wfx->wBitsPerSample = 64;
      wfx->nBlockAlign = wfx->wBitsPerSample / 8 * wfx->nChannels;
      wfx->nAvgBytesPerSec = wfx->nSamplesPerSec * wfx->nBlockAlign;
      wfx->cbSize = 0;

      if (use_extensible)
      {
        wfx->wFormatTag = WAVE_FORMAT_EXTENSIBLE;
        wfx->cbSize = 22;

        ext->SubFormat = GUID_SUBTYPE_IEEE_FLOAT;
        ext->Samples.wValidBitsPerSample = 64;
        ext->dwChannelMask = mask2ds(spk.mask);
      }
      break;

    default:
      // unknown format
      return false;
  }

  return true;
};

bool
wfx2spk(WAVEFORMATEX *wfx, Speakers &spk)
{
  int format, mask;
  WAVEFORMATEXTENSIBLE *wfex = 0;
  uint8_t *format_data = 0;
  size_t data_size = 0;

  if (!wfx)
    return false;

  if (wfx->wFormatTag == WAVE_FORMAT_DOLBY_AC3_SPDIF)
  {
    spk = Speakers(FORMAT_SPDIF, 0, wfx->nSamplesPerSec);
    return true;
  }

  if (wfx->wFormatTag == WAVE_FORMAT_EXTENSIBLE)
  {
    // extensible
    if (wfx->cbSize < 22)
      return false;

    wfex = (WAVEFORMATEXTENSIBLE *)wfx;

    // determine sample format
    if (wfex->SubFormat == GUID_SUBTYPE_IEEE_FLOAT)
      switch (wfx->wBitsPerSample)
      {
        case 32: format = FORMAT_PCMFLOAT;  break;
        case 64: format = FORMAT_PCMDOUBLE; break;
        default: return false;
      }
    else if (wfex->SubFormat == GUID_SUBTYPE_PCM)
      switch (wfx->wBitsPerSample)
      {
        case 16: format = FORMAT_PCM16; break;
        case 24: format = FORMAT_PCM24; break;
        case 32: format = FORMAT_PCM32; break;
        default: return false;
      }
    else if (wfex->SubFormat == GUID_DOLBY_AC3_SPDIF)
      format = FORMAT_SPDIF;
    else
      return false;

    // determine audio mode
    mask = ds2mask(wfex->dwChannelMask);
  }
  else
  {
    // determine sample format
    switch (wfx->wFormatTag)
    {
      case WAVE_FORMAT_IEEE_FLOAT:
        switch (wfx->wBitsPerSample)
        {
          case 32: format = FORMAT_PCMFLOAT;  break;
          case 64: format = FORMAT_PCMDOUBLE; break;
          default: return false;
        }
        break;

      case WAVE_FORMAT_PCM:
        switch (wfx->wBitsPerSample)
        {
          case 16: format = FORMAT_PCM16; break;
          case 24: format = FORMAT_PCM24; break;
          case 32: format = FORMAT_PCM32; break;
          default: return false;
        }
        break;

      case WAVE_FORMAT_AVI_AC3:
        format = FORMAT_AC3;
        break;

      case WAVE_FORMAT_AVI_DTS:
        format = FORMAT_DTS;
        break;

      case WAVE_FORMAT_MPEG:
        format = FORMAT_MPA;
        break;

      case WAVE_FORMAT_AVI_AAC:
        format = FORMAT_AAC_FRAME;
        break;

      default:
        return false;
    }

    // determine audio mode
    mask = 0;
    if (FORMAT_MASK(format) & FORMAT_CLASS_PCM)
      switch (wfx->nChannels)
      {
        case 1: mask = MODE_MONO;   break;
        case 2: mask = MODE_STEREO; break;
        case 3: mask = MODE_2_0_LFE;break;
        case 4: mask = MODE_QUADRO; break;
        case 5: mask = MODE_3_2;    break;
        case 6: mask = MODE_5_1;    break;
        case 7: mask = MODE_6_1;    break;
        case 8: mask = MODE_7_1;    break;
        default: return false;
      }

    // format data
    if (wfx->cbSize)
    {
      format_data = reinterpret_cast<uint8_t *>(wfx+1);
      data_size = wfx->cbSize;
    }
  }

  spk = Speakers(format, mask, wfx->nSamplesPerSec);
  spk.set_format_data(format_data, data_size);
  return true;
}

bool is_compatible(Speakers _spk, WAVEFORMATEX *_wfx)
{
  WAVEFORMATEXTENSIBLE wfx_tmp;

  if (!spk2wfx(_spk, (WAVEFORMATEX *)&wfx_tmp, true)) 
    return false;

  if (_wfx->cbSize == wfx_tmp.Format.cbSize)
    if (!memcmp(_wfx, &wfx_tmp, sizeof(WAVEFORMATEX) + wfx_tmp.Format.cbSize))
      return true;

  if (!spk2wfx(_spk, (WAVEFORMATEX *)&wfx_tmp, false)) 
    return false;

  if (_wfx->cbSize == wfx_tmp.Format.cbSize)
    if (!memcmp(_wfx, &wfx_tmp, sizeof(WAVEFORMATEX) + wfx_tmp.Format.cbSize))
      return true;

  return false;
};
