#include <dsound.h>
#include <mmreg.h>
#include <ks.h>
#include <ksmedia.h>
#include <math.h>
#include "sink_dsound.h"
#include "win32\winspk.h"

#define SAFE_DELETE(p)  { if (p) delete p; p = 0; }
#define SAFE_RELEASE(p) { if (p) p->Release(); p = 0; }


#define MAX_CACHE 256
class SpeakersCache
{
protected:
  Speakers allow_cache[MAX_CACHE];
  Speakers deny_cache[MAX_CACHE];
  int      allow_cache_size;
  int      deny_cache_size;

public:
  SpeakersCache(): allow_cache_size(0), deny_cache_size(0) {};

  bool is_allowed(Speakers spk)
  {
    for (int i = 0; i < allow_cache_size; i++)
      if (spk == allow_cache[i])
        return true;
    return false;
  }

  bool is_denied(Speakers spk)
  {
    for (int i = 0; i < deny_cache_size; i++)
      if (spk == deny_cache[i])
        return true;
    return false;
  }

  void allow(Speakers spk)
  {
    if (allow_cache_size < MAX_CACHE)
    {
      allow_cache[allow_cache_size] = spk;
      allow_cache_size++;
    }
  }

  void deny(Speakers spk)
  {
    if (deny_cache_size < MAX_CACHE)
    {
      deny_cache[deny_cache_size] = spk;
      deny_cache_size++;
    }
  }

};

SpeakersCache ds_cache;


DSoundSink::DSoundSink(HWND _hwnd, int _ds_buf_size_ms, int _preload_ms)
{
  hwnd = _hwnd;
  if (!hwnd) hwnd = GetForegroundWindow();
  if (!hwnd) hwnd = GetDesktopWindow();

  ds       = 0;
  ds_buf   = 0;
#ifdef DSOUND_SINK_PRIMARY_BUFFER
  ds_buf_prim = 0;
#endif
  memset(&wfx, 0, sizeof(wfx));

  buf_size     = 0;
  buf_size_ms  = _ds_buf_size_ms;
  preload_size = 0;
  preload_ms   = _preload_ms;
  cur          = 0;
  time         = 0;

  playing  = false;
  paused   = true;

  vol = 1.0;
  pan = 0;

  if FAILED(DirectSoundCreate(0, &ds, 0))
  {
    ds = 0;
    return;
  }

  if FAILED(ds->SetCooperativeLevel(hwnd, DSSCL_PRIORITY))
  {
    SAFE_RELEASE(ds);
    return;
  }
#ifdef DSoundSink_PRIMARY_BUFFER
  DSBUFFERDESC dsbdesc;
  ZeroMemory(&dsbdesc, sizeof(DSBUFFERDESC));
  dsbdesc.dwSize  = sizeof(DSBUFFERDESC);
  dsbdesc.dwFlags = DSBCAPS_PRIMARYBUFFER;
 
  if FAILED(ds->CreateSoundBuffer(&dsbdesc, &ds_buf_prim, 0))
  {
    SAFE_RELEASE(ds);
    return;
  }
#endif
}

DSoundSink::~DSoundSink()
{
  close();
  if (ds) ds->Release();
}




bool 
DSoundSink::query(Speakers _spk) const
{
  if (!ds) return false;

  if (ds_cache.is_allowed(_spk)) return true;
  if (ds_cache.is_denied (_spk)) return false;

  IDirectSoundBuffer  *ds_buf_test;
  WAVEFORMATEXTENSIBLE wfx_test;

  if (!spk2wfx(_spk, (WAVEFORMATEX *)&wfx_test, true))
  {
    ds_cache.deny(_spk);
    return false;
  }

  DSBUFFERDESC dsbdesc;
  ZeroMemory(&dsbdesc, sizeof(DSBUFFERDESC));
  dsbdesc.dwSize        = sizeof(DSBUFFERDESC);
  dsbdesc.dwFlags       = DSBCAPS_GETCURRENTPOSITION2 | DSBCAPS_GLOBALFOCUS;
  if (!_spk.is_spdif())
    dsbdesc.dwFlags    |= DSBCAPS_CTRLVOLUME | DSBCAPS_CTRLPAN;
  dsbdesc.dwBufferBytes = buf_size_ms * wfx_test.Format.nSamplesPerSec * wfx_test.Format.nBlockAlign / 1000;
  dsbdesc.lpwfxFormat   = (WAVEFORMATEX *)&wfx_test;

  if FAILED(ds->CreateSoundBuffer(&dsbdesc, &ds_buf_test, 0)) 
  {
    ds_cache.deny(_spk);
    return false;
  }

  ds_buf_test->Release();
  ds_cache.allow(_spk);
  return true;
}

