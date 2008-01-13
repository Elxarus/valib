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
  rng.seed(_seed);
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
    n = rng.get_range(-buf_size);

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
    samples_t samples = buf.get_samples();

    for (int ch = 0; ch < spk.nch(); ch++)
      rng.fill_samples(samples[ch], n);

    _chunk->set_linear(spk, samples, n, false, 0, eos);
    return true;
  }
  else
  {
    uint8_t *rawdata = buf.get_rawdata();
    rng.fill_raw(rawdata, n);
    _chunk->set_rawdata(spk, rawdata, n, false, 0, eos);
    return true;
  }
}
