#include <math.h>
#include "../../suite.h"
#include "buffer.h"
#include "source/generator.h"
#include "filters/gain.h"
#include "filters/filter_graph2.h"

static const Speakers spk = Speakers(FORMAT_LINEAR, MODE_STEREO, 48000);
static const size_t noise_size = 64 * 1024;
static const int seed = 123123;

///////////////////////////////////////////////////////////////////////////////
// GainGraph owns a filter
// Test correct construction and destruction

class GainGraph : public FilterGraph2
{
protected:
  Gain gain;
  static const int node_gain = 1;

  virtual int next_id(int id, Speakers spk) const
  {
    if (id == node_start) return gain.can_open(spk)? node_gain: node_err;
    if (id == node_gain) return node_end;
    return node_err;
  }
  virtual Filter2 *init_filter(int id, Speakers spk)
  {
    if (id == node_gain) return &gain;
    return 0;
  }

public:
  GainGraph(double gain_): gain(gain_) {};
  ~GainGraph() { destroy(); }

  void set_gain(double gain_) { gain.gain = gain_; }
};

///////////////////////////////////////////////////////////////////////////////
// LinearBuffer
// Simplest buffering filter. Fills the buffer and passes it through without
// any change. Requires flushing when some data is in the buffer.

class LinearBuffer : public SamplesFilter
{
protected:
  size_t buf_size;
  size_t buf_pos;
  SampleBuf buf;

public:
  LinearBuffer(size_t buf_size): buf_size(buf_size)
  {}

  virtual bool init(Speakers new_spk)
  {
    buf_pos = 0;
    return buf.allocate(new_spk.nch(), buf_size);
  }

  virtual void reset()
  { buf_pos = 0; }

  virtual bool process(Chunk2 &in, Chunk2 &out)
  {
    size_t buf_remains = buf_size - buf_pos;
    if (in.size < buf_remains)
    {
      copy_samples(buf, buf_pos, in.samples, 0, spk.nch(), in.size);
      buf_pos += in.size;

      in.set_empty();
      return false;
    }

    copy_samples(buf, buf_pos, in.samples, 0, spk.nch(), buf_remains);
    in.drop_samples(buf_remains);
    buf_pos = 0;

    out.set_linear(buf, buf_size);
    return true;
  }

  virtual bool flush(Chunk2 &out)
  {
    if (buf_pos <= 0)
      return false;

    out.set_linear(buf, buf_pos);
    buf_pos = 0;
    return true;
  }
};

///////////////////////////////////////////////////////////
// SmoothGain filter increases the gain smoothly at the
// beginning of the stream, preventing popping when
// the playback starts.
//
// This filter has an important property, that 
// process(block1) -> flush (and reset) -> process(block2) -> flush (and reset)
// does not equal to
// process(block1) -> process(block2) -> flush (and reset)

class SmoothGain : public SamplesFilter
{
protected:
  double current_gain;
  int current_sample;
  
public:
  double gain;
  int transition_samples;

  SmoothGain(): gain(1.0), transition_samples(0)
  {}

  SmoothGain(double gain_, int transition_samples_):
  gain(gain_), transition_samples(transition_samples_)
  {}

  /////////////////////////////////////////////////////////
  // SamplesFilter overrides

  virtual bool process(Chunk2 &in, Chunk2 &out)
  {
    out = in;
    in.set_empty();
    if (out.is_dummy())
      return false;

    const size_t size = out.size;
    if (!EQUAL_SAMPLES(gain, 1.0))
      for (int ch = 0; ch < spk.nch(); ch++)
        for (size_t s = 0; s < size; s++)
          out.samples[ch][s] *= gain;

    if (current_sample < transition_samples)
    {
      double dgain = 1.0 / transition_samples;
      size_t process_size = MIN(out.size, (size_t)(transition_samples - current_sample));

      for (int ch = 0; ch < spk.nch(); ch++)
        for (size_t s = 0; s < process_size; s++)
          out.samples[ch][s] *= current_gain + (dgain * s);

      current_gain += dgain * process_size;
      current_sample += process_size;
    }
    return true;
  }

