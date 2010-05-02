/*
  Signal generation sources
  Generator class is an abstract base for generators:
  * Silence generator (ZeroGen)
  * Noise generator (NoiseGen)
  * Sine wave generator (SineGen)
  * Line generator (LinGen)
*/

#ifndef VALIB_GENERATOR_H
#define VALIB_GENERATOR_H

#include "../source.h"
#include "../buffer.h"
#include "../rng.h"

class Generator;
class ZeroGen;
class NoiseGen;
class ToneGen;
class LineGen;



class Generator : public Source2
{
protected:
  Speakers  spk;
  SampleBuf samples;
  Rawdata   rawdata;
  size_t    chunk_size;
  uint64_t  stream_len;
  uint64_t  stream_pos;

  Generator::Generator():
  stream_len(0), chunk_size(0)
  {}

  Generator::Generator(Speakers spk_, uint64_t stream_len_, size_t chunk_size_):
  stream_len(0), chunk_size(0)
  { init(spk_, stream_len_, chunk_size_); }

  bool init(Speakers spk, uint64_t stream_len, size_t chunk_size = 4096);

  /////////////////////////////////////////////////////////
  // Interface to override

  virtual bool query_spk(Speakers spk) const { return true; }
  virtual void gen_samples(samples_t samples, size_t n) { assert(false); }
  virtual void gen_rawdata(uint8_t *rawdata, size_t n) { assert(false); }

public:
  size_t   get_chunk_size() const { return chunk_size; }
  uint64_t get_stream_len() const { return stream_len; }

  /////////////////////////////////////////////////////////
  // Source interface

  virtual bool get_chunk(Chunk2 &out);

  virtual void reset()
  {
    stream_pos = 0;
    reset_thunk();
  }

  virtual bool new_stream() const
  { return false; }

  virtual Speakers get_output() const
  { return spk; }
};

///////////////////////////////////////////////////////////////////////////////
// ZeroGen
// Silence generator

class ZeroGen : public Generator
{
protected:
  virtual void gen_samples(samples_t samples, size_t n)
  { zero_samples(samples, spk.nch(), n); }

  virtual void gen_rawdata(uint8_t *rawdata, size_t n)
  { memset(rawdata, 0, n); }

public:
  ZeroGen()
  {}

  ZeroGen(Speakers spk_, size_t stream_len_, size_t chunk_size_ = 4096):
  Generator(spk_, stream_len_, chunk_size_)
  {}

  bool init(Speakers spk_, uint64_t stream_len_, size_t chunk_size_ = 4096)
  { return Generator::init(spk_, stream_len_, chunk_size_); }

};

///////////////////////////////////////////////////////////////////////////////
// NoiseGen
// Noise generator

class NoiseGen : public Generator
{
protected:
  int seed;
  RNG rng;

  virtual void gen_samples(samples_t samples, size_t n)
  {
    for (size_t i = 0; i < n; i++)
      for (int ch = 0; ch < spk.nch(); ch++)
        samples[ch][i] = rng.get_sample();
  }

  virtual void gen_rawdata(uint8_t *rawdata, size_t n)
  { rng.fill_raw(rawdata, n); }

public:
  NoiseGen()
  {}

  NoiseGen(Speakers spk_, int seed_, uint64_t stream_len_, size_t chunk_size_ = 4096):
  Generator(spk_, stream_len_, chunk_size_), rng(seed_)
  {}

  bool init(Speakers spk_, int seed_, uint64_t stream_len_, size_t chunk_size_ = 4096)
  {
    seed = seed_;
    rng.seed(seed);
    return Generator::init(spk_, stream_len_, chunk_size_);
  }

  void reset()
  {
    rng.seed(seed);
    Generator::reset();
  }
};

///////////////////////////////////////////////////////////////////////////////
// ToneGen
// Tone generator

class ToneGen : public Generator
{
protected:
  double phase;
  double freq;
  double t;

  virtual bool query_spk(Speakers spk_) const
  { return spk_.format == FORMAT_LINEAR; }

  virtual void gen_samples(samples_t samples, size_t n);

public:
  ToneGen(): phase(0), freq(0)
  {}

  ToneGen(Speakers spk_, int _freq, double _phase, uint64_t stream_len_, size_t chunk_size_ = 4096):
  Generator(spk_, stream_len_, chunk_size_), phase(0), freq(0)
  { init(spk_, _freq, _phase, stream_len_, chunk_size_); }

  bool init(Speakers spk_, int freq_, double phase_, uint64_t stream_len_, size_t chunk_size_ = 4096)
  {
    phase = phase_;
    freq = freq_;
    t = phase;
    return Generator::init(spk_, stream_len_, chunk_size_);
  }

  void reset()
  {
    t = phase;
    Generator::reset();
  }
};

///////////////////////////////////////////////////////////////////////////////
// LineGen
// Line generator

class LineGen : public Generator
{
protected:
  double phase;
  double k;
  double t;

  virtual bool query_spk(Speakers spk_) const
  { return spk_.format == FORMAT_LINEAR; }

  virtual void gen_samples(samples_t samples, size_t n);

public:
  LineGen(): phase(0), k(1.0)
  {}

  LineGen(Speakers spk_, double start_, double k_, uint64_t stream_len_, size_t chunk_size_ = 4096):
  Generator(spk_, stream_len_, chunk_size_), phase(0), k(1.0)
  { init(spk_, start_, k_, stream_len_, chunk_size_); }

  bool init(Speakers spk_, double start_, double k_, uint64_t stream_len_, size_t chunk_size_ = 4096)
  {
    phase = start_;
    k = k_;
    t = phase;
    return Generator::init(spk_, stream_len_, chunk_size_);
  }

  void reset()
  {
    t = phase;
    Generator::reset();
  }
};

#endif
