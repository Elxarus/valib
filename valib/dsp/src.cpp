#include <math.h>
#include <vector>
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#include <boost/bind.hpp>

#include "../buffer.h"
#include "kaiser.h"
#include "fft.h"

#include "src.h"

class SRCCore;
class SRCManager;
class SRCImpl;

static const double k_conv = 2;
static const double k_fft = 20.1977305724455;

///////////////////////////////////////////////////////////////////////////////
// Math

static inline double sinc(double x) { return x == 0 ? 1 : sin(x)/x; }
static inline double lpf(int i, double freq) { return 2 * freq * sinc(i * 2 * M_PI * freq); }
static inline int gcd(int x, int y);
static inline unsigned int flp2(unsigned int x);
static inline unsigned int clp2(unsigned int x);

static double t_upsample(int l1, int m1, int l2, int m2, double a, double q);
static double t_downsample(int l1, int m1, int l2, int m2, double a, double q);
static double optimize_upsample(int l, int m, double a, double q, int &l1, int &m1, int &l2, int &m2);
static double optimize_downsample(int l, int m, double a, double q, int &l1, int &m1, int &l2, int &m2);

/**************************************************************************//**
  \class SRCCore
  \brief Sample rate conversion core filters.

  SRC filters may require much of memory. At the same time it is typical when
  many converters perform the same conversion. Thus it is not reasonable to
  create new filters for each converter. Instead, many converters may share
  the same set of filters.

  This class holds filters and parameters for sample rate conversion shared
  along multiple converters.

  Lifetime is managed by shared pointers. It should not be created directly.
  SRCManager manages references to available converters and creates new when
  nessesary.
******************************************************************************/

class SRCCore
{
public:
  typedef boost::shared_ptr<SRCCore> shared_ptr;
  typedef boost::weak_ptr<SRCCore> weak_ptr;

  // Input parameters
  int l, m;   // l/m - interpolation/decimation factor
  double a;   // attenuation factor [dB]
  double q;   // quality (passband width)

  // useful in calculations
  double rate;// conversion rate
  int l1, l2; // stage1/stage2 interpolation factor
  int m1, m2; // stage1/stage2 decimation factor

  // convolution stage filter
  int n1, n1x, n1y; // filter length, x and y lengths
  int c1, c1x, c1y; // center of the filter, x and y coordinates
  sample_t **f1;    // reordered filter [n1y][n1x]
  Samples f1_raw;   // raw filter [n1y * n1x]
  int *order;       // input positions [l]

  // fft stage filter
  int n2, n2b;      // filter size and fft size
  int c2;           // center of the filter
  Samples f2;       // filter [n2b]

  // buffer sizes
  size_t stage1_size;
  size_t stage2_size;
  size_t delay_size;

  SRCCore(const SRCParams &params);
  ~SRCCore();

  bool same_core(const SRCParams &params) const;
  static bool is_valid(const SRCParams &params);
};