  virtual void reset()
  {
    current_gain = 0;
    current_sample = 0;
  }

  virtual bool flush(Chunk2 &out)
  {
    current_gain = 0;
    current_sample = 0;
    return false;
  }
};

///////////////////////////////////////////////////////////////////////////////
// OfddSim
// Simulates ofdd filter behavior: output format is unknown after open, and
// changes after the first process() call.

class OfddSim : public Passthrough
{
protected:
  bool format_change;
  Speakers out_spk;

public:
  OfddSim() {};

  virtual bool init(Speakers new_spk)
  {
    reset();
    return true;
  }

  virtual void reset()
  { 
    format_change = false;
    out_spk = spk_unknown;
  }

  virtual bool process(Chunk2 &in, Chunk2 &out)
  {
    format_change = out_spk.is_unknown();
    if (out_spk.is_unknown())
      out_spk = spk;

    return Passthrough::process(in, out);
  }

  virtual bool new_stream() const
  { return format_change; }

  virtual Speakers get_output() const
  { return out_spk; }

};

TEST(filter_graph2, "FilterGraph2")

  /////////////////////////////////////////////////////////
  // Default FilterGraph2 must act as passthrough filter

  {
    FilterGraph2 graph_filter;
    NoiseGen noise1(spk, seed, noise_size);
    NoiseGen noise2(spk, seed, noise_size);
    CHECK(compare(log, &noise1, graph_filter, &noise2, 0) == 0);
  }

  /////////////////////////////////////////////////////////
  // FilterChain without filters = passthrough filter

  {
    FilterChain2 graph_filter;
    NoiseGen noise1(spk, seed, noise_size);
    NoiseGen noise2(spk, seed, noise_size);
    CHECK(compare(log, &noise1, graph_filter, &noise2, 0) == 0);
  }

  /////////////////////////////////////////////////////////
  // FilterGraph with one filter must act like this filter

  {
    double gain = 0.5;
    Gain ref_filter(gain);
    GainGraph graph_filter(gain);

    NoiseGen noise1(spk, seed, noise_size);
    NoiseGen noise2(spk, seed, noise_size);
    CHECK(compare(log, &noise1, graph_filter, &noise2, ref_filter) == 0);
  }

  /////////////////////////////////////////////////////////
  // FilterChain with one filter must act like this filter
  // * Passthrough filter does nothing
  // * Gain filter does not require flushing
  // * SmoothGain has a transition at the beginning
  // * LinearBuffer does nothing, but requires flushing
  // * OfddSim simulates ofdd behavior

  {
    double gain = 0.5;
    int    transition_samples = 10;
    // We need some data to remain in the buffer after
    // processing so buffer size should be fractional.
    const  size_t buf_size = size_t(noise_size / 2.5);

    // Reference filters we will compare to
    Passthrough  ref_pass;
    Gain         ref_gain(gain);
    SmoothGain   ref_smooth_gain(gain, transition_samples);
    LinearBuffer ref_linear_buffer(buf_size);
    OfddSim      ref_ofdd;

    // Filters to include into the chain
    Passthrough  tst_pass;
    Gain         tst_gain(gain);
    SmoothGain   tst_smooth_gain(gain, transition_samples);
    LinearBuffer tst_linear_buffer(buf_size);
    OfddSim      tst_ofdd;

    Filter2 *ref[] = { &ref_pass, &ref_gain, &ref_smooth_gain, &ref_linear_buffer, &ref_ofdd };
    Filter2 *tst[] = { &tst_pass, &tst_gain, &tst_smooth_gain, &tst_linear_buffer, &tst_ofdd };

    for (int i = 0; i < array_size(tst); i++)
    {
      FilterChain2 graph_filter;
      graph_filter.add_back(tst[i]);
      graph_filter.reset();

      NoiseGen noise1(spk, seed, noise_size);
      NoiseGen noise2(spk, seed, noise_size);
      CHECKT(compare(log, &noise1, graph_filter, &noise2, *ref[i]) == 0, 
        ("Chain with one filter (%s) fails with", typeid(*ref[i]).name()));
    }
  }

  /////////////////////////////////////////////////////////
  // FilterChain with 2 filters in different combinations
  // * Passthrough filter does nothing
  // * Gain filter does not require flushing
  // * SmoothGain has a transition at the beginning
  // * LinearBufferFilter requires flushing
  // * OfddSim simulates ofdd behavior
  // Any combination of these filters equals to SmoothGain
  // with different parameters. So we need only one
  // reference filter

  {
    const double gain = 0.5;
    const int transition_samples = 10;
    // We need some data to remain in the buffer after
    // processing so buffer size should be fractional.
    const size_t buf_size = size_t(noise_size / 2.5);

    // Filters to include into the chain
    Passthrough  tst_pass;
    Gain         tst_gain(gain);
    SmoothGain   tst_smooth_gain(gain, transition_samples);
    LinearBuffer tst_linear_buffer(buf_size);
    OfddSim      tst_ofdd;

    struct {
      Filter2 *f;
      double gain;
      int transition_samples;
    } tests[] = {
      { &tst_pass,          1.0,  0 },
      { &tst_gain,          gain, 0 },
      { &tst_smooth_gain,   gain, transition_samples },
      { &tst_linear_buffer, 1.0,  0 },
      { &tst_ofdd,          1.0,  0 }
    };

    for (int i = 0; i < array_size(tests); i++)
      for (int j = 0; j < array_size(tests); j++)
        if (i != j)
        {
          // filter chain
          FilterChain2 graph_filter;
          graph_filter.add_back(tests[i].f);
          graph_filter.add_back(tests[j].f);

          SmoothGain ref(
            tests[i].gain * tests[j].gain,
            tests[i].transition_samples + tests[j].transition_samples);

          // source and compare
          NoiseGen noise1(spk, seed, noise_size);
          NoiseGen noise2(spk, seed, noise_size);
          CHECKT(compare(log, &noise1, graph_filter, &noise2, ref) == 0,
            ("Chain with 2 filters fails: %s -> %s", typeid(*tests[i].f).name(), typeid(*tests[j].f).name()));
        }
  }

  /////////////////////////////////////////////////////////
  // Filter chain graph rebuild test
  // * Add a filter, test
  // * Add another filter and test again.
  // * Drop and test

  {
    const double gain = 0.5;
    // Reference filters we will compare to
    Gain         ref_gain1(gain);
    Gain         ref_gain2(gain*gain);
    // Filters to include into the chain
    Gain         tst_gain1(gain);
    Gain         tst_gain2(gain);

    FilterChain2 graph_filter;
    NoiseGen noise1(spk, seed, noise_size);
    NoiseGen noise2(spk, seed, noise_size);
    CHECK(compare(log, &noise1, graph_filter, &noise2, 0) == 0);

    graph_filter.add_back(&tst_gain1);
    noise1.init(spk, seed, noise_size);
    noise2.init(spk, seed, noise_size);
    CHECK(compare(log, &noise1, graph_filter, &noise2, ref_gain1) == 0);

    graph_filter.add_back(&tst_gain2);
    noise1.init(spk, seed, noise_size);
    noise2.init(spk, seed, noise_size);
    CHECK(compare(log, &noise1, graph_filter, &noise2, ref_gain2) == 0);

    graph_filter.clear();
    noise1.init(spk, seed, noise_size);
    noise2.init(spk, seed, noise_size);
    CHECK(compare(log, &noise1, graph_filter, &noise2, 0) == 0);
  }

  TEST_END(filter_graph2);

