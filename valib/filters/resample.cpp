#include <string.h>
#include "resample.h"
#include "../divisors.h"
#include "../dsp/kaiser.h"
#include "../dsp/fftsg_ld.h"

#define SAFE_DELETE(x) { if (x) delete(x); x = 0; }
static const double k_conv = 2;
static const double k_fft = 20.1977305724455;

///////////////////////////////////////////////////////////////////////////////
// Math

inline int div_floor(int a, int b) { return a/b; }
inline int div_ceil(int a, int b)  { return (a+b-1)/b; }
inline double sinc(double x) { return x == 0 ? 1 : sin(x)/x; }
inline double lpf(int i, double freq) { return 2 * freq * sinc(i * 2 * M_PI * freq); }
inline int gcd(int x, int y);
inline unsigned int flp2(unsigned int x);
inline unsigned int clp2(unsigned int x);

///////////////////////////////////////////////////////////////////////////////
// Resample class definition
///////////////////////////////////////////////////////////////////////////////

Resample::Resample(): 
  NullFilter(FORMAT_MASK_LINEAR),
  a(100.0), q(0.99), fs(0), fd(0), nch(0), 
  g(0), l(0), m(0), l1(0), l2(0), m1(0), m2(0),
  n1(0), n1x(0), n1y(0),
  f1_raw(0), f1(0), order(0),
  n2(0), n2b(0), f2(0),
  fft_ip(0), fft_w(0)
{
  for (int i = 0; i < NCHANNELS; i++)
  {
    buf1[i] = 0;
    buf2[i] = 0;
    delay2[i] = 0;
  }

  sample_rate = 0;
  out_spk.set_unknown();
  out_samples.zero();
  out_size = 0;
};

Resample::Resample(int _sample_rate, double _a, double _q): 
  NullFilter(FORMAT_MASK_LINEAR),
  a(100.0), q(0.99), fs(0), fd(0), nch(0), 
  g(0), l(0), m(0), l1(0), l2(0), m1(0), m2(0),
  n1(0), n1x(0), n1y(0),
  c1(0), c1x(0), c1y(0),
  f1_raw(0), f1(0), order(0),
  n2(0), n2b(0), c2(0), f2(0),
  fft_ip(0), fft_w(0)
{
  for (int i = 0; i < NCHANNELS; i++)
  {
    buf1[i] = 0;
    buf2[i] = 0;
    delay2[i] = 0;
  }

  sample_rate = 0;
  out_spk.set_unknown();
  out_samples.zero();
  out_size = 0;

  set(_sample_rate, _a, _q);
};

Resample::~Resample()
{
  uninit();
}



///////////////////////////////////////////////////////////////////////////////
// User interface
///////////////////////////////////////////////////////////////////////////////

bool
Resample::set(int _sample_rate, double _a, double _q)
{
  if (_sample_rate < 0) return false;
  if (_a < 6) return false;
  if (_q < 0.1) return false;
  if (_q >= 0.9999999999) return false;
  
  sample_rate = _sample_rate;
  a = _a;
  q = _q;

  uninit();
  out_spk = spk;
  if (!spk.is_unknown() && sample_rate != 0)
  {
    out_spk.sample_rate = sample_rate;
    if (sample_rate > spk.sample_rate)
      init_upsample(spk.nch(), spk.sample_rate, sample_rate);
    if (sample_rate < spk.sample_rate)
      init_downsample(spk.nch(), spk.sample_rate, sample_rate);
  }

  return true;
}

///////////////////////////////////////////////////////////////////////////////
// Filter interface
///////////////////////////////////////////////////////////////////////////////

void
Resample::reset()
{
  NullFilter::reset();
  if (sample_rate)
  {
    if (sample_rate > spk.sample_rate)
      reset_upsample();
    if (sample_rate < spk.sample_rate)
      reset_downsample();
  }
}

bool
Resample::set_input(Speakers _spk)
{
  FILTER_SAFE(NullFilter::set_input(_spk));

  uninit();
  out_spk = _spk;

  if (sample_rate)
  {
    out_spk.sample_rate = sample_rate;
    if (sample_rate > spk.sample_rate)
      init_upsample(spk.nch(), spk.sample_rate, sample_rate);
    if (sample_rate < spk.sample_rate)
      init_downsample(spk.nch(), spk.sample_rate, sample_rate);
  }
  return true;
}

Speakers
Resample::get_output() const
{
  return out_spk;
}