SRCCore::SRCCore(const SRCParams &params)
{
  int i;

  assert(is_valid(params));
  int g = gcd(params.fs, params.fd);
  l = params.fd / g;
  m = params.fs / g;
  a = params.a;
  q = params.q;
  rate = double(l) / double(m);

  if (m < l)
    optimize_upsample(l, m, a, q, l1, m1, l2, m2);
  else
    optimize_downsample(l, m, a, q, l1, m1, l2, m2);

  ///////////////////////////////////////////////////////////////////////////
  // We can consider the attenuation as amount of noise introduced by a filter.
  // Two stages introduces twice more noise, so to keep the output noise below
  // the user-specified, we should add 6dB attenuation to both stages.
  // Also, the noise in the stopband, produced at each stage is folded into
  // the passband during decimation. Decimation factor is the noise gain level,
  // so we should add it to the attenuation.

  double alpha; // alpha parameter for the kaiser window
  double a1 = a + log10(double(m1))*20 + 6; // convolution stage attenuation
  double a2 = a + log10(double(m2))*20 + 6; // fft stage attenuation

  ///////////////////////////////////////////////////////////////////////////
  // Find filters' parameters: transition band width and cennter frequency

  double phi = double(l1) / double(m1);
  double df1, lpf1, df2, lpf2;

  if (m < l) // upsample
  {
    // convolution stage
    df1  = (phi - q) / (2 * l1);
    lpf1 = (phi + q) / (4 * l1);
    // fft stage
    df2  = (1 - q) / (2 * phi * l2);
    lpf2 = (1 + q) / (4 * phi * l2);
  }
  else // downsample
  {
    // convolution stage
    df1  = (phi - q * rate) / (2 * l1);
    lpf1 = (phi + q * rate) / (4 * l1);
    // fft stage
    df2  = rate * (1 - q) / (2 * phi * l2);
    lpf2 = rate * (1 + q) / (4 * phi * l2);
  }

  ///////////////////////////////////////////////////////////////////////////
  // Build convolution stage filter

  // find the fiter length
  n1 = kaiser_n(a1, df1) | 1;
  n1x = (n1 + l1 - 1) / l1;     // make n1x odd; larger, because we
  n1x = n1x | 1;     // should not make the filter weaker
  n1y = l1;
  n1 = n1x * n1y;    // use all available space for the filter
  n1 = n1 - 1 | 1;   // make n1 odd (type1 filter); smaller, because we
                     // must fit the filter into the given space
  c1 = (n1 - 1) / 2; // center of the filter

  // allocate the filter
  // f1[n1y][n1x]
  f1 = new sample_t *[n1y];
  f1[0] = new sample_t[n1x * n1y];
  for (i = 0; i < n1x * n1y; i++) f1[0][i] = 0;
  for (i = 1; i < n1y; i++) f1[i] = f1[0] + i * n1x;

  f1_raw.allocate(n1y * n1x);
  f1_raw.zero();

  // build the filter
  alpha = kaiser_alpha(a1);
  for (i = 0; i < n1; i++)
    f1_raw[i] = (sample_t) (kaiser_window(i - c1, n1, alpha) * lpf(i - c1, lpf1) * l1);

  // reorder the filter
  // find coordinates of the filter's center
  for (int y = 0; y < n1y; y++)
    for (int x = 0; x < n1x; x++)
    {
      int p = l1-1 - (y*m1)%l1 + x*l1;
      f1[y][x] = f1_raw[p];
      if (p == c1)
        c1x = x, c1y = y;
    }

  // data ordering
  order = new int[l1];
  for (i = 0; i < l1; i++) 
    order[i] = i * m1 / l1;

  ///////////////////////////////////////////////////////////////////////////
  // Build fft stage filter

  // filter length must be odd (type 1 filter), but fft length must be even;
  // therefore n2 is even, but only n2-1 bins will be used for the filter
  n2 = kaiser_n(a2, df2) | 1;
  n2 = clp2(n2);
  n2b = n2*2;
  c2 = n2 / 2 - 1;

  // allocate the filter
  // f2[n2b]
  f2.allocate(n2b);
  f2.zero();

  // make the filter
  // filter length is n2-1
  alpha = kaiser_alpha(a2);
  for (i = 0; i < n2-1; i++)
    f2[i] = (sample_t)(kaiser_window(i - c2, n2-1, alpha) * lpf(i - c2, lpf2) * l2 / n2);

  // init fft and convert the filter to frequency domain
  FFT fft(n2b);
  fft.rdft(f2);

  ///////////////////////////////////////////////////////
  // Buffer sizes

  stage1_size = n2*m1/l1+n1x+1;
  stage2_size = n2b;
  delay_size = n2/m2+1;
}

SRCCore::~SRCCore()
{
  if (f1) safe_delete(f1[0]);
  safe_delete(f1);
  safe_delete(order);
}

bool
SRCCore::same_core(const SRCParams &params) const
{
  static const double a_threshold = 0.1;   // attenuation threshold = 0.1dB
  static const double q_threshold = 0.001; // quality threshold = 0.1%
  int g = gcd(params.fs, params.fd);
  int other_l = params.fd / g;
  int other_m = params.fs / g;
  return
    l == other_l &&
    m == other_m &&
    fabs(a - params.a) < a_threshold &&
    fabs(q / params.q - 1) < q_threshold;
}

bool
SRCCore::is_valid(const SRCParams &params)
{
  if (params.fs <= 0 || params.fd <= 0 || params.fs == params.fd) return false;
  if (params.a < 6) return false;
  if (params.a > 200) return false;
  if (params.q < 0.1) return false;
  if (params.q >= 0.9999999999) return false;
  return true;
}

