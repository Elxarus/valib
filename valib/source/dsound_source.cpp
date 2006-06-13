#include <dsound.h>
#include <ks.h>
#include <ksmedia.h>
#include "dsound_source.h"
#include "win32\winspk.h"

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
  spk = spk_unknown;
  ds_capture = 0;
  ds_buf     = 0;
  buf_size   = 0;
  cur        = 0;
}

bool
DSoundSource::open(Speakers _spk, size_t _buf_size_ms, LPCGUID _device)
{
  WAVEFORMATEXTENSIBLE wfx;
  memset(&wfx, 0, sizeof(wfx));

  if (spk2wfx(_spk, (WAVEFORMATEX*)(&wfx), true))
    if (open((WAVEFORMATEX*)(&wfx), _buf_size_ms, _device))
      return true;

  if (spk2wfx(_spk, (WAVEFORMATEX*)&wfx, false))
    if (open((WAVEFORMATEX*)(&wfx), _buf_size_ms, _device))
      return true;

  spk = _spk;
  return false;
}

bool
DSoundSource::open(WAVEFORMATEX *wf, size_t _buf_size_ms, LPCGUID _device)
{
  buf_size = wf->nBlockAlign * wf->nSamplesPerSec * _buf_size_ms / 1000;
  if (!out_buf.allocate(buf_size))
    return false;

  // DirectSound buffer description
  DSCBUFFERDESC dscbd;
  memset(&dscbd, 0, sizeof(dscbd));
  dscbd.dwSize        = sizeof(dscbd);
  dscbd.dwBufferBytes = buf_size;
  dscbd.lpwfxFormat   = wf;

  // Create everything
  if FAILED(DirectSoundCaptureCreate(_device, &ds_capture, 0))
  {
    zero_all();
    return false;
  }

  if FAILED(ds_capture->CreateCaptureBuffer(&dscbd, &ds_buf, 0))
  {
    ds_capture->Release();
    zero_all();
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
DSoundSource::start()
{
  if (!ds_buf) return false;
  ds_buf->Start(DSCBSTART_LOOPING);
  return true;
}

void 
DSoundSource::stop()
{
  if (!ds_buf) return;
  ds_buf->Stop();
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

size_t 
DSoundSource::captured_ms() const
{
  return captured_size() * 1000 / spk.sample_rate;
}

Speakers
DSoundSource::get_output() const
{
  return spk;
}

bool
DSoundSource::is_empty() const
{
  if (chunk_size_ms)
    return captured_ms() < chunk_size_ms;
  else
    return ds_buf != 0;
}

bool 
DSoundSource::get_chunk(Chunk *_chunk)
{
  if (!ds_buf) return false;

  DWORD read_cur;
  DWORD data_size;
  void *data1;
  void *data2;
  DWORD len1;
  DWORD len2;

  if FAILED(ds_buf->GetCurrentPosition(0, &read_cur)) 
    return false;

  data_size = buf_size + read_cur - cur;
  if (data_size >= buf_size)
    data_size -= buf_size;

  if (!data_size)
  {
    _chunk->set_empty(spk);
    return true;
  }

  if FAILED(ds_buf->Lock(cur, data_size, &data1, &len1, &data2, &len2, 0))
    return false;

  memcpy(out_buf.get_data(), data1, len1);
  memcpy(out_buf.get_data() + len1, data2, len2);

  cur += len1 + len2;
  if (cur >= buf_size)
    cur -= buf_size;

  if FAILED(ds_buf->Unlock(data1, len1, data2, len2))
    return false;

  _chunk->set_rawdata(spk, out_buf.get_data(), len1 + len2);
  return true;
};