bool
Resample::get_chunk(Chunk *_chunk)
{
  if (sample_rate)
  {
    if (sample_rate > spk.sample_rate)
    {
      // Upsample
      if (size)
      {
        size_t processed = process_upsample(samples.samples, size);
        drop_samples(processed);
        _chunk->set_linear(out_spk, out_samples, out_size);
      }
      else if (flushing)
      {
        bool send_eos = flush_upsample();
        _chunk->set_linear(out_spk, out_samples, out_size);
        if (send_eos)
        {
          _chunk->set_eos();
          reset();
        }
      }
      return true;
    }

    if (sample_rate < spk.sample_rate)
    {
      // Downsample
      if (size)
      {
        size_t processed = process_downsample(samples.samples, size);
        drop_samples(processed);
        _chunk->set_linear(out_spk, out_samples, out_size);
      }
      else if (flushing)
      {
        bool send_eos = flush_downsample();
        _chunk->set_linear(out_spk, out_samples, out_size);
        if (send_eos)
        {
          _chunk->set_eos();
          reset();
        }
      }
      return true;
    }
  }

  send_chunk_inplace(_chunk, size);
  return true;
}


///////////////////////////////////////////////////////////////////////////////
// Init functions
///////////////////////////////////////////////////////////////////////////////

int
Resample::init_upsample(int _nch, int _fs, int _fd)
{
  int i;
  uninit();

  assert(_fs < _fd);

  fs = _fs; // source sample rate
  fd = _fd; // destinationsample rate
  nch = _nch; // number fo channels

  g = gcd(fs, fd);
  l = fd / g; // interpolation factor
  m = fs / g; // decimation factor

  l1 = l;                   // convolution stage interpolation factor (always L)
  l2 = 1;                   // fft stage interpolation factor (always 1)
  m2 = optimize_upsample(); // fft stage decimation factor
  m1 = m / m2;              // convolution stage decimation factor
  assert((m % m2) == 0);    // just in case...

  // fs1 is double because fs and fd may be coprime (gcd = 1)
  // so fs*fd may be > 2^32
  double fs1 = double(fs) * double(fd) / double(g);   // sample rate after interpolation [Hz]
  double fs2 = double(fd) * double(m2);               // sample rate after convolution stage [Hz]
  double alpha = kaiser_alpha(a);                     // alpha parameter for kaiser window

  ///////////////////////////////////////////////////////////////////////////
  // Build convolution stage filter

  // in df1 we take in account transition band df,
  // otherwise filter length could be too large for
  // close frequencies like (44100->44101)
  double df = fs * (1-q);
  double df1  = (fs2 - fs)/2 + df; // transition band width [Hz]
  double lpf1 = fs/2 - df + df1/2; // center of the transition band [Hz]
  df1 /= fs1; lpf1 /= fs1;         // normalize

  // filter length must be odd (type 1 filter)
  n1 = kaiser_n(a + log10(m1)*10, df1);
  n1x = n1 / l1;
  n1x = n1x | 1; // make odd (larger)
  n1y = l1;
  n1 = n1x * n1y;
  n1 = n1 - 1 | 1; // make odd (smaller)
  c1 = (n1 - 1) / 2; // center of the filter

  // allocate the filter
  // f1[n1y][n1x]
  f1 = new sample_t *[n1y];
  f1[0] = new sample_t[n1x * n1y];
  for (i = 0; i < n1x * n1y; i++) f1[0][i] = 0;
  for (i = 1; i < n1y; i++) f1[i] = f1[0] + i * n1x;

  f1_raw = new sample_t[n1y * n1x];
  for (i = 0; i < n1x * n1y; i++) f1_raw[i] = 0;

  // build the filter
  for (i = 0; i < n1; i++)
    f1_raw[i] = (sample_t) (kaiser_window(i - c1, n1, alpha) * lpf(i - c1, lpf1) * l);

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

  double lpf2 = (fs - df)/2;
  lpf2 /= fs2;

  // filter length must be odd (type 1 filter), but fft length must be even;
  // therefore n2 is even, but only n2-1 bins will be used for the filter
  n2 = kaiser_n(a + log10(m2)*10, df/fs2);
  n2 = clp2(n2);
  n2b = n2*2;
  c2 = n2 / 2 - 1;

  // allocate the filter
  // f2[n2b]
  f2 = new sample_t[n2b];
  for (i = 0; i < n2b; i++) f2[i] = 0;

  // make the filter
  // filter length is n2-1
  for (i = 0; i < n2-1; i++)
    f2[i] = (sample_t)(kaiser_window(i - c2, n2, alpha) * lpf(i - c2, lpf2) / n2);

  // convert the filter to frequency domain and init fft for future use
  fft_ip    = new int[(int)(2 + sqrt(n2b))];
  fft_w     = new sample_t[n2b/2];
  fft_ip[0] = 0;

  rdft(n2b, 1, f2, fft_ip, fft_w);

  ///////////////////////////////////////////////////////
  // Allocate buffers

  const size_t buf1_size = n2*m1/l1+n1x;
  buf1[0] = new sample_t[buf1_size * nch];
  for (i = 1; i < nch; i++)
    buf1[i] = buf1[0] + i * buf1_size;

  buf2[0] = new sample_t[n2b * nch];
  for (i = 1; i < nch; i++)
    buf2[i] = buf2[0] + i * n2b;

  const size_t delay2_size = n2/m2;
  delay2[0] = new sample_t[delay2_size * nch];
  for (i = 1; i < nch; i++)
    delay2[i] = delay2[0] + i * delay2_size;

  out_samples.zero();
  for (i = 0; i < nch; i++)
    out_samples[i] = buf2[i];

  out_size = 0;
  reset();

#if RESAMPLE_PERF
  stage1.reset();
  stage2.reset();
#endif

  return true;
}

