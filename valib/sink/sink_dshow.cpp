#include "sink_dshow.h"
#include "win32\winspk.h"

DEFINE_GUID(MEDIASUBTYPE_AVI_AC3, 
0x00002000, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71);

DEFINE_GUID(MEDIASUBTYPE_AVI_DTS, 
0x00002001, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71);

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

  if (*mt.FormatType() == FORMAT_WaveFormatEx)
    return wfx2spk((WAVEFORMATEX *)mt.Format(), spk);
  else
    return false;
}

bool spk2mt(Speakers spk, CMediaType &mt, bool use_wfx)
{
  WAVEFORMATEXTENSIBLE wfx;
  if (!spk2wfx(spk, (WAVEFORMATEX *)&wfx, use_wfx))
    return false;

  mt.SetType(&MEDIATYPE_Audio);
  if (spk.format == FORMAT_SPDIF)
    mt.SetSubtype(&MEDIASUBTYPE_DOLBY_AC3_SPDIF);
  else
    mt.SetSubtype(&MEDIASUBTYPE_PCM);
  mt.SetFormatType(&FORMAT_WaveFormatEx);
  mt.SetFormat((BYTE*)&wfx, sizeof(WAVEFORMATEX) + wfx.Format.cbSize);
  return true;
};



DShowSink::DShowSink(CTransformFilter *pTransformFilter, HRESULT * phr)
: CTransformOutputPin(NAME("DShowSink"), pTransformFilter, phr, L"Out")
{
  DbgLog((LOG_TRACE, 3, "DShowSink(%x)::DShowSink()", this));

  spk = def_spk;
  send_mt = false;
  discontinuity = false;
}

bool 
DShowSink::query_downstream(Speakers _spk) const
{
  CMediaType mt;
  if (spk2mt(_spk, mt, true) && query_downstream(&mt))
  {
    DbgLog((LOG_TRACE, 3, "DShowSink(%x)::query_downstream(%s %s %iHz extensible): Ok", this, _spk.mode_text(), _spk.format_text(), _spk.sample_rate));
    return true;
  }
  else if (spk2mt(_spk, mt, false) && query_downstream(&mt))
  {
    DbgLog((LOG_TRACE, 3, "DShowSink(%x)::query_downstream(%s %s %iHz): Ok", this, _spk.mode_text(), _spk.format_text(), _spk.sample_rate));
    return true;
  }
  else
  {
    DbgLog((LOG_TRACE, 3, "DShowSink(%x)::query_downstream(%s %s %iHz): Failed", this, _spk.mode_text(), _spk.format_text(), _spk.sample_rate));
    return false;
  }
}

bool 
DShowSink::query_downstream(const CMediaType *mt) const
{
  IPinConnection *connection;

  if (!m_Connected) 
    return true;

  m_Connected->QueryInterface(IID_IPinConnection, (void **)&connection);
  if (connection) 
  {
    // Try DynamicQueryAccept
    if (connection->DynamicQueryAccept(mt) == S_OK)
    {
      connection->Release();
      return true;
    }
    connection->Release();
  }

  // Try QueryAccept
  if (m_Connected->QueryAccept(mt) == S_OK)
  {
    m_Connected->QueryAccept(&m_mt);
    return true;
  }

  return false;
}

// Sink interface
bool 
DShowSink::query_input(Speakers _spk) const
{
  DbgLog((LOG_TRACE, 3, "DShowSink(%x)::query_input(%s %s %iHz)", this, _spk.mode_text(), _spk.format_text(), _spk.sample_rate));
  return query_downstream(_spk);
}

bool 
DShowSink::set_input(Speakers _spk)
{
  DbgLog((LOG_TRACE, 3, "DShowSink(%x)::set_input(%s %s %iHz)", this, _spk.mode_text(), _spk.format_text(), _spk.sample_rate));
  return query_downstream(_spk);
};

bool 
DShowSink::process(const Chunk *chunk)
{
  if (chunk->is_empty())
    return true;

  // Process speaker configuraion changes
  if (spk != chunk->spk)
  {
    CMediaType mt;
    CMediaType mt_wfx;
    if (!spk2mt(chunk->spk, mt, false) || !spk2mt(chunk->spk, mt_wfx, true))
      return false;

    if (m_mt != mt && m_mt != mt_wfx)
    {
      DbgLog((LOG_TRACE, 3, "DShowSink(%x)::process(): Speakers change (%s %s %iHz)", this, chunk->spk.mode_text(), chunk->spk.format_text(), chunk->spk.sample_rate));

      if (query_downstream(&mt_wfx))
      {
        DbgLog((LOG_TRACE, 3, "DShowSink(%x)::process(): Speakers change Ok (extensible)", this));
        m_mt = mt_wfx;
      }
      else if (query_downstream(&mt))
      {
        DbgLog((LOG_TRACE, 3, "DShowSink(%x)::process(): Speakers change Ok", this));
        m_mt = mt;
      }
      else
      {
        DbgLog((LOG_TRACE, 3, "DShowSink(%x)::process(): Speakers change FAILED!", this));
        return false;
      }
      send_mt = true;
    }
    spk = chunk->spk;
  }

  if (!m_Connected)
    return true;

  IMediaSample *sample;

  uint8_t *sample_buf;
  int sample_size;

  uint8_t *chunk_buf = chunk->buf;
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
    if (discontinuity)
    {
      DbgLog((LOG_TRACE, 3, "DShowSink(%x)::process(): Sending discontiniuity...", this));
      sample->SetDiscontinuity(true);
      discontinuity = false;
    }

    // Timestamp
    if (chunk->timestamp)
    {
      REFERENCE_TIME begin = __int64(double(chunk->time) / spk.sample_rate * 10000000);
      REFERENCE_TIME end   = __int64(double(chunk->time + chunk->size) / spk.sample_rate * 10000000);
      sample->SetTime(&begin, &end);
      DbgLog((LOG_TRACE, 3, "<- timestamp: %ims\t%.0fsm", int(begin/10000), chunk->time));
    }
    else
      sample->SetTime(0, 0);

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

    // Send
    HRESULT hr = Deliver(sample);
    sample->Release();
    if FAILED(hr) 
      return false;
  }
  return true;
}
