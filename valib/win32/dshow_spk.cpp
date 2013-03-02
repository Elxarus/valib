#include "dshow_spk.h"
#include "winspk.h"
#include "guids.h"
#include "wmcodecdsp.h"

struct VORBISFORMAT2  //matroska.org
{
  DWORD channels;
  DWORD samplesPerSec;
  DWORD bitsPerSample;
  DWORD headerSize[3];
};

///////////////////////////////////////////////////////////////////////////////
// Speakers class format can be converted to DirectShow media type in number
// of ways because of media type redundancy and ambiguity. But each media type
// defines exactly one format. (One-to-many relation)
//
// Therefore we can convert media type to Speakers unambiguously but have to
// enumerate media types when converting from Speakers to media type.
//
// Also we cannot compare Speakers and media type and have to enumerate all
// possible conversions.
//
// To convert Speakers to media type following variations are used:
//
// 1) for PCM formats:
//    Media type:    MEDIATYPE_Audio
//    Media subtype: MEDIASUBTYPE_PCM
// 1.1) Wave format tag: WAVE_FORMAT_PCM / WAVE_FORMAT_IEEE_FLOAT
// 1.1) Wave format tag: WAVE_FORMAT_EXTENSIBLE
//      Subformat: KSDATAFORMAT_SUBTYPE_PCM / KSDATAFORMAT_SUBTYPE_IEEE_FLOAT
//
// 2) for SPDIF format:
// 2.1) Media type:      MEDIATYPE_Audio
//      Media subtype:   MEDIASUBTYPE_DOLBY_AC3_SPDIF
//      Wave format tag: WAVE_FORMAT_DOLBY_AC3_SPDIF
// 2.2) Media type:      MEDIATYPE_Audio
//      Media subtype:   MEDIASUBTYPE_PCM
//      Wave format tag: WAVE_FORMAT_DOLBY_AC3_SPDIF
//      This format is used by DivX Player. It does not understand any other
//      SPDIF format
//
// Extensible format is not used for SPDIF because I wanted to make as small
// changes as possible. Enumerating of all possible SPDIF formats will require
// much more changes.
//
// Other formats (MPA, AC3, etc) are not used currently.

