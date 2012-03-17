#include <dsound.h>
#include <mmreg.h>
#include <ks.h>
#include <ksmedia.h>
#include "dsound_source.h"
#include "../win32/winspk.h"
#include "../win32/hresult_exception.h"

DSoundSource::DSoundSource()
{
  zero_all();
  chunk_size_ms = 0;
}

DSoundSource::DSoundSource(Speakers _spk, size_t _buf_size_ms, LPCGUID _device)
{
  zero_all();
  chunk_size_ms = 0;
  open(_spk, _buf_size_ms, _device);
}

DSoundSource::~DSoundSource()
{
  if (is_open())
    close();
}

void 
DSoundSource::zero_all()
{
  spk         = spk_unknown;
  buf_size    = 0;
  buf_size_ms = 0;
  bytes2time  = 0.0;

  ds_capture  = 0;
  ds_buf      = 0;

  capturing   = false;
  cur         = 0;
  time        = 0;
}

bool
DSoundSource::open(Speakers _spk, size_t _buf_size_ms, LPCGUID _device)
{
  spk = _spk;

  WAVEFORMATEXTENSIBLE wfx;
  memset(&wfx, 0, sizeof(wfx));

  if (spk2wfx(_spk, (WAVEFORMATEX*)(&wfx), true))
    if (open((WAVEFORMATEX*)(&wfx), _buf_size_ms, _device))
      return true;

  if (spk2wfx(_spk, (WAVEFORMATEX*)&wfx, false))
    if (open((WAVEFORMATEX*)(&wfx), _buf_size_ms, _device))
      return true;

  zero_all();
  return false;
}

bool
DSoundSource::open(WAVEFORMATEX *wf, size_t _buf_size_ms, LPCGUID _device)
{
  buf_size_ms = _buf_size_ms;
  buf_size = DWORD(wf->nBlockAlign * wf->nSamplesPerSec * buf_size_ms / 1000);
  bytes2time = 1.0 / wf->nAvgBytesPerSec;

  // DirectSound buffer description
  DSCBUFFERDESC dscbd;
  memset(&dscbd, 0, sizeof(dscbd));
  dscbd.dwSize        = sizeof(dscbd);
  dscbd.dwBufferBytes = buf_size;
  dscbd.lpwfxFormat   = wf;

  // Open DirectSound
  if FAILED(DirectSoundCaptureCreate(_device, &ds_capture, 0))
    return false;

  if FAILED(ds_capture->CreateCaptureBuffer(&dscbd, &ds_buf, 0))
  {
    ds_capture->Release();
    return false;
  }

  // Allocate audio buffer
  // We do this last because DirectSound allocation 
  // may fail and we may not require this buffer
  if (!out_buf.allocate(buf_size))
  {
    ds_buf->Release();
    ds_capture->Release();
    return false;
  }

  cur = 0;
  return true;
}

void
DSoundSource::close()
{
  if (ds_buf)
  {
    ds_buf->Stop();
    ds_buf->Release();
  }

  if (ds_capture)
    ds_capture->Release();

  zero_all();
}

bool
DSoundSource::is_open() const
{
  return ds_buf != 0;
}


bool
DSoundSource::is_started() const
{
  return capturing;
}

bool 
DSoundSource::start()
{
  if (!ds_buf) return false;

  time = 0;
  if SUCCEEDED(ds_buf->Start(DSCBSTART_LOOPING))
    capturing = true;

  return capturing;
}

void 
DSoundSource::stop()
{
  if (!ds_buf) return;

  ds_buf->Stop();
  capturing = false;
}

size_t 
DSoundSource::captured_size() const
{
  if (!ds_buf) return 0;
  DWORD read_cur;

  if FAILED(ds_buf->GetCurrentPosition(0, &read_cur))
    return 0;

  if (read_cur >= cur)
    return read_cur - cur;
  else
    return read_cur + buf_size - cur;
}

vtime_t
DSoundSource::captured_time() const
{
  return captured_size() * bytes2time;
}

///////////////////////////////////////////////////////////////////////////////
// Source interface

bool 
DSoundSource::get_chunk(Chunk &chunk)
{
  if (!ds_buf)
    return false;

  DWORD read_cur;
  DWORD data_size;
  void *data1;
  void *data2;
  DWORD len1;
  DWORD len2;
  HRESULT hr;

  if FAILED(hr = ds_buf->GetCurrentPosition(0, &read_cur)) 
    THROW(EDirectSound() << boost::errinfo_api_function("IDirectSoundCaptureBuffer::GetCurrentPosition()") << errinfo_hresult(hr));

  data_size = buf_size + read_cur - cur;
  if (data_size >= buf_size)
    data_size -= buf_size;

  if (!data_size)
  {
    chunk.clear();
    return true;
  }

  if FAILED(hr = ds_buf->Lock(cur, data_size, &data1, &len1, &data2, &len2, 0))
    THROW(EDirectSound() << boost::errinfo_api_function("IDirectSoundCaptureBuffer::Lock()") << errinfo_hresult(hr));

  memcpy(out_buf.begin(), data1, len1);
  memcpy(out_buf.begin() + len1, data2, len2);

  cur += len1 + len2;
  if (cur >= buf_size)
    cur -= buf_size;

  if FAILED(ds_buf->Unlock(data1, len1, data2, len2))
    THROW(EDirectSound() << boost::errinfo_api_function("IDirectSoundCaptureBuffer::Unlock()") << errinfo_hresult(hr));

  chunk.set_rawdata(out_buf.begin(), len1 + len2, true, time);
  time += (len1 + len2) * bytes2time;
  return true;
};
