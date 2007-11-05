#include "sink_dshow.h"
#include "../win32/winspk.h"

DEFINE_GUID(MEDIASUBTYPE_AVI_AC3, 
0x00002000, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71);

DEFINE_GUID(MEDIASUBTYPE_AVI_DTS, 
0x00002001, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71);

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
  int sample_rate = 0;
  if (*mt.FormatType() == FORMAT_WaveFormatEx)
    sample_rate = ((WAVEFORMATEX *)mt.Format())->nSamplesPerSec;

  if ((*mt.Type() == MEDIATYPE_MPEG2_PES) ||
      (*mt.Type() == MEDIATYPE_DVD_ENCRYPTED_PACK))
  {
    spk = Speakers(FORMAT_PES, 0, sample_rate);
    return true;
  }

  if (*mt.Subtype() == MEDIASUBTYPE_DOLBY_AC3 || 
      *mt.Subtype() == MEDIASUBTYPE_AVI_AC3)
  {
    spk = Speakers(FORMAT_AC3, 0, sample_rate);
    return true;
  }

  if (*mt.Subtype() == MEDIASUBTYPE_DTS || 
      *mt.Subtype() == MEDIASUBTYPE_AVI_DTS)
  {
    spk = Speakers(FORMAT_DTS, 0, sample_rate);
    return true;
  }

  if (*mt.Subtype() == MEDIASUBTYPE_MPEG1AudioPayload)
  {
    spk = Speakers(FORMAT_MPA, 0, sample_rate);
    return true;
  }

  if (*mt.Subtype() == MEDIASUBTYPE_MPEG2_AUDIO)
  {
    spk = Speakers(FORMAT_MPA, 0, sample_rate);
    return true;
  }

  if (*mt.Subtype() == MEDIASUBTYPE_DOLBY_AC3_SPDIF)
  {
    spk = Speakers(FORMAT_SPDIF, 0, sample_rate);
    return true;
  }

  if (*mt.FormatType() != FORMAT_WaveFormatEx)
    return false;

  if (!wfx2spk((WAVEFORMATEX *)mt.Format(), spk))
    return false;

  // DVD LPCM uses big-endian format
  if (*mt.Subtype() == MEDIASUBTYPE_DVD_LPCM_AUDIO)
    switch (spk.format)
    {
      case FORMAT_PCM16: spk.format = FORMAT_PCM16_BE; break;
      case FORMAT_PCM24: spk.format = FORMAT_PCM24_BE; break;
      case FORMAT_PCM32: spk.format = FORMAT_PCM32_BE; break;
      case FORMAT_PCMFLOAT: spk.format = FORMAT_PCMFLOAT; break;
    }

  return true;
}

bool spk2mt(Speakers spk, CMediaType &mt, bool use_wfx)
{
  WAVEFORMATEXTENSIBLE wfx;

  if (spk.format == FORMAT_SPDIF)
  {
    // SPDIF media types

    mt.SetType(&MEDIATYPE_Audio);
    if (!spk2wfx(spk, (WAVEFORMATEX *)&wfx, false))
      return false;

    if (use_wfx)
      mt.SetSubtype(&MEDIASUBTYPE_PCM);
    else
      mt.SetSubtype(&MEDIASUBTYPE_DOLBY_AC3_SPDIF);
  }
  else
  {
    // PCM Media types

    mt.SetType(&MEDIATYPE_Audio);
    mt.SetSubtype(&MEDIASUBTYPE_PCM);
    if (!spk2wfx(spk, (WAVEFORMATEX *)&wfx, use_wfx))
      return false;
  }

  mt.SetFormatType(&FORMAT_WaveFormatEx);
  mt.SetFormat((BYTE*)&wfx, sizeof(WAVEFORMATEX) + wfx.Format.cbSize);
  return true;
};



DShowSink::DShowSink(CTransformFilter *pTransformFilter, HRESULT * phr)
: CTransformOutputPin(NAME("DShowSink"), pTransformFilter, phr, L"Out")
{
  DbgLog((LOG_TRACE, 3, "DShowSink(%x)::DShowSink()", this));

  spk = spk_unknown;
  send_mt = false;
  send_dc = false;
  hr = S_OK;
}