bool mt2spk(CMediaType mt, Speakers &spk)
{
  const GUID type = *mt.Type();
  const GUID subtype = *mt.Subtype();
  const GUID formattype = *mt.FormatType();

  WAVEFORMAT *wf = 0;
  size_t wf_size = 0;
  int sample_rate = 0;

  if ((formattype == FORMAT_WaveFormatEx) &&
      (mt.FormatLength() > sizeof(WAVEFORMAT)))
  {
    wf = (WAVEFORMAT *)mt.Format();
    wf_size = mt.FormatLength();
    sample_rate = wf->nSamplesPerSec;
  }

  /////////////////////////////////////////////////////////
  // HD LPCM

  if (type == MEDIATYPE_Audio &&
      subtype == MEDIASUBTYPE_HDMV_LPCM_AUDIO &&
      wf && wf->wFormatTag == 1)
  {
    spk = wf2spk(wf, wf_size);
    switch (spk.format)
    {
      case FORMAT_PCM16: spk.format = FORMAT_PCM16_BE; return true;
      case FORMAT_PCM24: spk.format = FORMAT_PCM24_BE; return true;
      case FORMAT_PCM32: spk.format = FORMAT_PCM32_BE; return true;
      default: return false;
    }
  }

  /////////////////////////////////////////////////////////
  // Compressed formats

  if (type == MEDIATYPE_MPEG2_PES ||
      type == MEDIATYPE_MPEG2_PACK ||
      type == MEDIATYPE_DVD_ENCRYPTED_PACK)
    if (subtype == MEDIASUBTYPE_DOLBY_AC3 ||
        subtype == MEDIASUBTYPE_DTS ||
        subtype == MEDIASUBTYPE_MPEG1AudioPayload ||
        subtype == MEDIASUBTYPE_MPEG2_AUDIO ||
        subtype == MEDIASUBTYPE_DVD_LPCM_AUDIO)
    {
      spk = Speakers(FORMAT_PES, 0, sample_rate);
      return true;
    }

  if (subtype == MEDIASUBTYPE_DOLBY_AC3 || 
      subtype == MEDIASUBTYPE_AVI_AC3)
  {
    // It may be AC3 or EAC3
    spk = Speakers(FORMAT_DOLBY, 0, sample_rate);
    return true;
  }

  if (subtype == MEDIASUBTYPE_DOLBY_DDPLUS)
  {
    spk = Speakers(FORMAT_EAC3, 0, sample_rate);
    return true;
  }

  if (subtype == MEDIASUBTYPE_DOLBY_TRUEHD)
  {
    spk = Speakers(FORMAT_TRUEHD, 0, sample_rate);
    return true;
  }

  if (subtype == MEDIASUBTYPE_DTS || 
      subtype == MEDIASUBTYPE_DTS_HD ||
      subtype == MEDIASUBTYPE_AVI_DTS)
  {
    spk = Speakers(FORMAT_DTS, 0, sample_rate);
    return true;
  }

  if (subtype == MEDIASUBTYPE_MPEG1AudioPayload ||
      subtype == MEDIASUBTYPE_MPEG2_AUDIO)
  {
    spk = Speakers(FORMAT_MPA, 0, sample_rate);
    return true;
  }

  if (subtype == MEDIASUBTYPE_DOLBY_AC3_SPDIF)
  {
    spk = Speakers(FORMAT_SPDIF, 0, sample_rate);
    return true;
  }
/*
  if (subtype == MEDIASUBTYPE_Vorbis &&
      formattype == FORMAT_Vorbis && 
      mt.FormatLength() > sizeof(VORBISFORMAT))
  {
    VORBISFORMAT *format = (VORBISFORMAT *)mt.Format();
    spk = Speakers(FORMAT_VORBIS, 0, format->samplesPerSec);
    spk.set_format_data(mt.Format(), mt.FormatLength());
  }
*/
  if (subtype == MEDIASUBTYPE_Vorbis2 &&
      formattype == FORMAT_Vorbis2 &&
      mt.FormatLength() > sizeof(VORBISFORMAT2))
  {
    VORBISFORMAT2 *format = (VORBISFORMAT2 *)mt.Format();
    spk = Speakers(FORMAT_VORBIS, 0, format->samplesPerSec);
    spk.set_format_data(mt.Format(), mt.FormatLength());
    return true;
  }

  /////////////////////////////////////////////////////////
  // LPCM

  if (subtype == MEDIASUBTYPE_DVD_LPCM_AUDIO)
  {
    PCMWAVEFORMAT *pcmwf = wf_cast<PCMWAVEFORMAT>(wf, wf_size);
    if (!pcmwf) return false;

    int format, mode;
    switch (pcmwf->wBitsPerSample)
    {
      case 16: format = FORMAT_PCM16_BE; break;
      case 20: format = FORMAT_LPCM20; break;
      case 24: format = FORMAT_LPCM24; break;
      default: return false;
    }
    switch (pcmwf->wf.nChannels)
    {
      case 1: mode = MODE_MONO; break;
      case 2: mode = MODE_STEREO; break;
      default: return false;
    }
    spk = Speakers(format, mode, sample_rate);
    return true;
  }

  /////////////////////////////////////////////////////////
  // General WAVEFORMAT conversion

  spk = Speakers();
  if (wf)
    spk = wf2spk(wf, wf_size);
  return !spk.is_unknown();
}

bool spk2mt(Speakers spk, CMediaType &mt, int i)
{
  if (spk.format == FORMAT_SPDIF)
  {
    // SPDIF media types
    if (i < 0 || i >= 2)
      return false;

    std::auto_ptr<WAVEFORMATEX> wfe(spk2wfe(spk, 0));
    if (!wfe.get())
      return false;

    mt.SetType(&MEDIATYPE_Audio);
    mt.SetSubtype(i == 0? &MEDIASUBTYPE_DOLBY_AC3_SPDIF: &MEDIASUBTYPE_PCM);
    mt.SetFormatType(&FORMAT_WaveFormatEx);
    mt.SetFormat((BYTE*)wfe.get(), sizeof(WAVEFORMATEX) + wfe->cbSize);
    return true;
  }
  else if (FORMAT_MASK(spk.format) & FORMAT_CLASS_PCM)
  {
    // PCM media types
    std::auto_ptr<WAVEFORMATEX> wfe(spk2wfe(spk, i));
    if (!wfe.get())
      return false;

    mt.SetType(&MEDIATYPE_Audio);
    mt.SetSubtype(&MEDIASUBTYPE_PCM);
    mt.SetFormatType(&FORMAT_WaveFormatEx);
    mt.SetFormat((BYTE*)wfe.get(), sizeof(WAVEFORMATEX) + wfe->cbSize);
    return true;
  }
  else
    return false;
}