int
Resample::init_downsample(int _nch, int _fs, int _fd)
{
  int i;
  uninit();

  assert(_fd < _fs);

  fs = _fs; // source sample rate
  fd = _fd; // destinationsample rate
  nch = _nch; // number fo channels

  g = gcd(fs, fd);
  l = fd / g; // interpolation factor
  m = fs / g; // decimation factor

  l2 = optimize_downsample(); // fft stage interpolation factor
  l1 = l / l2;                // convolution stage interpolation factor
  m2 = 1;                     // fft stage decimation factor (always 1)
  m1 = m;                     // convolution stage decimation factor (always M)
  assert((l % l2) == 0);      // just in case...

  // fs1 is double because fs and fd may be coprime (gcd = 1)
  // so fs*fd may be > 2^32
  double fs2 = double(fs) * double(l2);               // sample rate after convolution stage [Hz]
  double fs1 = double(fs) * double(fd) / double(g);   // sample rate after interpolation [Hz]
  double alpha = kaiser_alpha(a);                     // alpha parameter for kaiser window

  ///////////////////////////////////////////////////////////////////////////
  // Build fft stage filter

  double df = fd * (1-q);
  double lpf2 = (fd - df)/2;
  lpf2 /= fs2;

  // filter length must be odd (type 1 filter), but fft length must be even;
  // therefore n2 is even, but only n2-1 bins will be used for the filter
  n2 = kaiser_n(a + log10(m2)*10, df/fs2);
  n2 = clp2(n2);
  n2b = n2*2;
  c2 = n2 / 2 - 1;

  // allocate the filter
  // f2[n2b]
  f2 = new sample_t[n2b];
  for (i = 0; i < n2b; i++) f2[i] = 0;

  // make the filter
  // filter length is n2-1
  for (i = 0; i < n2-1; i++)
    f2[i] = (sample_t)(kaiser_window(i - c2, n2-1, alpha) * lpf(i - c2, lpf2) / n2);

  // convert the filter to frequency domain and init fft for future use
  fft_ip    = new int[(int)(2 + sqrt(n2b))];
  fft_w     = new sample_t[n2b/2];
  fft_ip[0] = 0;

  rdft(n2b, 1, f2, fft_ip, fft_w);

  ///////////////////////////////////////////////////////////////////////////
  // Build convolution stage filter

  // in df1 we take in account transition band df,
  // otherwise filter length could be too large for
  // close frequencies like (44100->44101)
  double df1  = (fs2 - fd)/2 + df; // transition band width [Hz]
  double lpf1 = fd/2 - df + df1/2; // center of the transition band [Hz]
  df1 /= fs1; lpf1 /= fs1;         // normalize

  // filter length must be odd (type 1 filter)
  n1 = kaiser_n(a + log10(m1)*10, df1);
  n1x = n1 / l1;
  n1x = n1x | 1; // make odd (larger)
  n1y = l1;
  n1 = n1x * n1y;
  n1 = n1 - 1 | 1; // make odd (smaller)
  c1 = (n1 - 1) / 2; // center of the filter

  // allocate the filter
  // f1[n1y][n1x]
  f1 = new sample_t *[n1y];
  f1[0] = new sample_t[n1x * n1y];
  for (i = 0; i < n1x * n1y; i++) f1[0][i] = 0;
  for (i = 1; i < n1y; i++) f1[i] = f1[0] + i * n1x;

  f1_raw = new sample_t[n1y * n1x];
  for (i = 0; i < n1x * n1y; i++) f1_raw[i] = 0;

  // build the filter
  for (i = 0; i < n1; i++)
    f1_raw[i] = (sample_t) (kaiser_window(i - c1, n1, alpha) * lpf(i - c1, lpf1) * l);

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

  ///////////////////////////////////////////////////////
  // init processing

  const size_t buf1_size = n2*m1/l1+n1x;
  buf1[0] = new sample_t[buf1_size * nch];
  for (i = 1; i < nch; i++)
    buf1[i] = buf1[0] + i * buf1_size;

  buf2[0] = new sample_t[n2b * nch];
  for (i = 1; i < nch; i++)
    buf2[i] = buf2[0] + i * n2b;

  const size_t delay2_size = n2/m2;
  delay2[0] = new sample_t[delay2_size * nch];
  for (i = 1; i < nch; i++)
    delay2[i] = delay2[0] + i * delay2_size;

  out_samples.zero();
  for (i = 0; i < nch; i++)
    out_samples[i] = buf2[i];

  out_size = 0;
  reset();

#if RESAMPLE_PERF
  stage1.reset();
  stage2.reset();
#endif

  return true;
}

