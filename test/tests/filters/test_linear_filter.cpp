#include <math.h>
#include "source/generator.h"
#include "filters/linear_filter.h"
#include "../../suite.h"

const size_t data_size = 65536;
const size_t block_size = 4096;

inline bool equal_time(vtime_t time1, vtime_t time2, vtime_t max_diff = 1e-6)
{ return fabs(time1 - time2) < max_diff; }

///////////////////////////////////////////////////////////////////////////////
// Generator makes a signal where sample[x] = x

class SequenceGen : public Generator
{
protected:
  int seq;

  virtual bool query_spk(Speakers spk) const
  { return spk.format == FORMAT_LINEAR; }
  
  virtual void gen_samples(samples_t samples, size_t n)
  {
    for (size_t i = 0; i < n; i++)
      samples[0][i] = i + seq;
    for (int ch = 1; ch < spk.nch(); ch++)
      memcpy(samples[ch], samples[0], n * sizeof(sample_t));
    seq += n;
  }

  virtual void gen_rawdata(uint8_t *rawdata, size_t n)
  {}

public:
  SequenceGen(): seq(0) {};
  SequenceGen(Speakers spk, size_t stream_len, size_t chunk_size = 4096):
  Generator(spk, stream_len, chunk_size), seq(0)
  { init(spk, stream_len, chunk_size); }

  bool init(Speakers spk, size_t stream_len, size_t chunk_size = 4096)
  {
    seq = 0;
    return Generator::init(spk, stream_len, chunk_size);
  }
};

///////////////////////////////////////////////////////////////////////////////
// Linear filter that checks:
// * the correct sequence of calls
// * the correct sequence of data received

class LinearFilterTester : public LinearFilter
{
protected:
  mutable Speakers queried_spk;
  bool need_reset_after_init;
  bool need_reset_after_flushing;
  bool err;
  int seq;
  bool do_reinit;

  SampleBuf buf;
  int pos;

public:
  LinearFilterTester():
    queried_spk(spk_unknown),
    need_reset_after_init(false),
    need_reset_after_flushing(false),
    err(false), seq(0), pos(0)
  {}

  ~LinearFilterTester()
  {}

  void reinit() { do_reinit = true; }
  void reset_seq() { seq = 0; }

  virtual bool is_ok() const
  { return !err; }

  virtual bool query(Speakers spk) const
  {
    queried_spk = spk;
    return true;
  }

  virtual bool init(Speakers spk, Speakers &out_spk)
  {
    do_reinit = false;
    buf.allocate(spk.nch(), block_size * 2);
    pos = 0;

    out_spk = spk;
    if (queried_spk != spk)
      err = true;
    return true;
  }

  virtual void reset_state()
  {
    need_reset_after_init = false;
    need_reset_after_flushing = false;
    pos = 0;
  }

  virtual bool process_samples(samples_t in, size_t in_size, samples_t &out, size_t &out_size, size_t &gone)
  {
    if (int(in[0][0]) != seq)
      err = true;

    if (pos >= 2 * block_size)
    {
      for (int ch = 0; ch < in_spk.nch(); ch++)
        memmove(buf[ch], buf[ch] + block_size, block_size * sizeof(sample_t));
      pos = block_size;
    }

    size_t n = block_size * 2 - pos;
    if (in_size < n)
    {
      for (int ch = 0; ch < in_spk.nch(); ch++)
        memcpy(buf[ch] + pos, in[ch], in_size * sizeof(sample_t));

      out.zero();
      out_size = 0;
      gone = in_size;
      seq += in_size;
      pos += in_size;
    }
    else
    {
      for (int ch = 0; ch < in_spk.nch(); ch++)
        memcpy(buf[ch] + pos, in[ch], n * sizeof(sample_t));

      out = buf;
      out_size = block_size;
      gone = n;
      seq += n;
      pos += n;
    }
    return true;
  }

  virtual bool flush(samples_t &out, size_t &out_size)
  {
    if (!need_flushing())
      err = true;
    need_reset_after_flushing = true;

    if (pos >= 2 * block_size)
    {
      for (int ch = 0; ch < in_spk.nch(); ch++)
        memmove(buf[ch], buf[ch] + block_size, block_size * sizeof(sample_t));
      pos = block_size;
    }

    out = buf;
    out_size = pos;
    pos = 0;
    return true;
  }

  virtual bool need_flushing() const
  { return pos > 0; }

  virtual bool want_reinit() const
  { return do_reinit; }
};

///////////////////////////////////////////////////////////////////////////////
// Check:
// * Pass data to process_samples() in the correct order
// * Receive data from get_chunk() in the corrent order
// * Correct passing of timestamps

TEST(linear_filter, "LinearFilter test")
  Speakers spk(FORMAT_LINEAR, MODE_STEREO, 48000);

  const size_t block_multipliers[] =
  { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 13, 16 };
  const int nmultipliers = array_size(block_multipliers);
  bool was_reinit = false;

  Chunk chunk;
  SequenceGen gen;
  LinearFilterTester f;

  f.set_input(spk);
  CHECK(f.is_ok());

  for (int i = 0; i < nmultipliers * 2; i++)
  {
    int seq = 0;
    f.reset_seq();

    size_t current_chunk = 0;
    size_t current_block_size = i < nmultipliers? 
      block_size / block_multipliers[i]:
      block_size * block_multipliers[i - nmultipliers];

    gen.init(spk, data_size, current_block_size);
    while (!gen.is_empty())
    {
      if (seq >= block_size && !was_reinit)
      {
        f.reinit();
        was_reinit = true;
      }

      while (f.is_empty())
      {
        gen.get_chunk(&chunk);
        chunk.sync = true;
        chunk.time = current_chunk * 100;
        current_chunk++;

        if (gen.is_empty())
          CHECK(chunk.eos);
        f.process(&chunk);
      }
      CHECK(f.is_ok());

      while (!f.is_empty())
      {
        f.get_chunk(&chunk);
        CHECK(f.is_ok());
        if (chunk.size)
          CHECK(int(chunk.samples[0][0]) == seq);

        if (chunk.sync)
        {
          vtime_t chunk_time1 =
            vtime_t(seq / current_block_size) * 100 + 
            vtime_t(seq % current_block_size) / spk.sample_rate;
          vtime_t chunk_time2 = chunk_time1 - 100 + 
            vtime_t(current_block_size) / spk.sample_rate;

          CHECK(equal_time(chunk.time, chunk_time1) || equal_time(chunk.time, chunk_time2));
        }
        seq += chunk.size;
      }
    }

  }
TEST_END(linear_filter);
