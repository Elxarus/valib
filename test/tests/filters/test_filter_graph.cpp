#include <math.h>
#include "../../suite.h"
#include "buffer.h"
#include "source/generator.h"
#include "filters/gain.h"
#include "filters/filter_graph.h"

static const Speakers spk = Speakers(FORMAT_LINEAR, MODE_STEREO, 48000);
static const size_t noise_size = 64 * 1024;
static const int seed = 123123;

///////////////////////////////////////////////////////////////////////////////
// GainGraph owns a filter
// Test correct construction and destruction

class GainGraph : public FilterGraph
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

  virtual bool init()
  {
    buf_pos = 0;
    return buf.allocate(spk.nch(), buf_size);
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

      in.clear();
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
// StepGain
// Zeroes first samples of the stream and gain the rest.
//
// This filter has an important property, that 
// process(block1) -> flush -> reset -> process(block2) -> flush
// does not equal to
// process(block1) -> process(block2) -> flush
//
// This property help us to detect an excessive flushing.


class StepGain : public Gain
{
protected:
  int sample;
  
public:
  int step_samples;

  StepGain(): step_samples(0)
  {}

  StepGain(double gain_, int step_samples_):
  Gain(gain_), step_samples(step_samples_)
  {}

  /////////////////////////////////////////////////////////
  // SamplesFilter overrides

  virtual bool process(Chunk2 &in, Chunk2 &out)
  {
    if (!Gain::process(in, out))
      return false;

    if (sample < step_samples)
    {
      size_t zero_size = MIN(out.size, (size_t)(step_samples - sample));
      zero_samples(out.samples, spk.nch(), zero_size);
      sample += zero_size;
    }

    return true;
  }

  virtual void reset()
  { sample = 0; }

  virtual bool flush(Chunk2 &out)
  {
    sample = 0;
    return false;
  }
};

///////////////////////////////////////////////////////////////////////////////
// OfddMock
// Simulates ofdd filter behavior: output format is unknown after open, and
// changes after the first process() call.

class OfddMock : public Passthrough
{
protected:
  bool format_change;
  Speakers out_spk;

public:
  OfddMock() {};

  virtual bool init()
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

///////////////////////////////////////////////////////////////////////////////
// FormatChangeMock
// Simulates format change during processing

class FormatChangeMock : public Passthrough
{
protected:
  int chunk_count;
  bool format_change;

  int first_change;
  int second_change;

public:
  FormatChangeMock():
  first_change(-1), second_change(-1),
  chunk_count(0), format_change(false)
  {}

  FormatChangeMock(int first_change_, int second_change_):
  first_change(first_change_), second_change(second_change_),
  chunk_count(0), format_change(false)
  {}

  void set(int first_change_, int second_change_)
  {
    first_change = first_change_;
    second_change = second_change_;
  }

  virtual void reset()
  { chunk_count = 0; }

  virtual bool process(Chunk2 &in, Chunk2 &out)
  {
    if (in.is_dummy())
      return false;

    format_change = (chunk_count == first_change) || (chunk_count == second_change);
    chunk_count++;

    out = in;
    in.clear();
    return true;
  }

  virtual bool new_stream() const
  { return format_change; }
};

///////////////////////////////////////////////////////////////////////////////
// CallCounter
// Counts calls to Filter2 interface functions.

class CallCounter : public Filter2
{
protected:
  Speakers spk;

public:
  mutable int n_can_open;
  int n_open;
  int n_close;
  int n_reset;
  int n_process;
  int n_flush;
  mutable int n_new_stream;
  mutable int n_is_open;
  mutable int n_get_input;
  mutable int n_get_output;

  CallCounter()
  { reset_counters(); }

  void reset_counters()
  {
    n_can_open = 0;
    n_open = 0;
    n_close = 0;
    n_reset = 0;
    n_process = 0;
    n_flush = 0;
    n_new_stream = 0;
    n_is_open = 0;
    n_get_input = 0;
    n_get_output = 0;
  }

  virtual bool can_open(Speakers spk) const
  { n_can_open++; return true; }

  virtual bool open(Speakers spk_)
  {
    n_open++;
    spk = spk_;
    return !spk.is_unknown();
  }

  virtual void close()
  { n_close++; spk = spk_unknown; }

  virtual void reset()
  { n_reset++; }

  virtual bool process(Chunk2 &in, Chunk2 &out)
  {
    n_process++;
    out = in;
    in.clear();
    return !out.is_dummy();
  }

  virtual bool flush(Chunk2 &out)
  {
    n_flush++;
    return false;
  }

  virtual bool new_stream() const
  { n_new_stream++; return false; }

  virtual bool is_open() const
  { n_is_open++; return !spk.is_unknown(); }

  virtual bool is_ofdd() const
  { return false; }

  virtual Speakers get_input() const
  { n_get_input++; return spk; }

  virtual Speakers get_output() const
  { n_get_output++; return spk; }
};


TEST(filter_graph, "FilterGraph")

  /////////////////////////////////////////////////////////
  // Default FilterGraph must act as passthrough filter

  {
    FilterGraph graph_filter;
    NoiseGen noise1(spk, seed, noise_size);
    NoiseGen noise2(spk, seed, noise_size);
    CHECK(compare(log, &noise1, graph_filter, &noise2, 0) == 0);
  }

  /////////////////////////////////////////////////////////
  // FilterChain without filters = passthrough filter

  {
    FilterChain graph_filter;
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
  // * StepGain has a transition at the beginning
  // * LinearBuffer does nothing, but requires flushing
  // * OfddMock simulates ofdd behavior

  {
    double gain = 0.5;
    int    step_samples = 10;
    // We need some data to remain in the buffer after
    // processing so buffer size should be fractional.
    const  size_t buf_size = size_t(noise_size / 2.5);

    // Reference filters we will compare to
    Passthrough  ref_pass;
    Gain         ref_gain(gain);
    StepGain     ref_step_gain(gain, step_samples);
    LinearBuffer ref_linear_buffer(buf_size);
    OfddMock      ref_ofdd;

    // Filters to include into the chain
    Passthrough  tst_pass;
    Gain         tst_gain(gain);
    StepGain     tst_step_gain(gain, step_samples);
    LinearBuffer tst_linear_buffer(buf_size);
    OfddMock      tst_ofdd;

    Filter2 *ref[] = { &ref_pass, &ref_gain, &ref_step_gain, &ref_linear_buffer, &ref_ofdd };
    Filter2 *tst[] = { &tst_pass, &tst_gain, &tst_step_gain, &tst_linear_buffer, &tst_ofdd };

    for (int i = 0; i < array_size(tst); i++)
    {
      FilterChain graph_filter;
      graph_filter.add_back(tst[i]);
      graph_filter.open(spk);
      log->msg("Testing chain: %s", graph_filter.print_chain().c_str());

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
  // * StepGain has a transition at the beginning
  // * LinearBufferFilter requires flushing
  // * OfddMock simulates ofdd behavior
  // Any combination of these filters equals to StepGain
  // with different parameters. So we need only one
  // reference filter

  {
    const double gain = 0.5;
    const int step_samples = 10;
    // We need some data to remain in the buffer after
    // processing so buffer size should be fractional.
    const size_t buf_size = size_t(noise_size / 2.5);

    // Filters to include into the chain
    Passthrough  tst_pass;
    Gain         tst_gain(gain);
    StepGain     tst_step_gain(gain, step_samples);
    LinearBuffer tst_linear_buffer(buf_size);
    OfddMock     tst_ofdd;

    struct {
      Filter2 *f;
      double gain;
      int step_samples;
    } tests[] = {
      { &tst_pass,          1.0,  0 },
      { &tst_gain,          gain, 0 },
      { &tst_step_gain,     gain, step_samples },
      { &tst_linear_buffer, 1.0,  0 },
      { &tst_ofdd,          1.0,  0 }
    };

    for (int i = 0; i < array_size(tests); i++)
      for (int j = 0; j < array_size(tests); j++)
        if (i != j)
        {
          // filter chain
          FilterChain graph_filter;
          graph_filter.add_back(tests[i].f);
          graph_filter.add_back(tests[j].f);
          graph_filter.open(spk);
          log->msg("Testing chain: %s", graph_filter.print_chain().c_str());

          StepGain ref(
            tests[i].gain * tests[j].gain,
            tests[i].step_samples + tests[j].step_samples);

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

    FilterChain graph_filter;
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

  /////////////////////////////////////////////////////////
  // Format change in the chain should do the following:
  // * Flush downstream
  // * Rebuild the tail of the chain
  // * Set new_stream() flag

  {
    CallCounter counter;
    FormatChangeMock format_change;

    FilterChain graph_filter;
    graph_filter.add_back(&counter);

    // One open() and one flush() during regular processing
    NoiseGen noise1(spk, seed, noise_size);
    NoiseGen noise2(spk, seed, noise_size);
    CHECK(compare(log, &noise1, graph_filter, &noise2, 0) == 0);
    CHECK(counter.n_open == 1 && counter.n_flush == 1);
  }

  {
    int format_change_pos[][2] = {
      { 0, -1 }, { 1, -1 },
      { 0,  1 }, { 0,  2 }, { 1,  2 }, { 1,  3 }
    };

    for (int i = 0; i < array_size(format_change_pos); i++)
    {
      CallCounter counter;
      FormatChangeMock format_change(format_change_pos[i][0], format_change_pos[i][1]);

      FilterChain graph_filter;
      graph_filter.add_back(&format_change);
      graph_filter.add_back(&counter);

      // Fitler is open during the chain building
      // and after each format change
      int n_open = 1;
      if (format_change_pos[i][0] >= 0) n_open++;
      if (format_change_pos[i][1] >= 0) n_open++;

      // Filter is flushed after the end of processing
      // and each format change, excluding the case when
      // format changes at the first chunk of the stream.
      int n_flush = 1;
      if (format_change_pos[i][0] > 0) n_flush++;
      if (format_change_pos[i][1] > 0) n_flush++;

      NoiseGen noise1(spk, seed, noise_size);
      NoiseGen noise2(spk, seed, noise_size);
      CHECK(compare(log, &noise1, graph_filter, &noise2, 0) == 0);
      CHECK(counter.n_open == n_open && counter.n_flush == n_flush);
    }
  }

  {
    // Count new_stream()'s
    int format_change_pos[][2] = {
      { -1, -1 }, { 0, -1 }, { 1, -1 },
      { 0,  1 }, { 0,  2 }, { 1,  2 }, { 1,  3 }
    };

    for (int i = 0; i < array_size(format_change_pos); i++)
    {
      int format_changes = 0;
      int n_new_stream = 0;
      if (format_change_pos[i][0] >= 0) format_changes++;
      if (format_change_pos[i][1] >= 0) format_changes++;

      FormatChangeMock format_change(format_change_pos[i][0], format_change_pos[i][1]);

      FilterChain graph_filter;
      graph_filter.add_back(&format_change);
      graph_filter.open(spk);
      CHECK(graph_filter.is_open());

      NoiseGen noise(spk, seed, noise_size);
      while (!noise.is_empty())
      {
        Chunk src_chunk;
        noise.get_chunk(&src_chunk);

        Chunk2 in(src_chunk), out;
        while (graph_filter.process(in, out))
          if (graph_filter.new_stream())
            n_new_stream++;
      }

      Chunk2 out;
      while (graph_filter.flush(out))
        if (graph_filter.new_stream())
          n_new_stream++;

      CHECK(n_new_stream == format_changes);
    }
  }

TEST_END(filter_graph);