void
Resample::reset_upsample()
{
  int ch;
  if (fs && fd)
  {
    pos_l = c1y;
    pos_m = pos_l * m1 / l1;

    pre_samples = c2;
    post_samples = c1x;

    pos1 = c1x;
    pos2 = 0;
    shift = 0;

    for (ch = 0; ch < nch; ch++)
      memset(buf1[ch], 0, pos1 * sizeof(sample_t));

    for (ch = 0; ch < nch; ch++)
      memset(delay2[ch], 0, n2/m2 * sizeof(sample_t));
  }
}

void
Resample::reset_downsample()
{
  int ch;
  if (fs && fd)
  {
    pos_l = 0;
    pos_m = pos_l * m1 / l1;

    pre_samples = stage1_out(c2 - c1x);
    post_samples = 0;

    pos1 = 0;
    pos2 = 0;
    shift = 0;

    for (ch = 0; ch < nch; ch++)
      memset(buf1[ch], 0, pos1 * sizeof(sample_t));

    for (ch = 0; ch < nch; ch++)
      memset(delay2[ch], 0, n2/m2 * sizeof(sample_t));
  }
}

void
Resample::uninit()
{
  if (f1) SAFE_DELETE(f1[0]);
  SAFE_DELETE(f1);
  SAFE_DELETE(f1_raw);
  SAFE_DELETE(order);
  SAFE_DELETE(f2);
  SAFE_DELETE(fft_ip);
  SAFE_DELETE(fft_w);

  SAFE_DELETE(buf1[0]);
  SAFE_DELETE(buf2[0]);
  SAFE_DELETE(delay2[0]);

  fs = 0; fd = 0;
  g = 0; l = 0; m = 0; l1 = 0; l2 = 0; m1 = 0; m2 = 0;
  n1 = 0; n1x = 0; n1y = 0;
  c1 = 0; c1x = 0; c1y = 0;
  f1_raw = 0; f1 = 0; order = 0;
  n2 = 0; n2b = 0; c2 = 0; f2 = 0;
  fft_ip = 0; fft_w = 0;

  out_samples.zero();
  for (int i = 0; i < NCHANNELS; i++)
  {
    buf1[i] = 0;
    buf2[i] = 0;
    delay2[i] = 0;
  }
}



///////////////////////////////////////////////////////////////////////////////
// Processing functions
///////////////////////////////////////////////////////////////////////////////

