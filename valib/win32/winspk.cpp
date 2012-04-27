#include "winspk.h"

static const GUID GUID_DOLBY_AC3_SPDIF    = { 0x00000092, 0x0000, 0x0010, { 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71 } };
static const GUID GUID_SUBTYPE_PCM        = { 0x00000001, 0x0000, 0x0010, { 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71 } };
static const GUID GUID_SUBTYPE_IEEE_FLOAT = { 0x00000003, 0x0000, 0x0010, { 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71 } };
static const WORD extensible_size = sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX);

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

template<>
WAVEFORMATEXTENSIBLE *wf_cast<WAVEFORMATEXTENSIBLE>(void *wf, size_t size)
{
  if (wf == 0 || size < sizeof(WAVEFORMATEXTENSIBLE)) return 0;
  WAVEFORMATEXTENSIBLE *wfx = (WAVEFORMATEXTENSIBLE *)wf;
  if (sizeof(WAVEFORMATEX) + wfx->Format.cbSize > size) return 0;
  // WAVEFORMATEXTENSIBLE specific
  if (wfx->Format.wFormatTag != WAVE_FORMAT_EXTENSIBLE) return 0;
  if (wfx->Format.cbSize != extensible_size) return 0;
  return wfx;
}

inline void
fill_wfe(WAVEFORMATEX *wfe, WORD wFormatTag, WORD nChannels, DWORD nSamplesPerSec, WORD wBitsPerSample, WORD cbSize = 0)
{
  wfe->wFormatTag = wFormatTag;
  wfe->nChannels = nChannels;
  wfe->nSamplesPerSec = nSamplesPerSec;
  wfe->wBitsPerSample = wBitsPerSample;
  wfe->nBlockAlign = wBitsPerSample / 8 * wfe->nChannels;
  wfe->nAvgBytesPerSec = wfe->nSamplesPerSec * wfe->nBlockAlign;
  wfe->cbSize = cbSize;
}

///////////////////////////////////////////////////////////////////////////////
// PCM conversions

WAVEFORMATEX *pcm2waveformatex(Speakers spk)
{
  WAVEFORMATEX *wfe = new WAVEFORMATEX;
  memset(wfe, 0, sizeof(*wfe));
  fill_wfe(wfe, 
    spk.is_floating_point()? WAVE_FORMAT_IEEE_FLOAT: WAVE_FORMAT_PCM,
    spk.nch(), spk.sample_rate, spk.sample_size() * 8);
  return wfe;
}

WAVEFORMATEX *pcm2waveformatextensible(Speakers spk)
{
  WAVEFORMATEXTENSIBLE *wfx = new WAVEFORMATEXTENSIBLE;
  memset(wfx, 0, sizeof(*wfx));
  fill_wfe(&wfx->Format, 
    WAVE_FORMAT_EXTENSIBLE,
    spk.nch(), spk.sample_rate, spk.sample_size() * 8);

  wfx->Format.cbSize = extensible_size;
  wfx->SubFormat = spk.is_floating_point()? GUID_SUBTYPE_IEEE_FLOAT: GUID_SUBTYPE_PCM;
  wfx->Samples.wValidBitsPerSample = spk.sample_size() * 8;
  wfx->dwChannelMask = mask2ds(spk.mask);
  return &wfx->Format;
}

WAVEFORMATEX *pcm2wfe(Speakers spk, int i)
{
  // Return only WAVEFORMATEX for simple formats (mono/stereo 16bit pcm)
  // Return WAVEFORMATEXTENSIBLE on the first place.
  bool simple_format = ((spk.mask == MODE_MONO) || (spk.mask == MODE_STEREO)) && (spk.format == FORMAT_PCM16);

  if (i == 0 && !simple_format)
    return pcm2waveformatextensible(spk);
  if ((i == 0 && simple_format) || ((i == 1) && (!simple_format)))
    return pcm2waveformatex(spk);
  return 0;
}

///////////////////////////////////////////////////////////////////////////////
// SPDIF conversions

WAVEFORMATEX *spdif2waveformatex(Speakers spk)
{
  WAVEFORMATEX *wfe = new WAVEFORMATEX;
  memset(wfe, 0, sizeof(*wfe));
  fill_wfe(wfe, 
    WAVE_FORMAT_DOLBY_AC3_SPDIF,
    2, spk.sample_rate, 16);
  return wfe;
}

