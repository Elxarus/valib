#include <math.h>
#include "noise.h"
#include "generator.h"

/*
void
Generator::gen_samples(samples_t samples, int n)
{
  int ch, i;
  switch (type)
  {
    case GEN_ZERO:
      for (ch = 0; ch < spk.nch(); ch++)
        memset(samples[ch], 0, n * sizeof(sample_t));
      break;

    case GEN_NOISE:
      for (ch = 0; ch < spk.nch(); ch++)
        rng.fill_sample(samples[ch], n);
      break;

    case GEN_SINE:
      w = 2 * M_PI * freq / spk.sample_rate;
      for (i = 0; i < n; i++)
        samples[0][i] = sin(phase + i*w);
      phase += n*w;

      for (ch = 1; ch < spk.nch(); ch++)
        memcpy(samples[ch], samples[0], n * sizeof(sample_t));
      break;

    default: assert(false);
  }
}

void
Generator::gen_rawdata(uint8_t *rawdata, int n)
{
  int ch, i;
  uint8_t *rawdata = buf.get_rawdata();
  switch (type)
  {
    case GEN_ZERO:
      memset(rawdata, 0, n);
      return;

    case GEN_NOISE:
      rng.fill_rawdata(rawdata, n);
      return;

    case GEN_SINE:
      // Sine is not implemented for general rawdata case
      // todo: implement for PCM
      assert(false);

    default: assert(false);
  }
}
*/
///////////////////////////////////////////////////////////////////////////////
// Generator
// Base class for signal generation

Generator::Generator()
:spk(spk_unknown), stream_len(0), chunk_size(0)
{
}

Generator::Generator(Speakers _spk, size_t _stream_len, size_t _chunk_size)
:spk(spk_unknown), stream_len(0), chunk_size(0)
{
  setup(_spk, _stream_len, _chunk_size);
}

bool 
Generator::setup(Speakers _spk, size_t _stream_len, size_t _chunk_size)
{
  if (!_chunk_size)
    return false;

  if (!query_spk(_spk))
    return false;

  if (_spk.format == FORMAT_LINEAR)
  {
    if (!buf.allocate(_spk.nch(), _chunk_size))
      return false;

    spk = _spk;
    chunk_size = _chunk_size;
    stream_len = _stream_len;
  }
  else
  {
    if (!buf.allocate(_chunk_size))
      return false;

    spk = _spk;
    chunk_size = _chunk_size;
    stream_len = _stream_len;
  }
  return true;
}

// Source interface

Speakers 
Generator::get_output() const
{
  return spk;
}

bool 
Generator::is_empty() const
{
  return stream_len <= 0;
}

bool 
Generator::get_chunk(Chunk *_chunk)
{
  bool eos = false;
  size_t n = chunk_size;

  if (n >= stream_len)
  {
    n = stream_len;
    stream_len = 0;
    eos = true;
  }
  else
    stream_len -= n;

  if (spk.format == FORMAT_LINEAR)
  {
    samples_t samples = buf.get_samples();
    gen_samples(samples, n);
    _chunk->set_linear(spk, samples, n, false, 0, eos);
    return true;
  }
  else
  {
    uint8_t *rawdata = buf.get_rawdata();
    gen_rawdata(rawdata, n);
    _chunk->set_rawdata(spk, rawdata, n, false, 0, eos);
    return true;
  }
}

///////////////////////////////////////////////////////////////////////////////
// ZeroGen
// Silence generator

void
ZeroGen::gen_samples(samples_t samples, size_t n)
{
  for (int ch = 0; ch < spk.nch(); ch++)
    memset(samples[ch], 0, n * sizeof(sample_t));
}

void
ZeroGen::gen_rawdata(uint8_t *rawdata, size_t n)
{
  memset(rawdata, 0, n);
}

///////////////////////////////////////////////////////////////////////////////
// NoiseGen
// Noise generator

bool
NoiseGen::setup(Speakers _spk, int _seed, size_t _stream_len, size_t _chunk_size)
{
  rng.seed(_seed);
  return setup(_spk, _stream_len, _chunk_size);
}
void
NoiseGen::gen_samples(samples_t samples, size_t n)
{
  for (int ch = 0; ch < spk.nch(); ch++)
    rng.fill_samples(samples[ch], n);
}

void
NoiseGen::gen_rawdata(uint8_t *rawdata, size_t n)
{
  rng.fill_raw(rawdata, n);
}

///////////////////////////////////////////////////////////////////////////////
// ToneGen
// Tone generator

bool
ToneGen::query_spk(Speakers _spk) const
{
  return _spk.format == FORMAT_LINEAR;
}

bool
ToneGen::setup(Speakers _spk, int _freq, size_t _stream_len, size_t _chunk_size)
{
  phase = 0;
  freq = _freq;
  return setup(_spk, _stream_len, _chunk_size);
}
void
ToneGen::gen_samples(samples_t samples, size_t n)
{
  double w = 2 * M_PI * double(freq) / double(spk.sample_rate);
  for (size_t i = 0; i < n; i++)
    samples[0][i] = sin(phase + i*w);
  phase += n*w;

  for (int ch = 1; ch < spk.nch(); ch++)
    memcpy(samples[ch], samples[0], n * sizeof(sample_t));
}

void
ToneGen::gen_rawdata(uint8_t *rawdata, size_t n)
{
  // never be here
  assert(false);
}