///////////////////////////////////////////////////////////////////////////////
// CheckMediaType(), SetMediaType(), query_spk() and set_spk() functions must
// guarantee that media type and spk values are always in sync and media type
// is allowed by DirectShow rules.

HRESULT 
DShowSink::CheckMediaType(const CMediaType *_mt)
{
  // verified by filter
  return CTransformOutputPin::CheckMediaType(_mt);
}

HRESULT 
DShowSink::SetMediaType(const CMediaType *_mt)
{
  spk = spk_unknown; // sink must be reinitialized

  return CTransformOutputPin::SetMediaType(_mt);
}

bool 
DShowSink::query_downstream(const CMediaType *_mt) const
{
  // We cannot join query_downstream() and set_downstream() functions
  // because query function must be const.

  IPinConnection *connection;

  if (!m_Connected) 
    return true;

  if (*_mt == m_mt)
    return true;

  m_Connected->QueryInterface(IID_IPinConnection, (void **)&connection);
  if (connection) 
  {
    // Try DynamicQueryAccept
    if (connection->DynamicQueryAccept(_mt) == S_OK)
    {
      connection->Release();
      return true;
    }
    connection->Release();
  }

  // Try QueryAccept
  if (m_Connected->QueryAccept(_mt) == S_OK)
  {
    m_Connected->QueryAccept(&m_mt);
    return true;
  }

  return false;
}

bool 
DShowSink::set_downstream(const CMediaType *_mt)
{
  // We cannot join query_downstream() and set_downstream() functions
  // because query function must be const.

  IPinConnection *connection;

  if (!m_Connected)
  {
    m_mt = *_mt;
    return true;
  }

  if (*_mt == m_mt)
    return true;

  m_Connected->QueryInterface(IID_IPinConnection, (void **)&connection);
  if (connection) 
  {
    // Try DynamicQueryAccept
    if (connection->DynamicQueryAccept(_mt) == S_OK)
    {
      connection->Release();
      m_mt = *_mt;
      send_mt = true;
      return true;
    }
    connection->Release();
  }

  // Try QueryAccept
  if (m_Connected->QueryAccept(_mt) == S_OK)
  {
    m_mt = *_mt;
    send_mt = true;
    return true;
  }

  return false;
}

// Sink interface
bool 
DShowSink::query_input(Speakers _spk) const
{
  if (*m_mt.FormatType() == FORMAT_WaveFormatEx)
    if (is_compatible(_spk, (WAVEFORMATEX *)m_mt.Format()))
      return true;

  CMediaType mt;
  if (spk2mt(_spk, mt, true) && query_downstream(&mt))
  {
    DbgLog((LOG_TRACE, 3, "DShowSink(%x)::query_input(%s %s %iHz extensible): Ok", this, _spk.mode_text(), _spk.format_text(), _spk.sample_rate));
    return true;
  }
  else if (spk2mt(_spk, mt, false) && query_downstream(&mt))
  {
    DbgLog((LOG_TRACE, 3, "DShowSink(%x)::query_input(%s %s %iHz): Ok", this, _spk.mode_text(), _spk.format_text(), _spk.sample_rate));
    return true;
  }
  else
  {
    DbgLog((LOG_TRACE, 3, "DShowSink(%x)::query_input(%s %s %iHz): format refused", this, _spk.mode_text(), _spk.format_text(), _spk.sample_rate));
    return false;
  }
}

