#include "noise.h"

#define a 16807         /* multiplier */
#define m 2147483647L   /* 2**31 - 1 */
#define q 127773L       /* m div a */
#define r 2836          /* m mod a */


RNG::RNG(long _seed)
{
  set(_seed); 
};

void 
RNG::set(long _seed)
{
  seed = _seed;
}

long 
RNG::next()
{
  unsigned long lo, hi;

  lo = a * (long)(seed & 0xFFFF);
  hi = a * (long)((unsigned long)seed >> 16);
  lo += (hi & 0x7FFF) << 16;
  if (lo > m)
  {
    lo &= m;
    ++lo;
  }
  lo += hi >> 15;
  if (lo > m)
  {
    lo &= m;
    ++lo;
  }
  return (long)lo;
}

long 
RNG::get()
{
  return seed = next();
}




Noise::Noise(Speakers _spk, size_t _data_size, size_t _buf_size)
:spk(unk_spk), data_size(0), buf_size(0)
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
Noise::set_output(Speakers _spk, size_t _data_size, size_t _buf_size)
{
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
  size_t n = buf_size;

  bool eos = false;
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

    for (size_t s = 0; s < n; s++)
      for (size_t ch = 0; ch < nch; ch++)
        samples[ch][s] = double(rng.get()) / m;

    _chunk->set(spk, buf.get_samples(), n, false, 0, eos);
    return true;
  }
  else
  {
    // 32bit fill
    uint32_t *pos32 = (uint32_t *)buf.get_rawdata();
    uint32_t *end32 = pos32 + n / 4;
    while (pos32 < end32)
      *pos32++ = rng.get();

    // 8bit fill
    switch (n&3)
    {
      case 3: *pos32 = (*pos32 & 0xff000000) | (rng.get() & 0x00ffffff); break;
      case 2: *pos32 = (*pos32 & 0xffff0000) | (rng.get() & 0x0000ffff); break;
      case 1: *pos32 = (*pos32 & 0xffffff00) | (rng.get() & 0x000000ff); break;
    }

    _chunk->set(spk, buf.get_rawdata(), n, false, 0, eos);
    return true;
  }
}