WAVEFORMATEX *spdif2waveformatextensible(Speakers spk)
{
  WAVEFORMATEXTENSIBLE *wfx = new WAVEFORMATEXTENSIBLE;
  memset(wfx, 0, sizeof(WAVEFORMATEXTENSIBLE));
  fill_wfe(&wfx->Format, 
    WAVE_FORMAT_EXTENSIBLE,
    2, spk.sample_rate, 16);
  wfx->Format.cbSize = extensible_size;
  wfx->SubFormat = GUID_DOLBY_AC3_SPDIF;
  wfx->Samples.wValidBitsPerSample = 16;
  wfx->dwChannelMask = mask2ds(spk.mask);
  return &wfx->Format;
}

WAVEFORMATEX *
spdif2wfe(Speakers spk, int i)
{
  if (i == 0) return spdif2waveformatextensible(spk);
  if (i == 1) return spdif2waveformatex(spk);
  return 0;
}

///////////////////////////////////////////////////////////////////////////////

static struct {
  int format;
  WAVEFORMATEX * (*convert)(Speakers spk, int i);
}
spk2wfe_tbl[] =
{
  { FORMAT_PCM16,     pcm2wfe },
  { FORMAT_PCM24,     pcm2wfe },
  { FORMAT_PCM32,     pcm2wfe },
  { FORMAT_PCMFLOAT,  pcm2wfe },
  { FORMAT_PCMDOUBLE, pcm2wfe },
  { FORMAT_SPDIF,     spdif2wfe },
};

WAVEFORMATEX *
spk2wfe(Speakers spk, int i)
{
  for (int j = 0; j < array_size(spk2wfe_tbl); j++)
    if (spk2wfe_tbl[j].format == spk.format)
      return spk2wfe_tbl[j].convert(spk, i);
  return 0;
}

///////////////////////////////////////////////////////////////////////////////

bool
waveformatextensible2pcm(WAVEFORMAT *wf, size_t size, Speakers &spk)
{
  WAVEFORMATEXTENSIBLE *wfx = wf_cast<WAVEFORMATEXTENSIBLE>(wf, size);
  if (wfx == 0) return false;
  WAVEFORMATEX *wfe = &wfx->Format;

  int format;
  if (wfx->SubFormat == GUID_SUBTYPE_IEEE_FLOAT)
    switch (wfe->wBitsPerSample)
    {
      case 32: format = FORMAT_PCMFLOAT;  break;
      case 64: format = FORMAT_PCMDOUBLE; break;
      default: return false;
    }
  else if (wfx->SubFormat == GUID_SUBTYPE_PCM)
    switch (wfe->wBitsPerSample)
    {
      case 16: format = FORMAT_PCM16; break;
      case 24: format = FORMAT_PCM24; break;
      case 32: format = FORMAT_PCM32; break;
      default: return false;
    }
  else
    return false;

  int mask = ds2mask(wfx->dwChannelMask);
  int sample_rate = wfe->nSamplesPerSec;
  spk = Speakers(format, mask, sample_rate);
  return true;
}

bool
pcmwaveformat2pcm(WAVEFORMAT *wf, size_t size, Speakers &spk)
{
  PCMWAVEFORMAT *pcmwf = wf_cast<PCMWAVEFORMAT>(wf, size);
  if (pcmwf == 0) return false;

  int format;
  if (wf->wFormatTag == WAVE_FORMAT_IEEE_FLOAT)
    switch (pcmwf->wBitsPerSample)
    {
      case 32: format = FORMAT_PCMFLOAT;  break;
      case 64: format = FORMAT_PCMDOUBLE; break;
      default: return false;
    }
  else if (wf->wFormatTag == WAVE_FORMAT_PCM)
    switch (pcmwf->wBitsPerSample)
    {
      case 16: format = FORMAT_PCM16; break;
      case 24: format = FORMAT_PCM24; break;
      case 32: format = FORMAT_PCM32; break;
      default: return false;
    }
  else
    return false;

  int mask = nch2mask(wf->nChannels);
  int sample_rate = wf->nSamplesPerSec;
  spk = Speakers(format, mask, sample_rate);
  return true;
}