bool 
DShowSink::set_input(Speakers _spk)
{
  if (*m_mt.FormatType() == FORMAT_WaveFormatEx)
    if (is_compatible(_spk, (WAVEFORMATEX *)m_mt.Format()))
    {
      spk = _spk;
      return true;
    }

  CMediaType mt;
  if (spk2mt(_spk, mt, true) && set_downstream(&mt))
  {
    spk = _spk;
    DbgLog((LOG_TRACE, 3, "DShowSink(%x)::set_input(%s %s %iHz extensible): Ok %s", this, _spk.mode_text(), _spk.format_text(), _spk.sample_rate, send_mt? "(send mediatype)": ""));
    return true;
  } 
  else if (spk2mt(_spk, mt, false) && set_downstream(&mt))
  {
    spk = _spk;
    DbgLog((LOG_TRACE, 3, "DShowSink(%x)::set_input(%s %s %iHz): Ok %s", this, _spk.mode_text(), _spk.format_text(), _spk.sample_rate, send_mt? "(send mediatype)": ""));
    return true;
  }
  else
  {
    spk = spk_unknown;
    DbgLog((LOG_TRACE, 3, "DShowSink(%x)::set_input(%s %s %iHz): Failed", this, _spk.mode_text(), _spk.format_text(), _spk.sample_rate));
    return false;
  }
};

Speakers
DShowSink::get_input() const
{
  return spk;
};

bool 
DShowSink::process(const Chunk *chunk)
{
  if (chunk->is_dummy())
    return true;

  // Process speaker configuraion changes
  if (spk != chunk->spk)
  {
    if (set_input(chunk->spk))
    {
      DbgLog((LOG_TRACE, 3, "DShowSink(%x)::process(): Speakers change (%s %s %iHz) OK", this, chunk->spk.mode_text(), chunk->spk.format_text(), chunk->spk.sample_rate));
    }
    else
    {
      DbgLog((LOG_TRACE, 3, "DShowSink(%x)::process(): Speakers change (%s %s %iHz) FAILAED!", this, chunk->spk.mode_text(), chunk->spk.format_text(), chunk->spk.sample_rate));
      return false;
    }
  }

  if (!m_Connected)
    return true;

  IMediaSample *sample;

  uint8_t *sample_buf;
  int sample_size;

  uint8_t *chunk_buf = chunk->rawdata;
  int chunk_size = chunk->size;

  while (chunk_size)
  {
    // Allocate output sample
    if FAILED(GetDeliveryBuffer(&sample, 0, 0, 0))
    {
      DbgLog((LOG_TRACE, 3, "DShowSink(%x)::process(): GetDeliveryBuffer failed!", this));
      return false;
    }

    // Dynamic format change
    if (send_mt)
    {
      DbgLog((LOG_TRACE, 3, "DShowSink(%x)::process(): Sending media type...", this));
      query_downstream(&m_mt);
      sample->SetMediaType(&m_mt);
      send_mt = false;
    }

    // Discontinuity
    if (send_dc)
    {
      DbgLog((LOG_TRACE, 3, "DShowSink(%x)::process(): Sending discontiniuity...", this));
      sample->SetDiscontinuity(true);
      send_dc = false;
    }
    
    // Other sample flags
    sample->SetSyncPoint(true);
    sample->SetMediaTime(0, 0);

    // Data
    sample->GetPointer((BYTE**)&sample_buf);
    sample_size = MIN(sample->GetSize(), chunk_size);
    if FAILED(sample->SetActualDataLength(sample_size))
    {
      sample->Release();
      return false;
    }
    memcpy(sample_buf, chunk_buf, sample_size);
    chunk_buf  += sample_size;
    chunk_size -= sample_size;

    // Timestamp
    // (uses sample_size determined before)
    if (chunk->sync)
    {
      REFERENCE_TIME begin = __int64(chunk->time * 10000000);

      WAVEFORMATEX *wfx = (WAVEFORMATEX *)m_mt.Format();
      if (wfx->nAvgBytesPerSec)
      {
        vtime_t len = vtime_t(sample_size) / wfx->nAvgBytesPerSec;
        REFERENCE_TIME end   = __int64(chunk->time * 10000000) + __int64(len * 10000000);
        sample->SetTime(&begin, &end);
      }
      else
        sample->SetTime(&begin, 0);
    }
    else
      sample->SetTime(0, 0);

    // Send
    hr = Deliver(sample);
    sample->Release();
    if FAILED(hr)
    {
      DbgLog((LOG_TRACE, 3, "DShowSink(%x)::process(): Deliver() FAILED! (0x%x)", this, hr));
      return false;
    }
  }
  return true;
}
