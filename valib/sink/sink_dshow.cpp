#include <memory>
#include "sink_dshow.h"
#include "../win32/winspk.h"
#include "../win32/dshow_spk.h"
#include "../win32/hresult_exception.h"
#include "../log.h"

static const string log_module = "DShowSink";

DShowSink::DShowSink(CTransformFilter *pTransformFilter, HRESULT * phr)
: CTransformOutputPin(NAME("DShowSink"), pTransformFilter, phr, L"Out")
{
  send_mt = false;
  send_dc = false;
  preroll = false;
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
DShowSink::can_open(Speakers _spk) const
{
  if (*m_mt.FormatType() == FORMAT_WaveFormatEx)
    if (is_compatible(_spk, (WAVEFORMAT*)m_mt.Format(), m_mt.FormatLength()))
      return true;

  CMediaType mt;
  int i = 0;
  while (spk2mt(_spk, mt, i++))
    if (query_downstream(&mt))
    {
      valib_log(log_event, log_module, "can_open(%s[%i]): Ok", _spk.print().c_str(), i);
      return true;
    }

  if (i == 0)
    valib_log(log_event, log_module, "can_open(%s): cannot convert to media type", _spk.print().c_str());
  else
    valib_log(log_event, log_module, "can_open(%s): format refused by downstream", _spk.print().c_str());
  return false;
}

bool 
DShowSink::init()
{
  if (*m_mt.FormatType() == FORMAT_WaveFormatEx)
    if (is_compatible(spk, (WAVEFORMAT *)m_mt.Format(), m_mt.FormatLength()))
      return true;

  CMediaType mt;
  int i = 0;
  while (spk2mt(spk, mt, i++))
    if (set_downstream(&mt))
    {
      valib_log(log_event, log_module, "open(%s[%i]): Ok %s", spk.print().c_str(), i, send_mt? "(send mediatype)": "");
      return true;
    }

  if (i == 0)
    valib_log(log_event, log_module, "open(%s): cannot convert to media type", spk.print().c_str());
  else
    valib_log(log_event, log_module, "open(%s): format refused by downstream", spk.print().c_str());
  return false;
};

void 
DShowSink::process(const Chunk &chunk)
{
  if (!m_Connected)
  {
    hr = VFW_E_NOT_CONNECTED;
    THROW(Error() << errinfo_hresult(hr));
    return;
  }

  IMediaSample *sample;

  uint8_t *sample_buf;
  long sample_size;

  uint8_t *chunk_buf = chunk.rawdata;
  size_t chunk_size = chunk.size;

  hr = E_FAIL;
  while (chunk_size)
  {
    // Allocate output sample
    if FAILED(hr = GetDeliveryBuffer(&sample, 0, 0, 0))
      THROW(Error() <<
        boost::errinfo_api_function("CBaseOutputPin::GetDeliveryBuffer()") <<
        errinfo_hresult(hr));

    // Dynamic format change
    if (send_mt)
    {
      valib_log(log_event, log_module, "process(): Sending media type...");
      query_downstream(&m_mt);
      sample->SetMediaType(&m_mt);
      send_mt = false;
    }

    // Discontinuity
    if (send_dc)
    {
      valib_log(log_event, log_module, "process(): Sending discontiniuity...");
      sample->SetDiscontinuity(true);
      send_dc = false;
    }
    
    // Preroll
    if (preroll)
      sample->SetPreroll(true);
    
    // Other sample flags
    sample->SetSyncPoint(true);
    sample->SetMediaTime(0, 0);

    // Data
    sample->GetPointer((BYTE**)&sample_buf);
    sample_size = (long)MIN((size_t)sample->GetSize(), chunk_size);
    if FAILED(hr = sample->SetActualDataLength(sample_size))
    {
      sample->Release();
      THROW(Error() <<
        boost::errinfo_api_function("IMediaSample::SetActualDataLength()") <<
        errinfo_hresult(hr));
    }
    memcpy(sample_buf, chunk_buf, sample_size);
    chunk_buf  += sample_size;
    chunk_size -= sample_size;

    // Timestamp
    // (uses sample_size determined before)
    if (chunk.sync)
    {
      REFERENCE_TIME begin = __int64(chunk.time * 10000000);

      WAVEFORMATEX *wfx = (WAVEFORMATEX *)m_mt.Format();
      if (wfx->nAvgBytesPerSec)
      {
        vtime_t len = vtime_t(sample_size) / wfx->nAvgBytesPerSec;
        REFERENCE_TIME end   = __int64(chunk.time * 10000000) + __int64(len * 10000000);
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
      THROW(Error() <<
        boost::errinfo_api_function("CBaseOutputPin::Deliver()") <<
        errinfo_hresult(hr));
  }

  hr = S_OK;
}
