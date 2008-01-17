#include "zero_source.h"

ZeroSource::ZeroSource()
:spk(spk_unknown), buf_size(0), data_size(0)
{
}

ZeroSource::ZeroSource(Speakers _spk, size_t _data_size, size_t _buf_size)
:spk(spk_unknown), buf_size(0), data_size(0)
{
  set_output(_spk, _data_size, _buf_size);
}

size_t 
ZeroSource::get_data_size()
{ 
  return data_size; 
}

void   
ZeroSource::set_data_size(size_t _data_size)
{
  data_size = _data_size;
}


bool 
ZeroSource::set_output(Speakers _spk, size_t _data_size, size_t _buf_size)
{
  if (!_buf_size)
    return false;

  if (_spk.format == FORMAT_LINEAR)
  {
    if (!buf.allocate(_spk.nch(), _buf_size))
      return false;

    spk = _spk;
    buf_size = _buf_size;
    data_size = _data_size;
  }
  else
  {
    if (!buf.allocate(_buf_size))
      return false;

    spk = _spk;
    buf_size = _buf_size;
    data_size = _data_size;
  }
  return true;
}

// Source interface

Speakers 
ZeroSource::get_output() const
{
  return spk;
}

bool 
ZeroSource::is_empty() const
{
  return data_size <= 0;
}

bool 
ZeroSource::get_chunk(Chunk *_chunk)
{
  bool eos = false;
  size_t n = buf_size;

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
      memset(samples[ch], 0, n * sizeof(sample_t));

    _chunk->set_linear(spk, samples, n, false, 0, eos);
    return true;
  }
  else
  {
    uint8_t *rawdata = buf.get_rawdata();
    memset(rawdata, 0, n);
    _chunk->set_rawdata(spk, rawdata, n, false, 0, eos);
    return true;
  }
}
