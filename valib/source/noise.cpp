#include "noise.h"

#define abs(a) ((a) > 0? (a): -(a))

Noise::Noise()
:spk(spk_unknown), buf_size(0), data_size(0)
{
}

Noise::Noise(Speakers _spk, size_t _data_size, int _buf_size)
:spk(spk_unknown), buf_size(0), data_size(0)
{
  set_output(_spk, _data_size, _buf_size);
}

void 
Noise::set_seed(long _seed)
{
  rng.set(_seed);
}

size_t 
Noise::get_data_size()
{ 
  return data_size; 
}

void   
Noise::set_data_size(size_t _data_size)
{
  data_size = _data_size;
}


bool 
Noise::set_output(Speakers _spk, size_t _data_size, int _buf_size)
{
  if (!_buf_size)
    return false;

  if (_spk.format == FORMAT_LINEAR)
  {
    if (!buf.allocate(_spk.nch(), abs(_buf_size)))
      return false;

    spk = _spk;
    buf_size = _buf_size;
    data_size = _data_size;
  }
  else
  {
    if (!buf.allocate(abs(_buf_size)))
      return false;

    spk = _spk;
    buf_size = _buf_size;
    data_size = _data_size;
  }
  return true;
}

// Source interface

Speakers 
Noise::get_output() const
{
  return spk;
}

bool 
Noise::is_empty() const
{
  return data_size <= 0;
}

bool 
Noise::get_chunk(Chunk *_chunk)
{
  bool eos = false;
  size_t n;

  if (buf_size > 0)
    n = size_t(buf_size);
  else
    n = rng.get_uint(-buf_size);

  if (n >= data_size)
  {
    n = data_size;
    data_size = 0;
    eos = true;
  }
  else
    data_size -= n;

  if (spk.format == FORMAT_LINEAR)
  {
    size_t nch = spk.nch();
    samples_t samples = buf.get_samples();

    for (size_t ch = 0; ch < nch; ch++)
      for (size_t s = 0; s < n; s++)
        samples[ch][s] = rng.get_float();

    _chunk->set_linear(spk, buf.get_samples(), n, false, 0, eos);
    return true;
  }
  else
  {
    uint32_t *pos = (uint32_t *) buf.get_rawdata();
    uint32_t *end = pos + (n >> 2);

    // 32bit fill
    while (pos < end)
      *pos++ = rng.get_uint();

    // 8bit fill
    switch (n&3)
    {
      case 3: *pos = (*pos & 0xff000000) | (rng.get_uint() & 0x00ffffff); break;
      case 2: *pos = (*pos & 0xffff0000) | (rng.get_uint() & 0x0000ffff); break;
      case 1: *pos = (*pos & 0xffffff00) | (rng.get_uint() & 0x000000ff); break;
    }

    _chunk->set_rawdata(spk, buf.get_rawdata(), n, false, 0, eos);
    return true;
  }
}