/**************************************************************************//**
  \class SRCManager
  \brief Manages a list of available SRC cores.

  get_core() returns a pointer to a core for the conversion defined by the
  parameters given.
******************************************************************************/

class SRCManager
{
public:
  SRCManager()
  {}

  SRCCore::shared_ptr get_core(const SRCParams &params)
  {
    bool fragmented = false;
    for (size_t i = 0; i < list.size(); i++)
    {
      SRCCore::shared_ptr p = list[i].lock();
      fragmented = fragmented || !p;
      if (p && p->same_core(params))
        return p;
    }

    if (fragmented)
      defragment();

    SRCCore::shared_ptr p(new SRCCore(params));
    list.push_back(p);
    return p;
  }

protected:
  std::vector<SRCCore::weak_ptr> list;

  void defragment()
  {
    // Defragment the list: remove expired references.
    std::vector<SRCCore::weak_ptr> new_list = list;
    new_list.erase(
      std::remove_if(
        new_list.begin(),
        new_list.end(),
        boost::bind(&SRCCore::weak_ptr::expired, _1)),
      new_list.end());
    list.swap(new_list);
  }
};

static SRCManager src_manager;

/**************************************************************************//**
  \class SRCImpl
  \brief Implementation of the sample rate converter.
******************************************************************************/

class SRCImpl
{
public:
  SRCImpl(const SRCParams &param);

  void reset();
  size_t fill(const sample_t *in, size_t size);

  bool can_process() const;
  void process();

  bool need_flushing() const;
  void flush();

  inline sample_t *result() const
  { return buf2; }
  inline size_t size() const
  { return out_size; }

protected:
  boost::shared_ptr<SRCCore> core;

  int l1, m1;
  int l2, m2;
  int n1x, n1y;
  int c1x, c1y;
  int n2, n2b, c2;

  // processing
  FFT fft;          // fft transformer
  Samples buf1;     // stage1 buffer
  Samples buf2;     // stage2 buffer [n2b]
  Samples delay2;   // fft stage delay buffer [n2/m2]

  int pos_l, pos_m; // stage1 convolution positions [0..l1), [0..m1)
  int pos1;         // stage1 buffer position
  int shift;        // fft stage decimation shift
  int pre_samples;  // number of samples to drop from the beginning of output data
  int post_samples; // number of samples to add to the end of input data
  int out_size;     // output number of samples

  // stage1_in(): how much input samples required to generate N output samples
  // stage1_out(): how much output samples can be made out of N input samples
  // Note that stage1_out(stage1_in(N)) >= N
  inline int stage1_in(int n)  const { return (n + pos_l) * m1 / l1 - pos_m; }
  inline int stage1_out(int n) const { return ((pos_m + n) * l1 + m1 - 1) / m1 - (pos_m * l1 + m1 - 1) / m1; }

  void do_stage1(sample_t *in, sample_t *out, int n_in, int n_out);
  void do_stage2();
};

SRCImpl::SRCImpl(const SRCParams &params)
{
  assert(SRCCore::is_valid(params));
  core = src_manager.get_core(params);
  assert(core);

  l1 = core->l1; m1 = core->m1;
  l2 = core->l2, m2 = core->m2;
  n1x = core->n1x; n1y = core->n1y;
  c1x = core->c1x; c1y = core->c1y;
  n2 = core->n2; n2b = core->n2b; c2 = core->c2;

  buf1.allocate(core->stage1_size);
  buf2.allocate(core->stage2_size);
  delay2.allocate(core->delay_size);
  fft.set_length(core->n2b);
  reset();
}

void
SRCImpl::reset()
{
  pos_l = c1y;
  pos_m = pos_l * m1 / l1;

  pre_samples = c2 / m2;
  post_samples = c1x;
  out_size = 0;

  // To avoid signal shift we add c1x zero samples to the beginning,
  // so the first sample processed is guaranteed to match the center
  // of the filter.
  // Also, we should choose 'shift' value in such way, so
  // shift + pre_samples*m2 = c2

  pos1 = c1x;
  shift = c2 - pre_samples * m2;

  zero_samples(buf1, pos1);
  zero_samples(delay2, n2 / m2 + 1);
}

