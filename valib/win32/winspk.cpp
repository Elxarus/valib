#include <windows.h>
#include <mmreg.h>
#include <uuids.h>
#include <ks.h>
#include <ksmedia.h>
#include "winspk.h"
#include <initguid.h>

DEFINE_GUID(KSADATAFORMAT_DOLBY_AC3_SPDIF, 
0x00000092, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71);

const unsigned int ds_channels_tbl[64] = 
{
  0, 
  SPEAKER_FRONT_LEFT, 
                       SPEAKER_FRONT_CENTER, 
  SPEAKER_FRONT_LEFT | SPEAKER_FRONT_CENTER,
                                              SPEAKER_FRONT_RIGHT,
  SPEAKER_FRONT_LEFT                        | SPEAKER_FRONT_RIGHT,
                       SPEAKER_FRONT_CENTER | SPEAKER_FRONT_RIGHT,
  SPEAKER_FRONT_LEFT | SPEAKER_FRONT_CENTER | SPEAKER_FRONT_RIGHT,
                                                                    SPEAKER_BACK_LEFT,
  SPEAKER_FRONT_LEFT                                              | SPEAKER_BACK_LEFT,
                       SPEAKER_FRONT_CENTER                       | SPEAKER_BACK_LEFT,
  SPEAKER_FRONT_LEFT | SPEAKER_FRONT_CENTER                       | SPEAKER_BACK_LEFT,
                                              SPEAKER_FRONT_RIGHT | SPEAKER_BACK_LEFT,
  SPEAKER_FRONT_LEFT                        | SPEAKER_FRONT_RIGHT | SPEAKER_BACK_LEFT,
                       SPEAKER_FRONT_CENTER | SPEAKER_FRONT_RIGHT | SPEAKER_BACK_LEFT,
  SPEAKER_FRONT_LEFT | SPEAKER_FRONT_CENTER | SPEAKER_FRONT_RIGHT | SPEAKER_BACK_LEFT,
                                                                                        SPEAKER_BACK_RIGHT,
  SPEAKER_FRONT_LEFT                                                                  | SPEAKER_BACK_RIGHT,
                       SPEAKER_FRONT_CENTER                                           | SPEAKER_BACK_RIGHT,
  SPEAKER_FRONT_LEFT | SPEAKER_FRONT_CENTER                                           | SPEAKER_BACK_RIGHT,
                                              SPEAKER_FRONT_RIGHT                     | SPEAKER_BACK_RIGHT,
  SPEAKER_FRONT_LEFT                        | SPEAKER_FRONT_RIGHT                     | SPEAKER_BACK_RIGHT,
                       SPEAKER_FRONT_CENTER | SPEAKER_FRONT_RIGHT                     | SPEAKER_BACK_RIGHT,
  SPEAKER_FRONT_LEFT | SPEAKER_FRONT_CENTER | SPEAKER_FRONT_RIGHT                     | SPEAKER_BACK_RIGHT,
                                                                    SPEAKER_BACK_LEFT | SPEAKER_BACK_RIGHT,
  SPEAKER_FRONT_LEFT                                              | SPEAKER_BACK_LEFT | SPEAKER_BACK_RIGHT,
                       SPEAKER_FRONT_CENTER                       | SPEAKER_BACK_LEFT | SPEAKER_BACK_RIGHT,
  SPEAKER_FRONT_LEFT | SPEAKER_FRONT_CENTER                       | SPEAKER_BACK_LEFT | SPEAKER_BACK_RIGHT,
                                              SPEAKER_FRONT_RIGHT | SPEAKER_BACK_LEFT | SPEAKER_BACK_RIGHT,
  SPEAKER_FRONT_LEFT                        | SPEAKER_FRONT_RIGHT | SPEAKER_BACK_LEFT | SPEAKER_BACK_RIGHT,
                       SPEAKER_FRONT_CENTER | SPEAKER_FRONT_RIGHT | SPEAKER_BACK_LEFT | SPEAKER_BACK_RIGHT,
  SPEAKER_FRONT_LEFT | SPEAKER_FRONT_CENTER | SPEAKER_FRONT_RIGHT | SPEAKER_BACK_LEFT | SPEAKER_BACK_RIGHT,
                                                                                                             SPEAKER_LOW_FREQUENCY,
  SPEAKER_FRONT_LEFT                                                                                       | SPEAKER_LOW_FREQUENCY,
                       SPEAKER_FRONT_CENTER                                                                | SPEAKER_LOW_FREQUENCY,
  SPEAKER_FRONT_LEFT | SPEAKER_FRONT_CENTER                                                                | SPEAKER_LOW_FREQUENCY,
                                              SPEAKER_FRONT_RIGHT                                          | SPEAKER_LOW_FREQUENCY,
  SPEAKER_FRONT_LEFT                        | SPEAKER_FRONT_RIGHT                                          | SPEAKER_LOW_FREQUENCY,
                       SPEAKER_FRONT_CENTER | SPEAKER_FRONT_RIGHT                                          | SPEAKER_LOW_FREQUENCY,
  SPEAKER_FRONT_LEFT | SPEAKER_FRONT_CENTER | SPEAKER_FRONT_RIGHT                                          | SPEAKER_LOW_FREQUENCY,
                                                                    SPEAKER_BACK_LEFT                      | SPEAKER_LOW_FREQUENCY,
  SPEAKER_FRONT_LEFT                                              | SPEAKER_BACK_LEFT                      | SPEAKER_LOW_FREQUENCY,
                       SPEAKER_FRONT_CENTER                       | SPEAKER_BACK_LEFT                      | SPEAKER_LOW_FREQUENCY,
  SPEAKER_FRONT_LEFT | SPEAKER_FRONT_CENTER                       | SPEAKER_BACK_LEFT                      | SPEAKER_LOW_FREQUENCY,
                                              SPEAKER_FRONT_RIGHT | SPEAKER_BACK_LEFT                      | SPEAKER_LOW_FREQUENCY,
  SPEAKER_FRONT_LEFT                        | SPEAKER_FRONT_RIGHT | SPEAKER_BACK_LEFT                      | SPEAKER_LOW_FREQUENCY,
                       SPEAKER_FRONT_CENTER | SPEAKER_FRONT_RIGHT | SPEAKER_BACK_LEFT                      | SPEAKER_LOW_FREQUENCY,
  SPEAKER_FRONT_LEFT | SPEAKER_FRONT_CENTER | SPEAKER_FRONT_RIGHT | SPEAKER_BACK_LEFT                      | SPEAKER_LOW_FREQUENCY,
                                                                                        SPEAKER_BACK_RIGHT | SPEAKER_LOW_FREQUENCY,
  SPEAKER_FRONT_LEFT                                                                  | SPEAKER_BACK_RIGHT | SPEAKER_LOW_FREQUENCY,
                       SPEAKER_FRONT_CENTER                                           | SPEAKER_BACK_RIGHT | SPEAKER_LOW_FREQUENCY,
  SPEAKER_FRONT_LEFT | SPEAKER_FRONT_CENTER                                           | SPEAKER_BACK_RIGHT | SPEAKER_LOW_FREQUENCY,
                                              SPEAKER_FRONT_RIGHT                     | SPEAKER_BACK_RIGHT | SPEAKER_LOW_FREQUENCY,
  SPEAKER_FRONT_LEFT                        | SPEAKER_FRONT_RIGHT                     | SPEAKER_BACK_RIGHT | SPEAKER_LOW_FREQUENCY,
                       SPEAKER_FRONT_CENTER | SPEAKER_FRONT_RIGHT                     | SPEAKER_BACK_RIGHT | SPEAKER_LOW_FREQUENCY,
  SPEAKER_FRONT_LEFT | SPEAKER_FRONT_CENTER | SPEAKER_FRONT_RIGHT                     | SPEAKER_BACK_RIGHT | SPEAKER_LOW_FREQUENCY,
                                                                    SPEAKER_BACK_LEFT | SPEAKER_BACK_RIGHT | SPEAKER_LOW_FREQUENCY,
  SPEAKER_FRONT_LEFT                                              | SPEAKER_BACK_LEFT | SPEAKER_BACK_RIGHT | SPEAKER_LOW_FREQUENCY,
                       SPEAKER_FRONT_CENTER                       | SPEAKER_BACK_LEFT | SPEAKER_BACK_RIGHT | SPEAKER_LOW_FREQUENCY,
  SPEAKER_FRONT_LEFT | SPEAKER_FRONT_CENTER                       | SPEAKER_BACK_LEFT | SPEAKER_BACK_RIGHT | SPEAKER_LOW_FREQUENCY,
                                              SPEAKER_FRONT_RIGHT | SPEAKER_BACK_LEFT | SPEAKER_BACK_RIGHT | SPEAKER_LOW_FREQUENCY,
  SPEAKER_FRONT_LEFT                        | SPEAKER_FRONT_RIGHT | SPEAKER_BACK_LEFT | SPEAKER_BACK_RIGHT | SPEAKER_LOW_FREQUENCY,
                       SPEAKER_FRONT_CENTER | SPEAKER_FRONT_RIGHT | SPEAKER_BACK_LEFT | SPEAKER_BACK_RIGHT | SPEAKER_LOW_FREQUENCY,
  SPEAKER_FRONT_LEFT | SPEAKER_FRONT_CENTER | SPEAKER_FRONT_RIGHT | SPEAKER_BACK_LEFT | SPEAKER_BACK_RIGHT | SPEAKER_LOW_FREQUENCY,  
};

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

      ext->SubFormat = KSADATAFORMAT_DOLBY_AC3_SPDIF;
      ext->Samples.wValidBitsPerSample = 16;
      ext->dwChannelMask = ds_channels_tbl[MODE_STEREO];
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
        case FORMAT_AC3: wfx->wFormatTag = WAVE_FORMAT_AC3;
        case FORMAT_DTS: wfx->wFormatTag = WAVE_FORMAT_DTS;
        case FORMAT_MPA: wfx->wFormatTag = WAVE_FORMAT_MPEG;
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

        ext->SubFormat = KSDATAFORMAT_SUBTYPE_PCM;
        ext->Samples.wValidBitsPerSample = 16;
        ext->dwChannelMask = ds_channels_tbl[spk.mask];
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

        ext->SubFormat = KSDATAFORMAT_SUBTYPE_PCM;
        ext->Samples.wValidBitsPerSample = 24;
        ext->dwChannelMask = ds_channels_tbl[spk.mask];
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

        ext->SubFormat = KSDATAFORMAT_SUBTYPE_PCM;
        ext->Samples.wValidBitsPerSample = 32;
        ext->dwChannelMask = ds_channels_tbl[spk.mask];
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

        ext->SubFormat = KSDATAFORMAT_SUBTYPE_IEEE_FLOAT;
        ext->Samples.wValidBitsPerSample = 32;
        ext->dwChannelMask = ds_channels_tbl[spk.mask];
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
  sample_t level = 1.0;
  WAVEFORMATEXTENSIBLE *wfex = 0;

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
    if (wfex->SubFormat == KSDATAFORMAT_SUBTYPE_IEEE_FLOAT)
      format = FORMAT_PCMFLOAT;
    else if (wfex->SubFormat == KSDATAFORMAT_SUBTYPE_PCM)
      switch (wfx->wBitsPerSample)
      {
        case 16: format = FORMAT_PCM16; level = 32767;      break;
        case 24: format = FORMAT_PCM24; level = 8388607;    break;
        case 32: format = FORMAT_PCM32; level = 2147483647; break;
        default: return false;
      }
    else if (wfex->SubFormat == KSADATAFORMAT_DOLBY_AC3_SPDIF)
      format = FORMAT_SPDIF;
    else
      return false;

    // determine audio mode
    for (mask = 0; mask < sizeof(ds_channels_tbl) / sizeof(ds_channels_tbl[0]); mask++)
      if (ds_channels_tbl[mask] == wfex->dwChannelMask)
        break;

    if (mask == sizeof(ds_channels_tbl) / sizeof(ds_channels_tbl[0]))
      return false;
  }
  else
  {
    // determine sample format
    switch (wfx->wFormatTag)
    {
      case WAVE_FORMAT_IEEE_FLOAT:
        format = FORMAT_PCMFLOAT;
        break;

      case WAVE_FORMAT_PCM:
        switch (wfx->wBitsPerSample)
        {
          case 16: format = FORMAT_PCM16; level = 32767;      break;
          case 24: format = FORMAT_PCM24; level = 8388607;    break;
          case 32: format = FORMAT_PCM32; level = 2147483647; break;
          default: return false;
        }
        break;

      case WAVE_FORMAT_AC3:
        format = FORMAT_AC3;
        break;

      case WAVE_FORMAT_DTS:
        format = FORMAT_DTS;
        break;

      case WAVE_FORMAT_MPEG:
        format = FORMAT_MPA;
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
        case 3: mask = MODE_3_0;    break;
        case 4: mask = MODE_QUADRO; break;
        case 5: mask = MODE_3_2;    break;
        case 6: mask = MODE_5_1;    break;
        default: return false;
      }
  }

  spk = Speakers(format, mask, wfx->nSamplesPerSec, level);
  return true;
}

bool is_compatible(Speakers _spk, WAVEFORMATEX *_wfx)
{
  WAVEFORMATEXTENSIBLE wfx_tmp;

  if (!spk2wfx(_spk, (WAVEFORMATEX *)&wfx_tmp, true)) 
    return false;

  if (_wfx->cbSize == wfx_tmp.Format.cbSize)
    if (!memcmp(_wfx, &wfx_tmp, wfx_tmp.Format.cbSize))
      return true;

  if (!spk2wfx(_spk, (WAVEFORMATEX *)&wfx_tmp, false)) 
    return false;

  if (_wfx->cbSize == wfx_tmp.Format.cbSize)
    if (!memcmp(_wfx, &wfx_tmp, wfx_tmp.Format.cbSize))
      return true;

  return false;
};