inline void
Resample::do_stage1(sample_t *in[], sample_t *out[], int n_in, int n_out)
{
  assert(n_in == stage1_in(n_out) && n_out == stage1_out(n_in));

#if RESAMPLE_PERF
  stage1.start();
#endif

  for (int ch = 0; ch < nch; ch++)
  {
    int i = pos_l;
    int n = n_out;
    sample_t *iptr = in[ch] - pos_m;
    sample_t *optr = out[ch] - pos_l;

    while (n--)
    {
      double sum = 0;
      for (int j = 0; j < n1x; j++)
        sum += iptr[order[i] + j] * f1[i][j];
      optr[i] = sum;

      i++;
      if (i >= l1)
      {
        i = 0;
        iptr += m1;
        optr += l1;
      }
    }
  }
  pos_m = (pos_m + n_in) % m1;
  pos_l = (pos_l + n_out) % l1;

#if RESAMPLE_PERF
  stage1.stop();
#endif
}

inline void
Resample::do_stage2()
{
#if RESAMPLE_PERF
  stage2.start();
#endif

  for (int ch = 0; ch < nch; ch++)
  {
    memset(buf2[ch] + n2, 0, n2 * sizeof(sample_t));

    rdft(n2b, 1, buf2[ch], fft_ip, fft_w);

    buf2[ch][0] = f2[0] * buf2[ch][0];
    buf2[ch][1] = f2[1] * buf2[ch][1]; 

    for (int i = 1; i < n2; i++)
    {
      sample_t re,im;
      re = f2[i*2  ] * buf2[ch][i*2] - f2[i*2+1] * buf2[ch][i*2+1];
      im = f2[i*2+1] * buf2[ch][i*2] + f2[i*2  ] * buf2[ch][i*2+1];
      buf2[ch][i*2  ] = re;
      buf2[ch][i*2+1] = im;
    }

    rdft(n2b, -1, buf2[ch], fft_ip, fft_w);
  }

#if RESAMPLE_PERF
  stage2.stop();
#endif
}

int
Resample::process_upsample(sample_t *in_buf[], int nsamples)
{
  int ch, i, j;
  int processed = 0;
  out_size = 0;

  ///////////////////////////////////////////////////////
  // Stage 1 processing

  int n = n2*m1/l1 + n1x;
  if (nsamples < n - pos1)
  {
    for (ch = 0; ch < nch; ch++)
      memcpy(buf1[ch] + pos1, in_buf[ch], nsamples * sizeof(sample_t));
    pos1 += nsamples;
    return nsamples;
  }
  for (ch = 0; ch < nch; ch++)
    memcpy(buf1[ch] + pos1, in_buf[ch], (n - pos1) * sizeof(sample_t));
  processed = n - pos1;

  int n_out = n2 - pos2;
  int n_in = stage1_in(n2 - pos2);
  do_stage1(buf1, buf2, n_in, n_out);

  pos1 = n - n_in;
  for (ch = 0; ch < nch; ch++)
    memmove(buf1[ch], buf1[ch] + n_in, pos1 * sizeof(sample_t));

  ///////////////////////////////////////////////////////
  // Stage 2 processing

  do_stage2();

  // Decimate and overlap
  // The size of output data is always less or equal to
  // the size of input data. Therefore we can process it
  // in-place.

  for (ch = 0; ch < nch; ch++)
    for (i = shift, j = 0; i < n2; i += m2, j++)
      buf2[ch][j] = buf2[ch][i] + delay2[ch][j];
  shift = i - n2;
  out_size = j;

  for (ch = 0; ch < nch; ch++)
    for (i = n2 + shift, j = 0; i < n2b; i += m2, j++)
      delay2[ch][j] = buf2[ch][i];

  ///////////////////////////////////////////////////////
  // Drop null samples from the beginning

  if (pre_samples > 0)
    if (pre_samples > out_size)
    {
      pre_samples -= out_size;
      out_size = 0;
    }
    else
    {
      out_size -= pre_samples;
      for (int ch = 0; ch < nch; ch++)
        memmove(out_samples[ch], out_samples[ch] + pre_samples, out_size * sizeof(sample_t));
      pre_samples = 0;
    }

  return processed;
}

bool
Resample::flush_upsample()
{
  int ch;

  out_size = 0;
  int n = n2*m1/l1 + n1x - pos1;
  int actual_out_size = stage1_out(pos1 - c1x) + c2 - pre_samples;
  if (!actual_out_size)
    return true;

  for (ch = 0; ch < nch; ch++)
    memset(buf1[ch] + pos1, 0, n * sizeof(sample_t));
  post_samples -= n;
  pos1 += n;

  process_upsample(samples.samples, 0);

  if (post_samples <= 0)
  {
    out_size = actual_out_size;
    return true;
  }
  return false;
}

