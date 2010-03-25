#include <math.h>
#include "../../suite.h"
#include "buffer.h"
#include "source/generator.h"
#include "filters/gain.h"
#include "filters/filter_graph2.h"

static const Speakers spk = Speakers(FORMAT_LINEAR, MODE_STEREO, 48000);
static const size_t noise_size = 64 * 1024;
static const int seed = 123123;

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
// LinearBufferFilter
// Simplest buffering filter. Fills the buffer and passes it through without
// any change. Requires flushing when some data is in the buffer.

class LinearBufferFilter : public SamplesFilter
{
protected:
  size_t buf_size;
  size_t buf_pos;
  SampleBuf buf;

public:
  LinearBufferFilter(size_t buf_size): buf_size(buf_size)
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

  {
    double gain = 0.5;
    Gain ref_filter(gain);
    Gain gain_filter(gain);
    FilterChain2 graph_filter;
    graph_filter.add_back(&gain_filter, 0);

    NoiseGen noise1(spk, seed, noise_size);
    NoiseGen noise2(spk, seed, noise_size);
    CHECK(compare(log, &noise1, graph_filter, &noise2, ref_filter) == 0);
  }

  /////////////////////////////////////////////////////////
  // FilterChain with one buffering filter

  {
    // We need some data to remain in the buffer after
    // processing so buffer size should be fractional.
    const size_t buf_size = size_t(noise_size / 2.5);

    LinearBufferFilter buf_filter(buf_size);
    FilterChain2 graph_filter;
    graph_filter.add_back(&buf_filter, 0);

    NoiseGen noise1(spk, seed, noise_size);
    NoiseGen noise2(spk, seed, noise_size);
    CHECK(compare(log, &noise1, graph_filter, &noise2, 0) == 0);
  }



TEST_END(filter_graph2);

