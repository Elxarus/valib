/*
  Signal generation sources
  Generator class is an abstract base for generators:
  * Silence generator (ZeroGen)
  * Noise generator (NoiseGen)
  * Sine wave generator (SineGen)
*/

#ifndef VALIB_GENERATOR_H
#define VALIB_GENERATOR_H

#include "../filter.h"
#include "../data.h"
#include "../rng.h"

class Generator : public Source
{
protected:
  Speakers  spk;
  SampleBuf buf;
  size_t    chunk_size;
  size_t    stream_len;

  virtual bool query_spk(Speakers spk) const { return true; }
  virtual void gen_samples(samples_t samples, size_t n) { assert(false); }
  virtual void gen_rawdata(uint8_t *rawdata, size_t n) { assert(false); }

public:
  Generator();
  Generator(Speakers spk, size_t stream_len, size_t chunk_size = 4096);

  bool setup(Speakers spk, size_t stream_len, size_t chunk_size = 4096);
  size_t get_chunk_size() const  { return chunk_size;  }
  size_t get_stream_len() const { return stream_len; }

  // Source interface
  virtual Speakers get_output() const;
  virtual bool is_empty() const;
  virtual bool get_chunk(Chunk *chunk);
};

class ZeroGen : public Generator
{
protected:
  virtual void gen_samples(samples_t samples, size_t n);
  virtual void gen_rawdata(uint8_t *rawdata, size_t n);

public:
  ZeroGen() {};
  ZeroGen(Speakers _spk, size_t _stream_len, size_t _chunk_size = 4096)
  :Generator(_spk, _stream_len, _chunk_size) {}
};

class NoiseGen : public Generator
{
protected:
  RNG rng;
  virtual void gen_samples(samples_t samples, size_t n);
  virtual void gen_rawdata(uint8_t *rawdata, size_t n);

public:
  NoiseGen() {};
  NoiseGen(Speakers _spk, int _seed, size_t _stream_len, size_t _chunk_size = 4096)
  :Generator(_spk, _stream_len, _chunk_size), rng(_seed) {}

  bool setup(Speakers spk, int seed, size_t stream_len, size_t chunk_size = 4096);
};

class ToneGen : public Generator
{
protected:
  double phase;
  double freq;

  virtual bool query_spk(Speakers spk) const;
  virtual void gen_samples(samples_t samples, size_t n);
  virtual void gen_rawdata(uint8_t *rawdata, size_t n);

public:
  ToneGen(): phase(0), freq(0) {};
  ToneGen(Speakers _spk, int _freq, size_t _stream_len, size_t _chunk_size = 4096):
  Generator(_spk, _stream_len, _chunk_size), phase(0), freq(0)
  { setup(_spk, _freq, _stream_len, _chunk_size); }

  bool setup(Speakers spk, int freq, size_t stream_len, size_t chunk_size = 4096);
};

#endif