bool
waveformat2spdif(WAVEFORMAT *wf, size_t size, Speakers &spk)
{
  if (wf->wFormatTag == WAVE_FORMAT_DOLBY_AC3_SPDIF)
  {
    spk = Speakers(FORMAT_SPDIF, 0, wf->nSamplesPerSec);
    return true;
  }
  else if (wf->wFormatTag == WAVE_FORMAT_EXTENSIBLE)
  {
    WAVEFORMATEXTENSIBLE *wfx = wf_cast<WAVEFORMATEXTENSIBLE>(wf, size);
    if (wfx == 0 || wfx->SubFormat != GUID_DOLBY_AC3_SPDIF)
      return false;
    spk = Speakers(FORMAT_SPDIF, ds2mask(wfx->dwChannelMask), wf->nSamplesPerSec);
    return true;
  }
  return false;
}

bool
waveformat2spk(WAVEFORMAT *wf, size_t size, Speakers &spk)
{
  int format;
  switch (wf->wFormatTag)
  {
    case WAVE_FORMAT_AVI_AC3:
      format = FORMAT_AC3_EAC3;
      break;

    case WAVE_FORMAT_AVI_DTS:
      format = FORMAT_DTS;
      break;

    case WAVE_FORMAT_MPEGLAYER3:
    case WAVE_FORMAT_MPEG:
      format = FORMAT_MPA;
      break;

    case WAVE_FORMAT_AVI_AAC:
      format = FORMAT_AAC_FRAME;
      break;

    case WAVE_FORMAT_FLAC:
      format = FORMAT_FLAC;
      break;

    default:
      return false;
  }

  int mask = nch2mask(wf->nChannels);
  int sample_rate = wf->nSamplesPerSec;
  spk = Speakers(format, mask, sample_rate);
  
  // format data
  WAVEFORMATEX *wfe = wf_cast<WAVEFORMATEX>(wf, size);
  if (wfe && wfe->cbSize)
  {
    uint8_t *format_data = reinterpret_cast<uint8_t *>(wfe+1);
    size_t data_size = wfe->cbSize;
    spk.set_format_data(format_data, data_size);
  }

  return true;
}

///////////////////////////////////////////////////////////////////////////////

static bool (*wf2spk_converters[])(WAVEFORMAT *, size_t, Speakers &) = 
{
  pcmwaveformat2pcm,
  waveformat2spk,
  waveformatextensible2pcm,
  waveformat2spdif
};

Speakers wf2spk(WAVEFORMAT *wf, size_t size)
{
  if (!wf || size < sizeof(WAVEFORMAT)) return Speakers();
  Speakers result;
  for (int i = 0; i < array_size(wf2spk_converters); i++)
    if (wf2spk_converters[i](wf, size, result))
      return result;
  return Speakers();
}

Speakers wf2spk(WAVEFORMATEX *wave_format, size_t size)
{ return wf2spk((WAVEFORMAT *)wave_format, size); }

Speakers wf2spk(WAVEFORMATEXTENSIBLE *wave_format, size_t size)
{ return wf2spk((WAVEFORMAT *)wave_format, size); }

///////////////////////////////////////////////////////////////////////////////

bool wf_equal(WAVEFORMAT *wf1, size_t size1, WAVEFORMAT *wf2, size_t size2)
{
  if (size1 == 0 && size2 == 0) return true; // uninitialized structures are equal
  if (wf1 == 0 || wf2 == 0) return false;    // null pointers check
  // Compare pre-WAVEFORMATEX structures
  if (size1 < sizeof(WAVEFORMATEX) && size1 == size2)
    return memcmp(wf1, wf2, size1) == 0;
  // Compare WAVEFORMATEXs
  WAVEFORMATEX *wfe1 = wf_cast<WAVEFORMATEX>(wf1, size1);
  WAVEFORMATEX *wfe2 = wf_cast<WAVEFORMATEX>(wf2, size2);
  if (wfe1 && wfe2 && wfe1->cbSize == wfe2->cbSize)
    return memcmp(wfe1, wfe2, sizeof(WAVEFORMATEX) + wfe1->cbSize) == 0;
  return false;
}

bool is_compatible(Speakers spk, WAVEFORMAT *wf, size_t size)
{
  int i = 0;
  std::auto_ptr<WAVEFORMATEX> wfe(spk2wfe(spk, 0));
  while (wfe.get())
  {
    if (wf_equal((WAVEFORMAT *)wfe.get(), sizeof(WAVEFORMATEX) + wfe->cbSize, wf, size))
      return true;
    wfe.reset(spk2wfe(spk, ++i));
  }
  return false;
}