size_t
SRCImpl::fill(const sample_t *in, size_t size)
{
  size_t n = size;
  if (size >= (size_t)core->stage1_size - pos1)
    n = (size_t)core->stage1_size - pos1;

  copy_samples(buf1, pos1, in, 0, n);
  pos1 += (int)n;
  return n;
}

bool
SRCImpl::can_process() const
{
  return (size_t)pos1 == core->stage1_size;
}

void
SRCImpl::process()
{
  int i, j;

  ///////////////////////////////////////////////////////
  // Stage 1 processing

  int n_out = n2;
  int n_in = stage1_in(n2);
  assert(pos1 >= n_in);

  do_stage1(buf1, buf2, n_in, n_out);

  pos1 -= n_in;
  move_samples(buf1, 0, buf1, n_in, pos1);

  ///////////////////////////////////////////////////////
  // Stage 2 processing

  do_stage2();

  // Decimate and overlap
  // The size of output data is always less or equal to
  // the size of input data. Therefore we can process it
  // in-place.

  i = shift; j = 0;
  for (i = shift, j = 0; i < n2; i += m2, j++)
    buf2[j] = buf2[i] + delay2[j];
  shift = i - n2;
  out_size = j;

  for (i = n2 + shift, j = 0; i < n2b; i += m2, j++)
    delay2[j] = buf2[i];

  ///////////////////////////////////////////////////////
  // Drop null samples from the beginning

  if (pre_samples > 0)
  {
    if (pre_samples > out_size)
    {
      pre_samples -= out_size;
      out_size = 0;
    }
    else
    {
      out_size -= pre_samples;
      move_samples(buf2, 0, buf2, pre_samples, out_size);
      pre_samples = 0;
    }
  }
}

bool
SRCImpl::need_flushing() const
{
  return
    post_samples >= 0 &&
    ((stage1_out(pos1 - c1x) + c2 - shift) / m2 - pre_samples) > 0;
}

void
SRCImpl::flush()
{
  int actual_out_size = (stage1_out(pos1 - c1x) + c2 - shift) / m2 - pre_samples;
  if (actual_out_size == 0)
    return;

  ///////////////////////////////////////////////////////
  // Zero the tail of the stage 1 buffer

  int n = (int)core->stage1_size - pos1;
  zero_samples(buf1 + pos1, n);
  post_samples -= n;
  pos1 += n;

  ///////////////////////////////////////////////////////
  // Resample & output

  process();

  if (post_samples <= 0)
  {
    // zero is correct value for post_samples, but we
    // must indicate we're finished to need_flushing().
    post_samples = -1;

    // If we have no enough data, then
    // copy the rest from the delay buffer
    if (actual_out_size > out_size)
      copy_samples(buf2, out_size, delay2, 0, actual_out_size - out_size);
    out_size = actual_out_size;
  }
  else
    pos1 -= n;
}

void
SRCImpl::do_stage1(sample_t *in, sample_t *out, int n_in, int n_out)
{
  assert(n_in == stage1_in(n_out) || n_out == stage1_out(n_in));

  int i = pos_l;
  int n = n_out;
  sample_t *iptr = in - pos_m;
  sample_t *optr = out - pos_l;
  int *order = core->order;
  sample_t **f1 = core->f1;

  // Now iptr points to the 'imaginary' beginning of the block of M input
  // samples and optr points to the beginning of the block of L output
  // samples, so pos_m and pos_l are indexes at these blocks.
  //
  // But here a special case is possible. Consider L=3, M=5 and pos_m=4
  // (4 input samples processed and 3 output samples generated). In this
  // case pos_l=0 and order[pos_l]=0. So in-pos_m+order[pos_l] < in.
  // Thus we must skip last (unused) samples of the input block.

  if (order[pos_l] < pos_m)
     iptr += m1;

  while (n--)
  {
    double sum = 0;
    for (int j = 0; j < n1x; j++)
      sum += iptr[order[i] + j] * f1[i][j];
    optr[i++] = sum;

    if (i >= l1)
    {
      i = 0;
      iptr += m1;
      optr += l1;
    }
  }

  pos_m = (pos_m + n_in) % m1;
  pos_l = (pos_l + n_out) % l1;
}