bool 
DSoundSink::open(Speakers _spk)
{
  if (!ds) return false;
  if (!query(_spk)) return false; // update cache

  AutoLock autolock(&lock);

  if (ds_buf) close();

  spk = _spk;
  if (!spk2wfx(spk, (WAVEFORMATEX *)&wfx, true))
    return false;

  buf_size = buf_size_ms * wfx.Format.nSamplesPerSec * wfx.Format.nBlockAlign / 1000;
  preload_size = preload_ms * wfx.Format.nSamplesPerSec * wfx.Format.nBlockAlign / 1000;
  if (preload_size > buf_size / 2)
  {
    buf_size = preload_size * 2;
    buf_size_ms = preload_ms * 2;
  }

  DSBUFFERDESC dsbdesc;
  ZeroMemory(&dsbdesc, sizeof(DSBUFFERDESC));
  dsbdesc.dwSize        = sizeof(DSBUFFERDESC);
  dsbdesc.dwFlags       = DSBCAPS_GETCURRENTPOSITION2 | DSBCAPS_GLOBALFOCUS;
  if (!spk.is_spdif())
    dsbdesc.dwFlags    |= DSBCAPS_CTRLVOLUME | DSBCAPS_CTRLPAN;
  dsbdesc.dwBufferBytes = buf_size;
  dsbdesc.lpwfxFormat   = (WAVEFORMATEX *)&wfx;

  if FAILED(ds->CreateSoundBuffer(&dsbdesc, &ds_buf, 0)) return false;

  void *data;
  DWORD data_bytes;
  if FAILED(ds_buf->Lock(0, buf_size, &data, &data_bytes, 0, 0, 0)) 
  {
    SAFE_RELEASE(ds_buf);
    return false;
  }

  // Zero buffer???
  ZeroMemory(data, data_bytes);
  if FAILED(ds_buf->Unlock(data, data_bytes, 0, 0))
  {
    SAFE_RELEASE(ds_buf);
    return false;
  }

  ds_buf->SetCurrentPosition(0);
  playing = false;
  paused = false;
  cur = 0;

  set_vol(vol);
  set_pan(pan);

  return true;
}

void 
DSoundSink::close()
{
  AutoLock autolock(&lock);

  if (ds_buf) SAFE_RELEASE(ds_buf);
}

bool 
DSoundSink::is_open() const
{
  return ds_buf != 0;
}

Speakers
DSoundSink::get_spk() const
{
  return spk;
}




bool 
DSoundSink::is_time() const
{
  return true;
}

vtime_t
DSoundSink::get_time() const
{
  if (!ds_buf) return 0;
  DWORD play_cur, write_cur;
  ds_buf->GetCurrentPosition(&play_cur, &write_cur);
  if (play_cur < cur)
    return time - (cur - play_cur) / wfx.Format.nBlockAlign;
  else if (play_cur > cur)
    return time - (buf_size - play_cur + cur) / wfx.Format.nBlockAlign;
  else
    return 0;
}



void 
DSoundSink::stop()
{
  if (!ds_buf) return;
  AutoLock autolock(&lock);

  ds_buf->Stop();
  ds_buf->SetCurrentPosition(0);
  playing = false;
  cur = 0;
}

void 
DSoundSink::flush()
{
  if (!ds_buf) return;
  AutoLock autolock(&lock);

  if (!playing) 
    return;

  void *data1, *data2;
  DWORD data1_bytes, data2_bytes;
  DWORD play_cur;
  size_t data_size;

  // Zero rest of the buffer
  if FAILED(ds_buf->GetCurrentPosition(&play_cur, 0))
    return;

  data_size = play_cur - cur;
  if (data_size < 0)
    data_size += buf_size;

  if FAILED(ds_buf->Lock(cur, data_size, &data1, &data1_bytes, &data2, &data2_bytes, 0))
    return;

  memset(data1, 0, data1_bytes);
  if (data2_bytes)
    memset(data2, 0, data2_bytes);

  if FAILED(ds_buf->Unlock(data1, data1_bytes, data2, data2_bytes))
    return;

  // Sleep until end of playback
  data_size = buf_size - data_size;
  Sleep(data_size / (wfx.Format.nBlockAlign * wfx.Format.nSamplesPerSec / 1000) + 1);

  // Stop the playback
  ds_buf->Stop();
  ds_buf->SetCurrentPosition(0);
  playing = false;
  cur = 0;
}