int
Resample::process_downsample(sample_t *in_buf[], int nsamples)
{
  int ch, i;
  int processed = 0;
  out_size = 0;

  ///////////////////////////////////////////////////////
  // Stage 2 processing

  // fill the buffer

  if (nsamples < n2 - pos2)
  {
    for (ch = 0; ch < nch; ch++)
      memcpy(buf2[ch] + pos2, in_buf[ch], nsamples * sizeof(sample_t));
    pos2 += nsamples;
    return nsamples;
  }
  
  for (ch = 0; ch < nch; ch++)
    memcpy(buf2[ch] + pos2, in_buf[ch], (n2 - pos2) * sizeof(sample_t));
  processed = n2 - pos2;
  pos2 = 0;

  // convolution

  do_stage2();

  // overlap

  for (ch = 0; ch < nch; ch++)
    for (i = 0; i < n2; i++)
      buf2[ch][i] += delay2[ch][i];

  for (ch = 0; ch < nch; ch++)
    memcpy(delay2[ch], buf2[ch] + n2, n2 * sizeof(sample_t));

  ///////////////////////////////////////////////////////
  // Stage 1 processing
  // The size of output data is always less or equal to
  // the size of input data. Therefore we can process it
  // in-place.

  assert(n2 > n1x);
  out_size = stage1_out(n2);
  do_stage1(buf2, buf2, n2, out_size);

  ///////////////////////////////////////////////////////
  // Drop null samples from the beginning

  if (pre_samples > 0)
    if (pre_samples > out_size)
    {
      pre_samples -= out_size;
      out_size = 0;
    }
    else
    {
      out_size -= pre_samples;
      for (int ch = 0; ch < nch; ch++)
        memmove(out_samples[ch], out_samples[ch] + pre_samples, out_size * sizeof(sample_t));
      pre_samples = 0;
    }

  return processed;
}

bool
Resample::flush_downsample()
{
  out_size = 0;
  int actual_out_size = stage1_out(pos2 - c1x + c2) - pre_samples;
  if (!actual_out_size)
    return true;

  if (pos2)
  {
    for (int ch = 0; ch < nch; ch++)
      memset(buf2[ch] + pos2, 0, (n2 - pos2) * sizeof(sample_t));
    pos2 = n2;
    process_downsample(samples.samples, 0);
  }
  else
  {
    for (int ch = 0; ch < nch; ch++)
      memmove(out_samples[ch], out_samples[ch] + n2, n2/2 * sizeof(sample_t));
  }
  out_size = actual_out_size;
  return true;
}




///////////////////////////////////////////////////////////////////////////////
// Optimization functions
///////////////////////////////////////////////////////////////////////////////

double
Resample::t_upsample(int m2) const
{
  // upmix time approximation depending on stage2 decimation factor
  double alpha = (a - 7.95) / 14.36;
  double df = fs * (1-q);
  double t_conv = k_conv * 2 * alpha * double(fs) / double(m2 * fd - fs + 2 * df);
  double t_fft = k_fft * fd * m2 / double(fs) * log(2 * clp2(int(alpha * fd * m2 / double(df))));
  return t_conv + t_fft;
}

int
Resample::optimize_upsample() const
{
  int m2, m2_opt;
  double t, t_opt;

  /////////////////////////////////////////////////////////
  // Enum divisors of M

  m2_opt = 1; 
  t_opt = t_upsample(m2_opt);

  DivEnum d(m);
  d.next();                                    // exclude 1
  for (int i = 0; i < d.divisors() - 1 ; i++)  // include M
  {
    m2 = d.next();
    t = t_upsample(m2);
    if (t < t_opt)
    {
      t_opt = t;
      m2_opt = m2;
    }
  }

  return m2_opt;
}

int
Resample::optimize_downsample() const
{
  return 1;
}

///////////////////////////////////////////////////////////////////////////////
// Math
///////////////////////////////////////////////////////////////////////////////

inline int flp2(int x, int y)
{
  // largest power-of-2 <= x
  x = x | (x >> 1);
  x = x | (x >> 2);
  x = x | (x >> 4);
  x = x | (x >> 8);
  x = x | (x >> 16);
  return x - (x >> 1);
}

inline unsigned int clp2(unsigned int x)
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

inline int gcd(int x, int y)
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