void
SRCImpl::do_stage2()
{
  sample_t *f2 = core->f2;
  zero_samples(buf2 + n2, n2);
  fft.rdft(buf2);

  buf2[0] = f2[0] * buf2[0];
  buf2[1] = f2[1] * buf2[1]; 

  for (int i = 1; i < n2; i++)
  {
    sample_t re,im;
    re = f2[i*2  ] * buf2[i*2] - f2[i*2+1] * buf2[i*2+1];
    im = f2[i*2+1] * buf2[i*2] + f2[i*2  ] * buf2[i*2+1];
    buf2[i*2  ] = re;
    buf2[i*2+1] = im;
  }
  fft.inv_rdft(buf2);
}

///////////////////////////////////////////////////////////////////////////////
// StreamingSRC

class StreamingSRC::Impl : public SRCImpl
{
public:
  StreamingSRC::Impl(const SRCParams &params):
  SRCImpl(params)
  {}
};

StreamingSRC::StreamingSRC(): pimpl(0), f_result(0), f_size(0)
{}

StreamingSRC::StreamingSRC(const SRCParams &params):
pimpl(0), f_result(0), f_size(0)
{
  open(params);
}

StreamingSRC::StreamingSRC(int fs, int fd, double a, double q):
pimpl(0), f_result(0), f_size(0)
{
  open(fs, fd, a, q);
}

StreamingSRC::~StreamingSRC()
{
  safe_delete(pimpl);
}

bool
StreamingSRC::open(const SRCParams &params)
{
  close();
  if (!SRCCore::is_valid(params))
    return false;

  f_params = params;
  pimpl = new Impl(params);
  return true;
}

bool
StreamingSRC::open(int fs, int fd, double a, double q)
{
  return open(SRCParams(fs, fd, a, q));
}

void
StreamingSRC::close()
{
  safe_delete(pimpl);
  f_result = 0;
  f_size = 0;
}

bool
StreamingSRC::is_open() const
{
  return pimpl != 0;
}

void
StreamingSRC::reset()
{
  if (pimpl)
    pimpl->reset();
  f_result = 0;
  f_size = 0;
}

size_t
StreamingSRC::fill(const sample_t *source, size_t size)
{
  f_result = 0;
  f_size = 0;
  if (pimpl)
    return pimpl->fill(source, size);
  return 0;
}

bool
StreamingSRC::can_process() const
{
  return pimpl? pimpl->can_process(): false;
}

void
StreamingSRC::process()
{
  f_result = 0;
  f_size = 0;
  if (pimpl)
  {
    pimpl->process();
    f_result = pimpl->result();
    f_size = pimpl->size();
  }
}

bool
StreamingSRC::need_flushing() const
{
  return pimpl? pimpl->need_flushing(): false;
}

void
StreamingSRC::flush()
{
  f_result = 0;
  f_size = 0;
  if (pimpl)
  {
    pimpl->flush();
    f_result = pimpl->result();
    f_size = pimpl->size();
  }
}

///////////////////////////////////////////////////////////////////////////////
// BufferSRC

class BufferSRC::Impl : public SRCImpl
{
public:
  BufferSRC::Impl(const SRCParams &params):
  SRCImpl(params)
  {}

  int fs, fd;
  Samples buf;
  size_t buf_data;

  void process(const sample_t *in, size_t size)
  {
    buf.allocate((size + 1) * core->l / core->m + 1);
    buf_data = 0;

    reset();
    while (size)
    {
      size_t gone = fill(in, size);
      in += gone;
      size -= gone;

      if (can_process())
      {
        SRCImpl::process();
        assert(buf_data + out_size <= buf.size());
        copy_samples(buf, buf_data, buf2, 0, out_size);
        buf_data += out_size;
      }
    }

    while (need_flushing())
    {
      flush();
      assert(buf_data + out_size <= buf.size());
      copy_samples(buf, buf_data, buf2, 0, out_size);
      buf_data += out_size;
    }
  }

};

BufferSRC::BufferSRC(): pimpl(0), f_result(0), f_size(0)
{}

BufferSRC::BufferSRC(const SRCParams &params):
pimpl(0), f_result(0), f_size(0)
{
  open(params);
}

BufferSRC::BufferSRC(int fs, int fd, double a, double q):
pimpl(0), f_result(0), f_size(0)
{
  open(fs, fd, a, q);
}

BufferSRC::~BufferSRC()
{
  safe_delete(pimpl);
}

bool
BufferSRC::open(const SRCParams &params)
{
  close();
  if (!SRCCore::is_valid(params))
    return false;

  f_params = params;
  pimpl = new Impl(params);
  return true;
}