void DSoundSink::pause()
{
  if (!ds_buf) return;
  AutoLock autolock(&lock);

  ds_buf->Stop();
  paused = true;
}

void DSoundSink::unpause()
{
  AutoLock autolock(&lock);

  if (!ds_buf) return;
  if (playing)
    ds_buf->Play(0, 0, DSBPLAY_LOOPING);
  paused = false;
}

bool 
DSoundSink::is_paused() const
{
  return paused;
}



bool
DSoundSink::is_vol() const
{
  return true;
}

double
DSoundSink::get_vol() const
{
  return vol;
}

void 
DSoundSink::set_vol(double _vol)
{
  AutoLock autolock(&lock);

  vol = _vol;
  if (ds_buf)
  {
    // Convert volume [0;1] to decibels
    // In DirectSOund volume is specified in hundredths of decibels
    // Zero volume is converted to -100dB

    int v = -10000;
    if (vol > 0)
      v = int(log(vol) * 2000);
    else
      vol = 0;
    ds_buf->SetVolume(v);
  }
}

bool 
DSoundSink::is_pan() const
{
  return true;
}

double 
DSoundSink::get_pan() const
{
  return pan;
}

void 
DSoundSink::set_pan(double _pan)
{
  AutoLock autolock(&lock);

  pan = _pan;

  if (ds_buf)
  {
    // Convert value [-128;+128] to decibels
    // The volume is specified in hundredths of decibels
    // Boundaries are converted to +/-100dB
    int p = 0;
    if (pan >= 1.0)       p = +10000;
    else if (pan <= -1.0) p = -10000;
    else if (pan > 0)     p = int(-log(1 - pan) * 2000);
    else if (pan < 0)     p = int(+log(1 + pan) * 2000);
    ds_buf->SetPan(p);
  }
}


size_t 
DSoundSink::get_buffer_size() const
{
  if (!ds_buf) return false;
  DWORD play_cur;

  ds_buf->GetCurrentPosition(&play_cur, 0);
  if (play_cur > cur)
    return play_cur - cur;
  else if (play_cur < cur)
    return play_cur + buf_size - cur;
  else // play_cur = cur
    if (!playing) 
      return 0;
    else 
      return buf_size;
}

bool DSoundSink::write(const Chunk *chunk)
{
  AutoLock autolock(&lock);

  if (!ds_buf) return false;

  void *data1, *data2;
  DWORD data1_bytes, data2_bytes;
  DWORD play_cur;
  size_t data_size;

  size_t  size = chunk->get_size();
  uint8_t *buf = chunk->get_rawdata();

  while (size)
  {
    if FAILED(ds_buf->GetCurrentPosition(&play_cur, 0))
      return false;

    data_size = buf_size + play_cur - cur;
    if (data_size >= buf_size)
      data_size -= buf_size;

    if (!playing && !data_size)
      data_size = buf_size;

    if (data_size > size)
      data_size = size;
    else
      if (playing && data_size < buf_size / 2)
      {
        // now we have no enough space in buffer, so sleep until we have
        data_size = min(size, buf_size / 2) - data_size;
        Sleep(data_size / (wfx.Format.nBlockAlign * wfx.Format.nSamplesPerSec / 1000) + 1);
        continue;
      }

    if FAILED(ds_buf->Lock(cur, data_size, &data1, &data1_bytes, &data2, &data2_bytes, 0))
      return false;

    memcpy(data1, buf, data1_bytes);
    buf += data1_bytes;
    size -= data1_bytes;
    cur += data1_bytes;

    if (data2_bytes)
    {
      memcpy(data2, buf, data2_bytes);
      buf += data2_bytes;
      size -= data2_bytes;
      cur += data2_bytes;
    }

    if FAILED(ds_buf->Unlock(data1, data1_bytes, data2, data2_bytes))
      return false;

    if (!playing && cur > preload_size)
    {
      if (!paused)
        ds_buf->Play(0, 0, DSBPLAY_LOOPING);
      playing = true;
    }

    if (cur >= buf_size)
      cur -= buf_size;
  }

  size = chunk->get_size() / wfx.Format.nBlockAlign;
  if (chunk->is_sync())
    time = chunk->get_time() + size;
  else
    time += size;
  return true;
}