bool
BufferSRC::open(int fs, int fd, double a, double q)
{
  return open(SRCParams(fs, fd, a, q));
}

void
BufferSRC::close()
{
  safe_delete(pimpl);
  f_result = 0;
  f_size = 0;
}

bool
BufferSRC::is_open() const
{
  return pimpl != 0;
}

void
BufferSRC::process(const sample_t *in, size_t size)
{
  if (!pimpl) return;
  pimpl->process(in, size);
  f_result = pimpl->buf;
  f_size = pimpl->buf_data;
}


///////////////////////////////////////////////////////////////////////////////
// Optimization functions
///////////////////////////////////////////////////////////////////////////////

static double t_upsample(int l1, int m1, int l2, int m2, double a, double q)
{
  double phi = double(l1) / double(m1);
  double alpha_conv = (a + log10(double(m1))*20 + 6 - 7.95) / 14.36;
  double alpha_fft  = (a + log10(double(m2))*20 + 6 - 7.95) / 14.36;

  double t_conv = 2 * alpha_conv * k_conv / (phi - q);
  double t_fft = k_fft * phi * l2 * log(double(2 * clp2(int(2 * alpha_fft * phi * l2 / (1 - q)))));
  return t_fft + t_conv;
}

static double t_downsample(int l1, int m1, int l2, int m2, double a, double q)
{
  double phi = double(l1) / double(m1);
  double rate = double(l1 * l2) / double(m1 * m2);
  double alpha_conv = (a + log10(double(m1))*20 + 6 - 7.95) / 14.36;
  double alpha_fft  = (a + log10(double(m2))*20 + 6 - 7.95) / 14.36;

  double t_conv = 2 * alpha_conv * k_conv / (phi - q * rate);
  double t_fft = k_fft * phi * l2 * log(double(2 * clp2(int(2 * alpha_fft * phi * l2 / rate / (1 - q)))));
  return t_fft + t_conv;
}

static double optimize_upsample(int l, int m, double a, double q, int &l1, int &m1, int &l2, int &m2)
{
  l1 = l; m1 = m;
  l2 = 1; m2 = 1;
  double t_opt = t_upsample(l, m, 1, 1, a, q);

  for (int m2i = 2; m2i < m; m2i++)
  {
    int g = gcd(l * m2i, m);
    double t = t_upsample(l * m2i / g, m / g, 1, m2i, a, q);
    if (t < t_opt)
    {
      t_opt = t;
      l1 = l * m2i / g;
      m1 = m / g;
      l2 = 1;
      m2 = m2i;
    }
    else if (t > 10 * t_opt)
      return t_opt;
  }
  return t_opt;
}

static double optimize_downsample(int l, int m, double a, double q, int &l1, int &m1, int &l2, int &m2)
{
  l1 = l; m1 = m;
  l2 = 1; m2 = 1;
  double t_opt = t_downsample(l, m, 1, 1, a, q);

  for (int m2i = 2; m2i < m; m2i++)
  {
    int g = gcd(l * m2i, m);
    double t = t_downsample(l * m2i / g, m / g, 1, m2i, a, q);
    if (t < t_opt)
    {
      t_opt = t;
      l1 = l * m2i / g;
      m1 = m / g;
      l2 = 1;
      m2 = m2i;
    }
    else if (t > 10 * t_opt)
      return t_opt;
  }
  return t_opt;
}

///////////////////////////////////////////////////////////////////////////////
// Math
///////////////////////////////////////////////////////////////////////////////

static inline int flp2(int x, int y)
{
  // largest power-of-2 <= x
  x = x | (x >> 1);
  x = x | (x >> 2);
  x = x | (x >> 4);
  x = x | (x >> 8);
  x = x | (x >> 16);
  return x - (x >> 1);
}

static inline unsigned int clp2(unsigned int x)
{
  // smallest power-of-2 >= x
  x = x - 1;
  x = x | (x >> 1);
  x = x | (x >> 2);
  x = x | (x >> 4);
  x = x | (x >> 8);
  x = x | (x >> 16);
  return x + 1;
}

static inline int gcd(int x, int y)
{
  int t;
  while (y != 0)
  {
    t = x % y; 
    x = y; 
    y = t;
  }
  return x;
}
